#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/DebugProgramInstruction.h"

#include <algorithm>
#include <string>

using namespace llvm;

namespace {

static std::string getDebugName(Function &F, Value *V) {
  if (!V)
    return "";

  // LLVM new debug-info format: DbgVariableRecord
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {

      if (!I.hasDbgRecords())
        continue;

      for (DbgRecord &DR : I.getDbgRecordRange()) {

        auto *DVR = dyn_cast<DbgVariableRecord>(&DR);
        if (!DVR)
          continue;

        DILocalVariable *Var = DVR->getVariable();
        if (!Var)
          continue;

        for (unsigned Op = 0;
             Op < DVR->getNumVariableLocationOps();
             ++Op) {

          Value *Loc =
              DVR->getVariableLocationOp(Op);

          if (Loc == V)
            return Var->getName().str();
        }
      }
    }
  }

  // Old dbg.value / dbg.declare representation
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {

      auto *DVI =
          dyn_cast<DbgVariableIntrinsic>(&I);

      if (!DVI)
        continue;

      DILocalVariable *Var =
          DVI->getVariable();

      if (!Var)
        continue;

      for (unsigned Op = 0;
           Op < DVI->getNumVariableLocationOps();
           ++Op) {

        Value *Loc =
            DVI->getVariableLocationOp(Op);

        if (Loc == V)
          return Var->getName().str();
      }
    }
  }

  return "";
}

/*
 * Find an actual source-level variable name from a value.
 *
 * Example:
 *     %x = phi ...
 *     dbg.value(..., %x, ...)
 *
 * returns "x".
 */
static std::string findVariableName(
    Function &F,
    Value *V) {

  if (!V)
    return "";

  SmallVector<Value *, 32> Worklist;
  SmallPtrSet<Value *, 32> Visited;

  Worklist.push_back(V);

  while (!Worklist.empty()) {

    Value *Cur =
        Worklist.pop_back_val();

    if (!Cur ||
        !Visited.insert(Cur).second)
      continue;

    if (std::string Name =
            getDebugName(F, Cur);
        !Name.empty()) {
      return Name;
    }

    if (auto *LI =
            dyn_cast<LoadInst>(Cur)) {

      Value *Ptr =
          LI->getPointerOperand()
              ->stripPointerCasts();

      if (auto *AI =
              dyn_cast<AllocaInst>(Ptr)) {

        if (std::string Name =
                getDebugName(F, AI);
            !Name.empty()) {
          return Name;
        }
      }
    }

    if (auto *I =
            dyn_cast<Instruction>(Cur)) {

      for (Value *Op : I->operands())
        Worklist.push_back(Op);
    }
  }

  return "";
}


/*
 * Get the loop induction variable.
 *
 * This is called only after:
 *
 *     mem2reg
 *     loop-simplify
 *
 * so the normal C loop variable becomes a PHI-based induction variable.
 */
static PHINode *findLoopInductionVariable(
    Loop *L,
    ScalarEvolution &SE) {

  if (!L)
    return nullptr;

  /*
   * First try LLVM's normal induction-variable detection.
   */
  if (PHINode *IV = L->getInductionVariable(SE))
    return IV;

  /*
   * Canonical induction variable fallback.
   */
  if (PHINode *IV = L->getCanonicalInductionVariable())
    return IV;

  return nullptr;
}

static bool isDFSanConditionalCallback(CallInst *CI) {
  if (!CI)
    return false;

  Value *Called =
      CI->getCalledOperand()->stripPointerCasts();

  Function *F = dyn_cast<Function>(Called);

  if (!F)
    return false;

  return F->getName() ==
         "__dfsan_conditional_callback";
}

static void collectConditionVariables(
    Function &F, // <--- Add F here
    Value *V,
    SmallVectorImpl<std::string> &Names,
    SmallPtrSetImpl<Value *> &Visited) {

  if (!V || !Visited.insert(V).second)
    return;

  if (auto *LI = dyn_cast<LoadInst>(V)) {

    Value *Ptr =
        LI->getPointerOperand()->stripPointerCasts();

    if (auto *AI = dyn_cast<AllocaInst>(Ptr)) {

      std::string Name =
          getDebugName(F, AI); // Now F is in scope

      if (!Name.empty())
        Names.push_back(Name);

      return;
    }
  }

  if (auto *PN = dyn_cast<PHINode>(V)) {

    std::string Name =
      getDebugName(F, PN); // Now F is in scope
    if (!Name.empty())
      Names.push_back(Name);
  }

  if (auto *I = dyn_cast<Instruction>(V)) {

    for (Value *Op : I->operands())
      collectConditionVariables(
          F, // <--- Pass F to the recursive call
          Op,
          Names,
          Visited);
  }
}

static std::string findImmediateVariableName(
    Function &F,
    Value *V) {

  if (!V)
    return "";

  // Direct source-variable mapping.
  std::string Name =
      getDebugName(F, V);

  if (!Name.empty())
    return Name;

  // Load -> alloca -> source variable.
  if (auto *LI = dyn_cast<LoadInst>(V)) {

    Value *Ptr =
        LI->getPointerOperand()
           ->stripPointerCasts();

    if (auto *AI = dyn_cast<AllocaInst>(Ptr)) {

      Name = getDebugName(F, AI);

      if (!Name.empty())
        return Name;
    }
  }

  return "";
}

