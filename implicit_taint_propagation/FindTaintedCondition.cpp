#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;

namespace {

struct VariableInfo {
  std::string Name;
  unsigned Line = 0;
  unsigned Col = 0;
};

struct ConditionalInfo {
  BranchInst *Branch = nullptr;
  CallInst *DFSanCallback = nullptr;

  BasicBlock *Join = nullptr;

  uint32_t JoinID = 0;
  uint32_t BranchID = 0;
};

struct JoinGroup {
  BasicBlock *Join = nullptr;
  uint32_t JoinID = 0;

  std::vector<ConditionalInfo> Conditions;

  std::set<AllocaInst *> Variables;
  std::map<AllocaInst *, VariableInfo> VariableInfoMap;
};

static CallInst *findDFSanConditionalCallback(BranchInst *BR) {
  Instruction *Cur = BR->getPrevNode();

  for (unsigned I = 0; Cur != nullptr && I < 32; ++I) {
    auto *CI = dyn_cast<CallInst>(Cur);

    if (CI) {
      Function *Callee = CI->getCalledFunction();

      if (Callee) {
        StringRef Name = Callee->getName();

        if (Name == "__dfsan_conditional_callback") {
          return CI;
        }
      }
    }

    Cur = Cur->getPrevNode();
  }

  return nullptr;
}

static BasicBlock *findCommonPostDominator(
    BranchInst *BR,
    PostDominatorTree &PDT) {

  BasicBlock *Succ0 = BR->getSuccessor(0);
  BasicBlock *Succ1 = BR->getSuccessor(1);

  DomTreeNodeBase<BasicBlock> *Node0 = PDT.getNode(Succ0);
  DomTreeNodeBase<BasicBlock> *Node1 = PDT.getNode(Succ1);

  if (!Node0 || !Node1)
    return nullptr;

  std::set<DomTreeNodeBase<BasicBlock> *> Ancestors;

  for (DomTreeNodeBase<BasicBlock> *N = Node0;
       N != nullptr;
       N = N->getIDom()) {
    Ancestors.insert(N);
  }

  for (DomTreeNodeBase<BasicBlock> *N = Node1;
       N != nullptr;
       N = N->getIDom()) {

    if (Ancestors.count(N))
      return N->getBlock();
  }

  return nullptr;
}

static void collectVariablesInRegion(
    BasicBlock *Start,
    BasicBlock *Join,
    std::set<AllocaInst *> &Variables,
    std::map<AllocaInst *, VariableInfo> &VariableInfoMap) {

  if (!Start || !Join || Start == Join)
    return;

  std::vector<BasicBlock *> Worklist;
  std::set<BasicBlock *> Visited;

  Worklist.push_back(Start);
  Visited.insert(Start);

  while (!Worklist.empty()) {
    BasicBlock *BB = Worklist.back();
    Worklist.pop_back();

    if (BB == Join)
      continue;

    for (Instruction &I : *BB) {
      auto *SI = dyn_cast<StoreInst>(&I);

      if (!SI)
        continue;

      Value *Ptr = SI->getPointerOperand()->stripPointerCasts();
      auto *AI = dyn_cast<AllocaInst>(Ptr);

      if (!AI)
        continue;

      Type *AllocatedType = AI->getAllocatedType();

      if (!AllocatedType->isIntegerTy())
        continue;

      unsigned Bits = AllocatedType->getIntegerBitWidth();

      if (Bits > 64)
        continue;

      Variables.insert(AI);

      VariableInfo &VI = VariableInfoMap[AI];

      if (VI.Name.empty()) {
        VI.Name = AI->getName().str();
      }

      if (VI.Line == 0) {
        DebugLoc DL = SI->getDebugLoc();

        if (DL) {
          VI.Line = DL.getLine();
          VI.Col = DL.getCol();
        }
      }
    }

    for (BasicBlock *Succ : successors(BB)) {
      if (Succ == Join)
        continue;

      if (Visited.insert(Succ).second)
        Worklist.push_back(Succ);
    }
  }
}

static BasicBlock *splitConditionalEdge(
    BranchInst *BR,
    unsigned SuccessorIndex) {

  BasicBlock *Succ = BR->getSuccessor(SuccessorIndex);

  if (pred_size(Succ) == 1)
    return Succ;

  return SplitEdge(BR->getParent(), Succ);
}

class ImplicitTaintPass
    : public PassInfoMixin<ImplicitTaintPass> {

public:
  PreservedAnalyses run(
      Module &M,
      ModuleAnalysisManager &) {

    LLVMContext &Ctx = M.getContext();

    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I8Ty = Type::getInt8Ty(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);
    Type *I64Ty = Type::getInt64Ty(Ctx);
    PointerType *PtrTy = PointerType::get(Ctx, 0);

    FunctionType *BranchBeginTy =
        FunctionType::get(
            VoidTy,
            {I32Ty, I32Ty, I8Ty},
            false);

    FunctionType *BranchEdgeTy =
        FunctionType::get(
            VoidTy,
            {I32Ty, I32Ty, I8Ty},
            false);

    FunctionType *JoinValueTy =
        FunctionType::get(
            VoidTy,
            {I32Ty, I32Ty, I32Ty, I32Ty,
             PtrTy, I64Ty, I32Ty, PtrTy},
            false);

    FunctionType *JoinEndTy =
        FunctionType::get(
            VoidTy,
            {I32Ty},
            false);

    FunctionCallee BranchBeginFn =
        M.getOrInsertFunction(
            "__implicit_branch_begin",
            BranchBeginTy);

    FunctionCallee BranchEdgeFn =
        M.getOrInsertFunction(
            "__implicit_branch_edge",
            BranchEdgeTy);

    FunctionCallee JoinValueFn =
        M.getOrInsertFunction(
            "__implicit_join_value",
            JoinValueTy);

    FunctionCallee JoinEndFn =
        M.getOrInsertFunction(
            "__implicit_join_end",
            JoinEndTy);

    uint32_t NextJoinID = 1;
    uint32_t NextVariableID = 1;

    std::map<AllocaInst *, uint32_t> VariableIDs;
    std::vector<JoinGroup> Groups;

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      PostDominatorTree PDT;
      PDT.recalculate(F);

      std::map<BasicBlock *, size_t> JoinToGroup;
      std::vector<BranchInst *> CandidateBranches;

      for (BasicBlock &BB : F) {
        auto *BR = dyn_cast<BranchInst>(BB.getTerminator());

        if (!BR || !BR->isConditional())
          continue;

        if (!findDFSanConditionalCallback(BR))
          continue;

        CandidateBranches.push_back(BR);
      }

      for (BranchInst *BR : CandidateBranches) {
        CallInst *DFSanCallback =
            findDFSanConditionalCallback(BR);

        if (!DFSanCallback)
          continue;

        BasicBlock *Join =
            findCommonPostDominator(BR, PDT);

        if (!Join)
          continue;

        size_t GroupIndex;

        auto It = JoinToGroup.find(Join);

        if (It == JoinToGroup.end()) {
          GroupIndex = Groups.size();
          JoinToGroup.emplace(Join, GroupIndex);

          JoinGroup Group;
          Group.Join = Join;
          Group.JoinID = NextJoinID++;

          Groups.push_back(std::move(Group));
        } else {
          GroupIndex = It->second;
        }

        JoinGroup &Group = Groups[GroupIndex];

        ConditionalInfo CI;
        CI.Branch = BR;
        CI.DFSanCallback = DFSanCallback;
        CI.Join = Join;
        CI.JoinID = Group.JoinID;
        CI.BranchID =
            static_cast<uint32_t>(
                Group.Conditions.size() + 1);

        Group.Conditions.push_back(CI);

        collectVariablesInRegion(
            BR->getSuccessor(0),
            Join,
            Group.Variables,
            Group.VariableInfoMap);

        collectVariablesInRegion(
            BR->getSuccessor(1),
            Join,
            Group.Variables,
            Group.VariableInfoMap);
      }
    }

    for (JoinGroup &Group : Groups) {
      for (AllocaInst *AI : Group.Variables) {
        if (!VariableIDs.count(AI))
          VariableIDs.emplace(AI, NextVariableID++);
      }
    }

    bool Changed = false;

    for (JoinGroup &Group : Groups) {
      for (ConditionalInfo &CI : Group.Conditions) {
        BranchInst *BR = CI.Branch;
        CallInst *DFSanCallback = CI.DFSanCallback;

        Value *ConditionLabel =
            DFSanCallback->getArgOperand(0);

        IRBuilder<> BranchBuilder(BR);

        if (ConditionLabel->getType() != I8Ty) {
          if (ConditionLabel->getType()->isIntegerTy()) {
            ConditionLabel =
                BranchBuilder.CreateZExtOrTrunc(
                    ConditionLabel,
                    I8Ty);
          } else {
            continue;
          }
        }

        BranchBuilder.CreateCall(
            BranchBeginFn,
            {
                ConstantInt::get(I32Ty, CI.JoinID),
                ConstantInt::get(I32Ty, CI.BranchID),
                ConditionLabel
            });

        for (unsigned Edge = 0; Edge < 2; ++Edge) {
          BasicBlock *EdgeBB =
              splitConditionalEdge(BR, Edge);

          if (!EdgeBB)
            continue;

          Instruction *InsertPoint =
              &*EdgeBB->getFirstInsertionPt();

          IRBuilder<> EdgeBuilder(InsertPoint);

          EdgeBuilder.CreateCall(
              BranchEdgeFn,
              {
                  ConstantInt::get(I32Ty, CI.JoinID),
                  ConstantInt::get(I32Ty, CI.BranchID),
                  ConstantInt::get(
                      I8Ty,
                      Edge == 0 ? 1 : 0)
              });
        }

        Changed = true;
      }

      if (!Group.Join)
        continue;

      Instruction *JoinInsertPoint =
          &*Group.Join->getFirstInsertionPt();

      IRBuilder<> JoinBuilder(JoinInsertPoint);

      for (AllocaInst *AI : Group.Variables) {
        Type *Ty = AI->getAllocatedType();

        if (!Ty->isIntegerTy())
          continue;

        unsigned Bits = Ty->getIntegerBitWidth();

        if (Bits > 64)
          continue;

        VariableInfo VI =
            Group.VariableInfoMap[AI];

        std::string VariableName = VI.Name;

        if (VariableName.empty())
          VariableName = AI->getName().str();

        if (VariableName.empty())
          VariableName = "variable";

        if (VI.Line == 0) {
          for (User *U : AI->users()) {
            auto *SI = dyn_cast<StoreInst>(U);

            if (!SI)
              continue;

            DebugLoc DL = SI->getDebugLoc();

            if (DL) {
              VI.Line = DL.getLine();
              VI.Col = DL.getCol();
              break;
            }
          }
        }

        LoadInst *LoadedValue =
            JoinBuilder.CreateLoad(
                Ty,
                AI,
                VariableName + ".implicit.value");

        Value *Value64 =
            JoinBuilder.CreateZExtOrTrunc(
                LoadedValue,
                I64Ty);

        Value *NamePtr =
            JoinBuilder.CreateGlobalString(
                VariableName);

        unsigned Size = (Bits + 7) / 8;

        JoinBuilder.CreateCall(
            JoinValueFn,
            {
                ConstantInt::get(
                    I32Ty,
                    Group.JoinID),

                ConstantInt::get(
                    I32Ty,
                    VariableIDs[AI]),

                ConstantInt::get(
                    I32Ty,
                    VI.Line),

                ConstantInt::get(
                    I32Ty,
                    VI.Col),

                NamePtr,

                Value64,

                ConstantInt::get(
                    I32Ty,
                    Size),

                AI
            });
      }

      JoinBuilder.CreateCall(
          JoinEndFn,
          {
              ConstantInt::get(
                  I32Ty,
                  Group.JoinID)
          });

      Changed = true;
    }

    return Changed
               ? PreservedAnalyses::none()
               : PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK
PassPluginLibraryInfo llvmGetPassPluginInfo() {

  return {
      LLVM_PLUGIN_API_VERSION,
      "ImplicitTaint",
      LLVM_VERSION_STRING,

      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name,
               ModulePassManager &MPM,
               ArrayRef<PassBuilder::PipelineElement>) {

              if (Name == "implicit-taint") {
                MPM.addPass(ImplicitTaintPass());
                return true;
              }

              return false;
            });
      }};
}
