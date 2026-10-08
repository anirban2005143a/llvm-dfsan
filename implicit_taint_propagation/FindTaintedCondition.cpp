#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugProgramInstruction.h"

using namespace llvm;

namespace {

static uint32_t NextBranchId = 1;

static BasicBlock *getPostDomMerge(PostDominatorTree &PDT, BasicBlock *BB) {
  auto *Node = PDT.getNode(BB);
  if (!Node || !Node->getIDom())
    return nullptr;
  return Node->getIDom()->getBlock();
}

static void addUnique(SmallVectorImpl<AllocaInst *> &Out, AllocaInst *AI) {
  if (!AI)
    return;

  for (AllocaInst *Existing : Out) {
    if (Existing == AI)
      return;
  }

  Out.push_back(AI);
}

static StringRef getVariableName(Function &F, AllocaInst *AI) {
  if (!AI)
    return StringRef();

  if (!AI->getName().empty())
    return AI->getName();

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      for (DbgRecord &DR : I.getDbgRecordRange()) {
        if (auto *DVR = dyn_cast<DbgVariableRecord>(&DR)) {
          if (!DVR->isDbgDeclare())
            continue;

          if (DVR->getAddress() != AI)
            continue;

          if (DILocalVariable *Var = DVR->getVariable())
            return Var->getName();
        }
      }
    }
  }

  return StringRef();
}

static void collectRegionBlocks(BasicBlock *Start, BasicBlock *Stop,
                                SmallVectorImpl<BasicBlock *> &Blocks) {
  if (!Start || !Stop || Start == Stop)
    return;

  SmallVector<BasicBlock *, 32> Worklist;
  SmallPtrSet<BasicBlock *, 32> Seen;
  Worklist.push_back(Start);

  while (!Worklist.empty()) {
    BasicBlock *BB = Worklist.pop_back_val();
    if (!BB || BB == Stop || !Seen.insert(BB).second)
      continue;

    Blocks.push_back(BB);

    for (BasicBlock *Succ : successors(BB)) {
      if (Succ != Stop)
        Worklist.push_back(Succ);
    }
  }
}

static void collectWrittenI32Allocas(
    ArrayRef<BasicBlock *> Blocks,
    SmallVectorImpl<AllocaInst *> &Targets) {
  for (BasicBlock *BB : Blocks) {
    for (Instruction &I : *BB) {
      auto *SI = dyn_cast<StoreInst>(&I);
      if (!SI)
        continue;

      Value *Ptr = SI->getPointerOperand()->stripPointerCasts();
      auto *AI = dyn_cast<AllocaInst>(Ptr);
      if (!AI)
        continue;

      if (!AI->getAllocatedType()->isIntegerTy(32))
        continue;

      addUnique(Targets, AI);
    }
  }
}

static FunctionCallee getClearConditionFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  FunctionType *FT = FunctionType::get(VoidTy, {}, false);
  return M.getOrInsertFunction("__implicit_clear_condition", FT);
}

static FunctionCallee getRecordBranchFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  Type *I8Ty = Type::getInt8Ty(Ctx);
  FunctionType *FT = FunctionType::get(VoidTy, {I32Ty, I8Ty}, false);
  return M.getOrInsertFunction("__implicit_record_branch", FT);
}

static FunctionCallee getObserveFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  Type *I32PtrTy = PointerType::get(Ctx, 0);

  FunctionType *FT = FunctionType::get(
      VoidTy, {I8PtrTy, I32PtrTy}, false);
  return M.getOrInsertFunction("__implicit_observe_i32", FT);
}

static bool isDFSanConditionalCallback(const CallBase *CB) {
  if (!CB)
    return false;

  const Function *Callee = CB->getCalledFunction();
  if (!Callee)
    return false;

  StringRef Name = Callee->getName();
  return Name == "__dfsan_conditional_callback" ||
         Name == "__dfsan_conditional_callback_origin";
}

static CallBase *findConditionalCallback(BasicBlock &BB) {
  for (Instruction &I : BB) {
    if (auto *CB = dyn_cast<CallBase>(&I)) {
      if (isDFSanConditionalCallback(CB))
        return CB;
    }
  }

  return nullptr;
}

static void instrumentConditionalBranch(Module &M, BranchInst &BI,
                                        uint32_t BranchId) {
  BasicBlock *BB = BI.getParent();
  CallBase *DFSanCallback = findConditionalCallback(*BB);

  FunctionCallee Clear = getClearConditionFunction(M);
  FunctionCallee Record = getRecordBranchFunction(M);

  if (DFSanCallback) {
    IRBuilder<> BeforeCallback(DFSanCallback);
    BeforeCallback.CreateCall(Clear);
  } else {
    IRBuilder<> BeforeBranch(&BI);
    BeforeBranch.CreateCall(Clear);
  }

  IRBuilder<> BeforeBranch(&BI);
  LLVMContext &Ctx = M.getContext();
  Value *BranchIdValue = ConstantInt::get(Type::getInt32Ty(Ctx), BranchId);
  Value *Taken = BeforeBranch.CreateZExt(
      BI.getCondition(), Type::getInt8Ty(Ctx));

  BeforeBranch.CreateCall(Record, {BranchIdValue, Taken});
}

static void instrumentMerge(Module &M, Function &F, BasicBlock *Merge,
                            ArrayRef<AllocaInst *> Targets) {
  if (!Merge || Targets.empty())
    return;

  Instruction *IP = &*Merge->getFirstInsertionPt();
  IRBuilder<> B(IP);
  FunctionCallee Observe = getObserveFunction(M);

  SmallPtrSet<AllocaInst *, 16> Done;

  for (AllocaInst *AI : Targets) {
    if (!AI || !Done.insert(AI).second)
      continue;

    StringRef Name = getVariableName(F, AI);
    if (Name.empty())
      continue;

    Value *NamePtr = B.CreateGlobalString(Name, "implicit_var_name");
    Value *Addr = AI;
    B.CreateCall(Observe, {NamePtr, Addr});
  }
}

struct ImplicitTaintPropagation
    : public PassInfoMixin<ImplicitTaintPropagation> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    if (F.isDeclaration())
      return PreservedAnalyses::all();

    Module *M = F.getParent();
    if (!M)
      return PreservedAnalyses::all();

    PostDominatorTree &PDT =
        FAM.getResult<PostDominatorTreeAnalysis>(F);

    bool Changed = false;

    for (BasicBlock &BB : F) {
      auto *BI = dyn_cast<BranchInst>(BB.getTerminator());
      if (!BI || !BI->isConditional())
        continue;

      uint32_t BranchId = NextBranchId++;
      instrumentConditionalBranch(*M, *BI, BranchId);
      Changed = true;

      BasicBlock *Merge = getPostDomMerge(PDT, &BB);
      if (!Merge || Merge == &BB)
        continue;

      SmallVector<BasicBlock *, 32> Region;
      for (BasicBlock *Pred : predecessors(Merge))
        collectRegionBlocks(Pred, Merge, Region);

      SmallVector<AllocaInst *, 32> Targets;
      collectWrittenI32Allocas(Region, Targets);

      if (!Targets.empty()) {
        instrumentMerge(*M, F, Merge, Targets);
        Changed = true;
      }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ImplicitTaintPropagation",
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "implicit-taint")
                    return false;

                  MPM.addPass(createModuleToFunctionPassAdaptor(
                      ImplicitTaintPropagation()));
                  return true;
                });
          }};
}