static std::string getConditionVariableName(
    Function &F,
    Value *Condition) {

  if (!Condition)
    return "unknown";

  if (auto *Cmp =
          dyn_cast<ICmpInst>(Condition)) {

    for (Value *Op : Cmp->operands()) {

      if (isa<Constant>(Op))
        continue;

      std::string Name =
          findImmediateVariableName(F, Op);

      if (!Name.empty())
        return Name;
    }
  }

  return "unknown";
}


static Value *toI64(
    IRBuilder<> &IRB,
    Value *V) {

  Type *I64 =
      Type::getInt64Ty(
          IRB.getContext());

  if (!V)
    return ConstantInt::get(
        I64,
        0);

  if (V->getType() == I64)
    return V;

  if (V->getType()->isIntegerTy())
    return IRB.CreateSExtOrTrunc(
        V,
        I64);

  return ConstantInt::get(
      I64,
      0);
}

struct FindTaintedConditionPass
    : public PassInfoMixin<
          FindTaintedConditionPass> {

  PreservedAnalyses run(
      Function &F,
      FunctionAnalysisManager &AM) {

    Module *M =
        F.getParent();

    LLVMContext &Ctx =
        M->getContext();

    LoopInfo &LI =
        AM.getResult<LoopAnalysis>(F);

    ScalarEvolution &SE =
        AM.getResult<
            ScalarEvolutionAnalysis>(F);

    // ------------------------------------------------------------
    // Find maximum loop nesting depth.
    // ------------------------------------------------------------

    unsigned MaxDepth = 0;

    for (BasicBlock &BB : F) {

      Loop *L =
          LI.getLoopFor(&BB);

      unsigned Depth = 0;

      while (L) {
        ++Depth;
        L = L->getParentLoop();
      }

      MaxDepth =
          std::max(MaxDepth, Depth);
    }

    // ------------------------------------------------------------
    // Runtime storage for loop values.
    // ------------------------------------------------------------

    AllocaInst *LoopValuesAlloca =
        nullptr;

    ArrayType *LoopArrayTy =
        nullptr;

    if (MaxDepth > 0) {

      LoopArrayTy =
          ArrayType::get(
              Type::getInt64Ty(Ctx),
              MaxDepth);

      IRBuilder<> EntryBuilder(
          &F.getEntryBlock(),
          F.getEntryBlock()
              .getFirstInsertionPt());

      LoopValuesAlloca =
          EntryBuilder.CreateAlloca(
              LoopArrayTy,
              nullptr,
              "__implicit_loop_values");
    }

    // ------------------------------------------------------------
    // Runtime callback:
    //
    // void __implicit_branch_callback(
    //     uint8_t label,
    //     uint32_t line,
    //     uint32_t loop_count,
    //     const char *loop_names,
    //     const int64_t *loop_values,
    //     const char *variables);
    // ------------------------------------------------------------

    Type *VoidTy =
        Type::getVoidTy(Ctx);

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    Type *PtrTy =
        PointerType::get(Ctx, 0);

    FunctionType *CallbackTy =
      FunctionType::get(
          VoidTy,
          {
              I8Ty,   // label
              I32Ty,  // line
              I32Ty,  // column
              I32Ty,  // loop count
              PtrTy,  // loop names
              PtrTy,  // loop values
              PtrTy   // variable
          },
          false);

    FunctionCallee RuntimeCallback =
        M->getOrInsertFunction(
            "__implicit_branch_callback",
            CallbackTy);

    // ------------------------------------------------------------
    // Find all DFSan conditional callbacks.
    // ------------------------------------------------------------

    SmallVector<CallInst *, 32>
        DFSanCallbacks;

    for (BasicBlock &BB : F) {

      for (Instruction &I : BB) {

        auto *CI =
            dyn_cast<CallInst>(&I);

        if (!CI)
          continue;

        if (isDFSanConditionalCallback(CI))
          DFSanCallbacks.push_back(CI);
      }
    }

    // ------------------------------------------------------------
    // Process every DFSan conditional callback.
    // ------------------------------------------------------------

    for (CallInst *DFSanCB :
         DFSanCallbacks) {

      /*
       * DFSan's conditional callback is directly
       * associated with this conditional branch.
       */
      Instruction *Next =
          DFSanCB->getNextNode();

      auto *BI =
          dyn_cast_or_null<BranchInst>(Next);

      if (!BI ||
          !BI->isConditional())
        continue;

      Value *Label =
          DFSanCB->getArgOperand(0);

      if (Label->getType() != I8Ty) {

        IRBuilder<> TmpBuilder(DFSanCB);

        Label =
            TmpBuilder.CreateIntCast(
                Label,
                I8Ty,
                false);
      }

      // ----------------------------------------------------------
      // Source line
      // ----------------------------------------------------------

      unsigned Line = 0;
      unsigned Column = 0;

      DebugLoc DL =
          BI->getDebugLoc();

      if (DL) {
        Line = DL.getLine();
        Column = DL.getCol();
      } else {

        if (auto *CondI =
                dyn_cast<Instruction>(
                    BI->getCondition())) {

          DebugLoc CondDL =
              CondI->getDebugLoc();

          if (CondDL) {
            Line = CondDL.getLine();
            Column = CondDL.getCol();
          }
        }
      }

      // ----------------------------------------------------------
      // Tainted variable(s) in condition.
      // ----------------------------------------------------------

      std::string VariableNames =
        getConditionVariableName(
            F,
            BI->getCondition());

      if (VariableNames.empty())
        VariableNames = "unknown";

      // ----------------------------------------------------------
      // Find enclosing loops:
      //
      // outer -> inner
      // ----------------------------------------------------------

      SmallVector<Loop *, 8>
          EnclosingLoops;

      Loop *CurrentLoop =
          LI.getLoopFor(
              BI->getParent());

      while (CurrentLoop) {

        EnclosingLoops.push_back(
            CurrentLoop);

        CurrentLoop =
            CurrentLoop->getParentLoop();
      }

      std::reverse(
          EnclosingLoops.begin(),
          EnclosingLoops.end());

      // ----------------------------------------------------------
      // Find induction variable of each loop.
      // ----------------------------------------------------------

      SmallVector<PHINode *, 8>
          LoopIVs;

      std::string LoopNames;

      for (unsigned I = 0;
           I < EnclosingLoops.size();
           ++I) {

        PHINode *IV =
            findLoopInductionVariable(
                EnclosingLoops[I],
                SE);

        LoopIVs.push_back(IV);

        std::string Name =
          IV ? findVariableName(F, IV)
            : "";

        if (Name.empty())
          Name = "loop" +
                 std::to_string(I);

        if (I != 0)
          LoopNames += ",";

        LoopNames += Name;
      }

      IRBuilder<> IRB(
          DFSanCB);

      Value *LoopNamesPtr =
          nullptr;

      if (!LoopNames.empty()) {

        LoopNamesPtr =
            IRB.CreateGlobalString(
                LoopNames,
                "__implicit_loop_names");

      } else {

        LoopNamesPtr =
            ConstantPointerNull::get(
                cast<PointerType>(PtrTy));
      }

      Value *VariableNamesPtr =
          IRB.CreateGlobalString(
              VariableNames,
              "__implicit_variable_names");

      Value *LoopValuesPtr =
          ConstantPointerNull::get(
              cast<PointerType>(PtrTy));

      // ----------------------------------------------------------
      // Read current runtime i/j/k values.
      //
      // IMPORTANT:
      // These are SSA PHI values representing the CURRENT
      // iteration at the point where the tainted condition runs.
      // ----------------------------------------------------------

      if (!LoopIVs.empty() &&
          LoopValuesAlloca &&
          LoopArrayTy) {

        for (unsigned I = 0;
             I < LoopIVs.size();
             ++I) {

          PHINode *IV =
              LoopIVs[I];

          Value *CurrentValue;

          if (IV) {

            CurrentValue =
                toI64(
                    IRB,
                    IV);

          } else {

            CurrentValue =
                ConstantInt::get(
                    Type::getInt64Ty(Ctx),
                    -1,
                    true);
          }

          Value *ElementPtr =
              IRB.CreateInBoundsGEP(
                  LoopArrayTy,
                  LoopValuesAlloca,
                  {
                      IRB.getInt32(0),
                      IRB.getInt32(I)
                  });

          IRB.CreateStore(
              CurrentValue,
              ElementPtr);
        }

        LoopValuesPtr =
            IRB.CreateInBoundsGEP(
                LoopArrayTy,
                LoopValuesAlloca,
                {
                    IRB.getInt32(0),
                    IRB.getInt32(0)
                });
      }

      // ----------------------------------------------------------
      // Add our reporting callback.
      //
      // DFSan's original callback is KEPT.
      // ----------------------------------------------------------

      IRB.CreateCall(
        RuntimeCallback,
        {
            Label,
            ConstantInt::get(
                I32Ty,
                Line),
            ConstantInt::get(
                I32Ty,
                Column),
            ConstantInt::get(
                I32Ty,
                LoopIVs.size()),
            LoopNamesPtr,
            LoopValuesPtr,
            VariableNamesPtr
        });
    }

    return PreservedAnalyses::none();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK
PassPluginLibraryInfo
llvmGetPassPluginInfo() {

  return {
      LLVM_PLUGIN_API_VERSION,
      "FindTaintedCondition",
      LLVM_VERSION_STRING,

      [](PassBuilder &PB) {

        PB.registerPipelineParsingCallback(
            [](StringRef Name,
               FunctionPassManager &FPM,
               ArrayRef<
                   PassBuilder::PipelineElement>) {

              if (Name ==
                  "find-tainted-condition") {

                FPM.addPass(
                    FindTaintedConditionPass());

                return true;
              }

              return false;
            });
      }
  };
}