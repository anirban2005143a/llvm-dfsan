#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <algorithm>
#include <cstdint>
#include <memory>

using namespace llvm;

namespace {

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
  enum Kind {
    Unknown,
    Leaf,
    Choice
  } K = Unknown;

  Value *V = nullptr;
  unsigned ConditionID = 0;
  SmallVector<ExprPtr, 4> Arms;
};

static ExprPtr unknownExpr() {
  return std::make_shared<Expr>();
}

static ExprPtr leafExpr(Value *V) {
  auto E = std::make_shared<Expr>();
  E->K = Expr::Leaf;
  E->V = V;
  return E;
}

static ExprPtr choiceExpr(
    unsigned ID,
    ArrayRef<ExprPtr> Arms) {

  auto E = std::make_shared<Expr>();
  E->K = Expr::Choice;
  E->ConditionID = ID;
  E->Arms.append(Arms.begin(), Arms.end());

  return E;
}

static bool sameIRValue(
    Value *A,
    Value *B) {

  if (A == B)
    return true;

  auto *CA =
      dyn_cast_or_null<Constant>(A);

  auto *CB =
      dyn_cast_or_null<Constant>(B);

  if (CA && CB &&
      CA->getType() == CB->getType()) {

    return CA->isElementWiseEqual(CB);
  }

  auto *LA =
      dyn_cast_or_null<LoadInst>(A);

  auto *LB =
      dyn_cast_or_null<LoadInst>(B);

  if (LA && LB) {

    return LA->getType() ==
               LB->getType() &&
           LA->getPointerOperand()
                  ->stripPointerCasts() ==
               LB->getPointerOperand()
                  ->stripPointerCasts();
  }

  return false;
}

static bool sameExpr(
    const ExprPtr &A,
    const ExprPtr &B) {

  if (!A || !B)
    return !A && !B;

  if (A->K != B->K)
    return false;

  if (A->K == Expr::Unknown)
    return true;

  if (A->K == Expr::Leaf)
    return sameIRValue(
        A->V,
        B->V);

  if (A->ConditionID !=
          B->ConditionID ||
      A->Arms.size() !=
          B->Arms.size()) {

    return false;
  }

  for (unsigned I = 0;
       I < A->Arms.size();
       ++I) {

    if (!sameExpr(
            A->Arms[I],
            B->Arms[I])) {

      return false;
    }
  }

  return true;
}

static ExprPtr simplifyChoice(
    unsigned ID,
    ArrayRef<ExprPtr> Arms) {

  if (Arms.empty())
    return unknownExpr();

  for (unsigned I = 1;
       I < Arms.size();
       ++I) {

    if (!sameExpr(
            Arms[0],
            Arms[I])) {

      return choiceExpr(
          ID,
          Arms);
    }
  }

  return Arms[0];
}

static void collectChoiceConditions(
    const ExprPtr &E,
    SmallVectorImpl<unsigned> &Out) {

  if (!E ||
      E->K != Expr::Choice) {

    return;
  }

  if (std::find(
          Out.begin(),
          Out.end(),
          E->ConditionID) ==
      Out.end()) {

    Out.push_back(
        E->ConditionID);
  }

  for (const ExprPtr &Arm :
       E->Arms) {

    collectChoiceConditions(
        Arm,
        Out);
  }
}

struct ConditionInfo {
  Instruction *Term = nullptr;
  BasicBlock *Merge = nullptr;

  /*
   * Runtime slot containing the DFSan label
   * of the condition when this condition
   * actually executes.
   */
  AllocaInst *RuntimeSlot = nullptr;
};

static Type *labelTy(
    LLVMContext &C) {

  return Type::getInt8Ty(C);
}

static FunctionCallee getDFSanGetLabel(
    Module &M) {

  LLVMContext &C =
      M.getContext();

  FunctionType *FT =
      FunctionType::get(
          labelTy(C),
          {
              Type::getInt64Ty(C)
          },
          false);

  return M.getOrInsertFunction(
        "dfsan_get_label",
        FT);
}

static FunctionCallee getDFSanUnion(
    Module &M) {

  LLVMContext &C =
      M.getContext();

  FunctionType *FT =
      FunctionType::get(
          labelTy(C),
          {
              labelTy(C),
              labelTy(C)
          },
          false);

  return M.getOrInsertFunction(
      "dfsan_union",
      FT);
}

static FunctionCallee getDFSanAddLabel(
    Module &M) {

  LLVMContext &C =
      M.getContext();

  FunctionType *FT =
      FunctionType::get(
          Type::getVoidTy(C),
          {
              labelTy(C),
              PointerType::get(C, 0),
              M.getDataLayout()
                  .getIntPtrType(C)
          },
          false);

  return M.getOrInsertFunction(
      "dfsan_add_label",
      FT);
}

static Value *unionLabels(
    Module &M,
    IRBuilder<> &B,
    ArrayRef<Value *> Labels) {

  Value *Result = nullptr;

  FunctionCallee Union =
      getDFSanUnion(M);

  for (Value *V : Labels) {

    if (!V)
      continue;

    if (V->getType() !=
        labelTy(B.getContext())) {

      if (!V->getType()
               ->isIntegerTy()) {

        continue;
      }

      V = B.CreateIntCast(
          V,
          labelTy(B.getContext()),
          false,
          "implicit.label.cast");
    }

    if (!Result) {

      Result = V;

    } else {

      Result =
          B.CreateCall(
              Union,
              {
                  Result,
                  V
              },
              "implicit.label.union");
    }
  }

  return Result;
}

static Value *runtimeConditionLabel(
    Module &M,
    IRBuilder<> &B,
    Instruction *Term) {

  (void)M;

  if (!Term) {
    return ConstantInt::get(
        labelTy(B.getContext()),
        0);
  }

  /*
   * DFSan has already materialized the condition's
   * runtime shadow label when
   * -dfsan-conditional-callbacks is enabled.
   *
   * The callback looks like:
   *
   *   call void @__dfsan_conditional_callback(i8 %label)
   *
   * immediately before the branch/switch.
   *
   * Reuse that label directly instead of calling
   * dfsan_get_label(), because dfsan_get_label() is
   * a special DFSan ABI function and cannot be inserted
   * after DFSan instrumentation.
   */
  for (Instruction *I =
           Term->getPrevNode();
       I;
       I = I->getPrevNode()) {

    if (isa<DbgInfoIntrinsic>(I))
      continue;

    auto *CI =
        dyn_cast<CallInst>(I);

    if (!CI)
      break;

    Function *Callee =
        CI->getCalledFunction();

    if (!Callee)
      break;

    StringRef Name =
        Callee->getName();

    if ((Name ==
             "__dfsan_conditional_callback" ||
         Name ==
             "__dfsan_conditional_callback_origin") &&
        CI->arg_size() >= 1) {

      return CI->getArgOperand(0);
    }

    break;
  }

  return ConstantInt::get(
      labelTy(B.getContext()),
      0);
}

static BasicBlock *
immediatePostDominator(
    PostDominatorTree &PDT,
    Instruction *Term) {

  if (!Term)
    return nullptr;

  auto *Node =
      PDT.getNode(
          Term->getParent());

  if (!Node ||
      !Node->getIDom()) {

    return nullptr;
  }

  return Node->getIDom()
      ->getBlock();
}

static AllocaInst *
baseAlloca(Value *Ptr) {

  if (!Ptr)
    return nullptr;

  Ptr =
      Ptr->stripPointerCasts();

  while (auto *GEP =
             dyn_cast<GEPOperator>(Ptr)) {

    Ptr =
        GEP->getPointerOperand()
            ->stripPointerCasts();
  }

  return dyn_cast<AllocaInst>(Ptr);
}

static bool wholeAlloca(
    Value *Ptr,
    AllocaInst *AI) {

  return AI &&
         baseAlloca(Ptr) == AI &&
         Ptr->stripPointerCasts() == AI;
}

static StoreInst *
closestDominatingStore(
    Function &F,
    AllocaInst *AI,
    Instruction *Before,
    DominatorTree &DT) {

  StoreInst *Best = nullptr;
  unsigned BestDepth = 0;

  for (Instruction &I :
       instructions(F)) {

    auto *S =
        dyn_cast<StoreInst>(&I);

    if (!S)
      continue;

    if (!wholeAlloca(
            S->getPointerOperand(),
            AI)) {

      continue;
    }

    if (!DT.dominates(
            S,
            Before)) {

      continue;
    }

    unsigned Depth = 0;

    if (auto *N =
            DT.getNode(
                S->getParent())) {

      for (; N; N = N->getIDom())
        ++Depth;
    }

    if (!Best ||
        Depth > BestDepth ||
        (Depth == BestDepth &&
         Best->comesBefore(S))) {

      Best = S;
      BestDepth = Depth;
    }
  }

  return Best;
}

static ExprPtr initialExpr(
    Function &F,
    AllocaInst *AI,
    Instruction *Before,
    DominatorTree &DT) {

  if (auto *S =
          closestDominatingStore(
              F,
              AI,
              Before,
              DT)) {

    return leafExpr(
        S->getValueOperand());
  }

  return unknownExpr();
}

static bool writesTarget(
    Instruction &I,
    AllocaInst *AI) {

  if (auto *S =
          dyn_cast<StoreInst>(&I)) {

    return baseAlloca(
               S->getPointerOperand()) ==
           AI;
  }

  if (auto *R =
          dyn_cast<AtomicRMWInst>(&I)) {

    return baseAlloca(
               R->getPointerOperand()) ==
           AI;
  }

  if (auto *C =
          dyn_cast<AtomicCmpXchgInst>(&I)) {

    return baseAlloca(
               C->getPointerOperand()) ==
           AI;
  }

  if (auto *M =
          dyn_cast<MemSetInst>(&I)) {

    return baseAlloca(
               M->getRawDest()) ==
           AI;
  }

  if (auto *M =
          dyn_cast<MemTransferInst>(&I)) {

    return baseAlloca(
               M->getRawDest()) ==
           AI;
  }

  return false;
}

static ExprPtr applyWrite(
    Instruction &I,
    AllocaInst *AI,
    const ExprPtr &Current) {

  if (!writesTarget(I, AI))
    return Current;

  if (auto *S =
          dyn_cast<StoreInst>(&I)) {

    if (wholeAlloca(
            S->getPointerOperand(),
            AI)) {

      return leafExpr(
          S->getValueOperand());
    }
  }

  return unknownExpr();
}

class RegionEvaluator {

  Function &F;
  PostDominatorTree &PDT;

  DenseMap<
      Instruction *,
      unsigned> &ConditionIDs;

  AllocaInst *Target;

  SmallPtrSet<
      BasicBlock *,
      32> Active;

  unsigned Depth = 0;

  ExprPtr eval(
      BasicBlock *Start,
      BasicBlock *Stop,
      ExprPtr State) {

    if (!Start ||
        !Stop) {

      return unknownExpr();
    }

    if (Start == Stop)
      return State;

    if (++Depth > 1024) {

    --Depth;
    return unknownExpr();
    }

    if (!Active.insert(Start)
            .second) {

    --Depth;
    return State;
    }

    ExprPtr Current = State;

    for (Instruction &I :
         *Start) {

      if (&I ==
          Start->getTerminator()) {

        break;
      }

      Current =
          applyWrite(
              I,
              Target,
              Current);
    }

    Instruction *T =
        Start->getTerminator();

    ExprPtr Result =
        unknownExpr();

    if (auto *B =
            dyn_cast<BranchInst>(T)) {

      if (!B->isConditional()) {

        Result =
            eval(
                B->getSuccessor(0),
                Stop,
                Current);

      } else {

        BasicBlock *Merge =
            immediatePostDominator(
                PDT,
                T);

        auto It =
            ConditionIDs.find(T);

        if (!Merge ||
            It == ConditionIDs.end() ||
            !PDT.dominates(
                Stop,
                Merge)) {

          Result =
              unknownExpr();

        } else {

          ExprPtr TrueExpr =
              eval(
                  B->getSuccessor(0),
                  Merge,
                  Current);

          ExprPtr FalseExpr =
              eval(
                  B->getSuccessor(1),
                  Merge,
                  Current);

          SmallVector<
              ExprPtr,
              2> Arms;

          Arms.push_back(
              TrueExpr);

          Arms.push_back(
              FalseExpr);

          ExprPtr Joined =
              simplifyChoice(
                  It->second,
                  Arms);

          Result =
              eval(
                  Merge,
                  Stop,
                  Joined);
        }
      }

    } else if (
        auto *S =
            dyn_cast<SwitchInst>(T)) {

      BasicBlock *Merge =
          immediatePostDominator(
              PDT,
              T);

      auto It =
          ConditionIDs.find(T);

      if (!Merge ||
          It == ConditionIDs.end() ||
          !PDT.dominates(
              Stop,
              Merge)) {

        Result =
            unknownExpr();

      } else {

        SmallVector<
            ExprPtr,
            8> Arms;

        for (unsigned I = 0;
             I < S->getNumSuccessors();
             ++I) {

          Arms.push_back(
              eval(
                  S->getSuccessor(I),
                  Merge,
                  Current));
        }

        ExprPtr Joined =
            simplifyChoice(
                It->second,
                Arms);

        Result =
            eval(
                Merge,
                Stop,
                Joined);
      }

    } else if (
        T->getNumSuccessors() == 1) {

      Result =
          eval(
              T->getSuccessor(0),
              Stop,
              Current);

    } else if (
        isa<ReturnInst>(T) ||
        isa<ResumeInst>(T) ||
        isa<UnreachableInst>(T)) {

      Result = Current;
    }

    Active.erase(Start);
    --Depth;

    return Result;
  }

public:

  RegionEvaluator(
      Function &F,
      PostDominatorTree &PDT,
      DenseMap<
          Instruction *,
          unsigned> &IDs,
      AllocaInst *Target)
      : F(F),
        PDT(PDT),
        ConditionIDs(IDs),
        Target(Target) {}

  ExprPtr run(
      BasicBlock *Start,
      BasicBlock *Stop,
      ExprPtr Initial) {

    Active.clear();
    Depth = 0;

    return eval(
        Start,
        Stop,
        Initial);
  }
};

static void collectRegion(
    BasicBlock *Start,
    BasicBlock *Stop,
    SmallVectorImpl<
        BasicBlock *> &Out) {

  if (!Start ||
      Start == Stop) {

    return;
  }

  SmallVector<
      BasicBlock *,
      64> Work;

  SmallPtrSet<
      BasicBlock *,
      32> Seen;

  Work.push_back(Start);

  while (!Work.empty()) {

    BasicBlock *BB =
        Work.pop_back_val();

    if (!BB ||
        BB == Stop ||
        !Seen.insert(BB)
             .second) {

      continue;
    }

    Out.push_back(BB);

    for (BasicBlock *S :
         successors(BB)) {

      if (S != Stop)
        Work.push_back(S);
    }
  }
}

static void collectTargets(
    ArrayRef<BasicBlock *> Blocks,
    SmallVectorImpl<
        AllocaInst *> &Out) {

  for (BasicBlock *BB :
       Blocks) {

    for (Instruction &I :
         *BB) {

      AllocaInst *AI = nullptr;

      if (auto *S =
              dyn_cast<StoreInst>(&I)) {

        AI =
            baseAlloca(
                S->getPointerOperand());

      } else if (
          auto *R =
              dyn_cast<AtomicRMWInst>(&I)) {

        AI =
            baseAlloca(
                R->getPointerOperand());

      } else if (
          auto *C =
              dyn_cast<AtomicCmpXchgInst>(&I)) {

        AI =
            baseAlloca(
                C->getPointerOperand());

      } else if (
          auto *M =
              dyn_cast<MemSetInst>(&I)) {

        AI =
            baseAlloca(
                M->getRawDest());

      } else if (
          auto *M =
              dyn_cast<MemTransferInst>(&I)) {

        AI =
            baseAlloca(
                M->getRawDest());
      }

      if (AI &&
          std::find(
              Out.begin(),
              Out.end(),
              AI) == Out.end()) {

        Out.push_back(AI);
      }
    }
  }
}

static uint64_t allocaSize(
    AllocaInst *AI,
    const DataLayout &DL) {

  if (!AI)
    return 0;

  TypeSize S =
      DL.getTypeAllocSize(
          AI->getAllocatedType());

  if (S.isScalable())
    return 0;

  auto *N =
      dyn_cast<ConstantInt>(
          AI->getArraySize());

  if (!N)
    return 0;

  return S.getFixedValue() *
         N->getZExtValue();
}

static Value *loadRuntimeLabel(
    Module &M,
    IRBuilder<> &B,
    AllocaInst *Slot) {

  if (!Slot)
    return nullptr;

  return B.CreateLoad(
      labelTy(M.getContext()),
      Slot,
      "implicit.runtime.condition");
}

struct MergeTargetInfo {
  BasicBlock *Merge = nullptr;
  AllocaInst *Target = nullptr;
  SmallVector<unsigned, 16> IDs;
};

static void appendUniqueIDs(
    SmallVectorImpl<unsigned> &Dst,
    ArrayRef<unsigned> Src) {

  for (unsigned ID : Src) {

    if (std::find(
            Dst.begin(),
            Dst.end(),
            ID) == Dst.end()) {

      Dst.push_back(ID);
    }
  }
}

static void recordStoreDependencies(
    ArrayRef<BasicBlock *> Region,
    AllocaInst *Target,
    ArrayRef<unsigned> IDs,
    DenseMap<
        StoreInst *,
        SmallVector<unsigned, 8>> &StoreDependencies) {

  if (!Target || IDs.empty())
    return;

  for (BasicBlock *BB : Region) {

    if (!BB)
      continue;

    for (Instruction &I : *BB) {

      auto *S = dyn_cast<StoreInst>(&I);

      if (!S ||
          !wholeAlloca(
              S->getPointerOperand(),
              Target)) {

        continue;
      }

      appendUniqueIDs(
          StoreDependencies[S],
          IDs);
    }
  }
}

static void addRuntimeLabelsForWrite(
    Module &M,
    StoreInst *S,
    AllocaInst *Target,
    ArrayRef<ConditionInfo> Conditions,
    ArrayRef<unsigned> IDs) {

  if (!S ||
      !Target ||
      IDs.empty() ||
      !S->getNextNode()) {

    return;
  }

  uint64_t Size =
      allocaSize(
          Target,
          M.getDataLayout());

  if (!Size)
    return;

  IRBuilder<> B(
      S->getNextNode());

  SmallVector<
      Value *,
      16> Labels;

  for (unsigned ID : IDs) {

    if (ID >= Conditions.size())
      continue;

    if (Value *L =
            loadRuntimeLabel(
                M,
                B,
                Conditions[ID]
                    .RuntimeSlot)) {

      Labels.push_back(L);
    }
  }

  Value *Combined =
      unionLabels(
          M,
          B,
          Labels);

  if (!Combined)
    return;

  Value *Address =
      B.CreatePointerCast(
          Target,
          PointerType::get(
              M.getContext(),
              0),
          "implicit.target.address");

  Value *SizeValue =
      ConstantInt::get(
          M.getDataLayout()
              .getIntPtrType(
                  M.getContext()),
          Size);

  B.CreateCall(
      getDFSanAddLabel(M),
      {
          Combined,
          Address,
          SizeValue
      });
}


static void addRuntimeLabelsAtMerge(
    Module &M,
    ArrayRef<MergeTargetInfo> MergeTargets,
    ArrayRef<ConditionInfo> Conditions) {

  /*
   * IMPORTANT:
   *
   * Do not use RuntimeSlot here.
   *
   * RuntimeSlot only contains the label of a
   * condition when that condition actually executes.
   *
   * For:
   *
   *   if (secret1) {
   *       x = 10;
   *   } else if (secret2) {
   *       x = 40;
   *   } else {
   *       x = 10;
   *   }
   *
   * x depends on BOTH secret1 and secret2.
   *
   * Therefore we obtain the DFSan labels of the
   * condition's underlying loads directly from
   * shadow memory.
   */

  struct ConditionSource {
    Value *Address = nullptr;
    uint64_t Size = 0;
  };

  FunctionCallee ReadLabel =
      M.getOrInsertFunction(
          "dfsan_read_label",
          FunctionType::get(
              labelTy(M.getContext()),
              {
                  PointerType::get(
                      M.getContext(),
                      0),
                  M.getDataLayout()
                      .getIntPtrType(
                          M.getContext())
              },
              false));

  auto collectConditionSources =
      [&](auto &&Self,
          Value *V,
          SmallVectorImpl<
              ConditionSource> &Sources,
          SmallPtrSetImpl<Value *> &Seen) -> void {

    if (!V)
      return;

    if (!Seen.insert(V).second)
      return;

    /*
     * Stop at loads.
     *
     * DFSan shadow memory for this address
     * contains the actual taint label.
     */
    if (auto *L =
            dyn_cast<LoadInst>(V)) {

      TypeSize TS =
          M.getDataLayout()
              .getTypeStoreSize(
                  L->getType());

      if (TS.isScalable())
        return;

      uint64_t Size =
          TS.getFixedValue();

      if (!Size)
        return;

      Value *Address =
          L->getPointerOperand();

      bool Exists = false;

      for (const ConditionSource &S :
           Sources) {

        if (S.Address == Address &&
            S.Size == Size) {

          Exists = true;
          break;
        }
      }

      if (!Exists) {

        ConditionSource S;
        S.Address = Address;
        S.Size = Size;

        Sources.push_back(S);
      }

      return;
    }

    /*
     * Walk through icmp, casts, arithmetic,
     * select, phi, etc.
     */
    if (auto *U =
            dyn_cast<User>(V)) {

      for (Value *Op :
           U->operands()) {

        Self(
            Self,
            Op,
            Sources,
            Seen);
      }
    }
  };

  for (const MergeTargetInfo &MT :
       MergeTargets) {

    if (!MT.Merge ||
        !MT.Target ||
        MT.IDs.empty()) {

      continue;
    }

    uint64_t Size =
        allocaSize(
            MT.Target,
            M.getDataLayout());

    if (!Size)
      continue;

    Instruction *IP =
        &*MT.Merge
              ->getFirstInsertionPt();

    IRBuilder<> B(IP);

    SmallVector<
        Value *,
        16> Labels;

    for (unsigned ID :
         MT.IDs) {

      if (ID >= Conditions.size())
        continue;

      /*
       * FIX:
       * Conditions is ArrayRef<ConditionInfo>,
       * therefore this MUST be const.
       */
      const ConditionInfo &CI =
          Conditions[ID];

      if (!CI.Term)
        continue;

      Value *Condition = nullptr;

      if (auto *BI =
              dyn_cast<BranchInst>(
                  CI.Term)) {

        if (BI->isConditional())
          Condition =
              BI->getCondition();

      } else if (
          auto *SI =
              dyn_cast<SwitchInst>(
                  CI.Term)) {

        Condition =
            SI->getCondition();
      }

      if (!Condition)
        continue;

      SmallVector<
          ConditionSource,
          8> Sources;

      SmallPtrSet<
          Value *,
          32> Seen;

      collectConditionSources(
          collectConditionSources,
          Condition,
          Sources,
          Seen);

      /*
       * Every underlying source contributes its
       * current DFSan memory label.
       *
       * Thus, even if an else-if condition was not
       * executed on the current path, its secret's
       * label is still available.
       */
      for (const ConditionSource &S :
           Sources) {

        Value *Address =
            B.CreatePointerCast(
                S.Address,
                PointerType::get(
                    M.getContext(),
                    0),
                "implicit.condition.address");

        Value *SourceSize =
            ConstantInt::get(
                M.getDataLayout()
                    .getIntPtrType(
                        M.getContext()),
                S.Size);

        Value *L =
            B.CreateCall(
                ReadLabel,
                {
                    Address,
                    SourceSize
                },
                "implicit.condition.label");

        Labels.push_back(L);
      }

      /*
       * Fallback for conditions that do not reduce
       * to memory loads.
       */
      if (Sources.empty()) {

        if (Value *L =
                loadRuntimeLabel(
                    M,
                    B,
                    CI.RuntimeSlot)) {

          Labels.push_back(L);
        }
      }
    }

    Value *Combined =
        unionLabels(
            M,
            B,
            Labels);

    if (!Combined)
      continue;

    Value *Address =
        B.CreatePointerCast(
            MT.Target,
            PointerType::get(
                M.getContext(),
                0),
            "implicit.target.address");

    Value *SizeValue =
        ConstantInt::get(
            M.getDataLayout()
                .getIntPtrType(
                    M.getContext()),
            Size);

    B.CreateCall(
        getDFSanAddLabel(M),
        {
            Combined,
            Address,
            SizeValue
        });
  }
}

struct ImplicitTaintPropagation
    : PassInfoMixin<
          ImplicitTaintPropagation> {

  PreservedAnalyses run(
      Function &F,
      FunctionAnalysisManager &FAM) {

    if (F.isDeclaration())
      return PreservedAnalyses::all();

    Module *M =
        F.getParent();

    if (!M)
      return PreservedAnalyses::all();

    DominatorTree &DT =
        FAM.getResult<
            DominatorTreeAnalysis>(
            F);

    PostDominatorTree &PDT =
        FAM.getResult<
            PostDominatorTreeAnalysis>(
            F);

    DenseMap<
        Instruction *,
        unsigned> ConditionIDs;

    SmallVector<
        ConditionInfo,
        32> Conditions;

    /*
     * Discover all conditional branches/switches
     * and their immediate post-dominating merge.
     */
    for (BasicBlock &BB :
         F) {

      Instruction *T =
          BB.getTerminator();

      bool IsConditional =
          isa<SwitchInst>(T) ||
          (isa<BranchInst>(T) &&
           cast<BranchInst>(T)
               ->isConditional());

      if (!IsConditional)
        continue;

      BasicBlock *Merge =
          immediatePostDominator(
              PDT,
              T);

      unsigned ID =
          Conditions.size();

      ConditionInfo CI;

      CI.Term = T;
      CI.Merge = Merge;

      ConditionIDs[T] = ID;

      Conditions.push_back(CI);
    }

    if (Conditions.empty())
      return PreservedAnalyses::all();

    /*
     * Runtime label slots.
     *
     * Each condition gets one byte containing
     * the DFSan label of that condition during
     * the current execution.
     */
    IRBuilder<> EntryBuilder(
        &*F.getEntryBlock()
              .getFirstInsertionPt());

    for (unsigned ID = 0;
         ID < Conditions.size();
         ++ID) {

      ConditionInfo &CI =
          Conditions[ID];

      CI.RuntimeSlot =
          EntryBuilder.CreateAlloca(
              labelTy(M->getContext()),
              nullptr,
              "implicit.condition.slot." +
                  Twine(ID));

      EntryBuilder.CreateStore(
          ConstantInt::get(
              labelTy(M->getContext()),
              0),
          CI.RuntimeSlot);
    }
    
    
    /*
 * Static path analysis.
 *
 * We inspect ALL arms even though only one
 * arm executes at runtime.
 *
 * For every target variable:
 *
 *   same value on all paths
 *       -> no implicit taint
 *
 *   different values
 *       -> collect the conditions responsible
 *
 * The condition set is used in two places:
 *
 *   1. At the merge, so a variable that keeps
 *      its old value on the current path can
 *      still become implicitly tainted when an
 *      alternate path would have changed it.
 *
 *   2. At writes inside the controlled region,
 *      so loop iterations and nested writes do
 *      not lose a tainted condition merely because
 *      the condition's later evaluation becomes
 *      untainted.
 */
DenseMap<
    StoreInst *,
    SmallVector<unsigned, 8>> StoreDependencies;

SmallVector<
    MergeTargetInfo,
    64> MergeTargets;

for (unsigned ID = 0;
     ID < Conditions.size();
     ++ID) {

  ConditionInfo &CI =
      Conditions[ID];

  if (!CI.Merge)
    continue;

  SmallVector<
      BasicBlock *,
      128> Region;

  if (auto *B =
          dyn_cast<BranchInst>(
              CI.Term)) {

    collectRegion(
        B->getSuccessor(0),
        CI.Merge,
        Region);

    collectRegion(
        B->getSuccessor(1),
        CI.Merge,
        Region);

  } else if (
      auto *S =
          dyn_cast<SwitchInst>(
              CI.Term)) {

    for (unsigned I = 0;
         I < S->getNumSuccessors();
         ++I) {

      collectRegion(
          S->getSuccessor(I),
          CI.Merge,
          Region);
    }
  }

  SmallVector<
      AllocaInst *,
      32> Targets;

  collectTargets(
      Region,
      Targets);

  for (AllocaInst *Target :
       Targets) {

    if (!Target)
      continue;

    if (Target->getParent() !=
        &F.getEntryBlock()) {

      continue;
    }

    ExprPtr Initial =
        initialExpr(
            F,
            Target,
            CI.Term,
            DT);

    RegionEvaluator Evaluator(
        F,
        PDT,
        ConditionIDs,
        Target);

    SmallVector<
        ExprPtr,
        8> Arms;

    if (auto *B =
            dyn_cast<BranchInst>(
                CI.Term)) {

      Arms.push_back(
          Evaluator.run(
              B->getSuccessor(0),
              CI.Merge,
              Initial));

      Arms.push_back(
          Evaluator.run(
              B->getSuccessor(1),
              CI.Merge,
              Initial));

    } else if (
        auto *S =
            dyn_cast<SwitchInst>(
                CI.Term)) {

      for (unsigned I = 0;
           I < S->getNumSuccessors();
           ++I) {

        Arms.push_back(
            Evaluator.run(
                S->getSuccessor(I),
                CI.Merge,
                Initial));
      }
    }

    ExprPtr Final =
        simplifyChoice(
            ID,
            Arms);

    SmallVector<
        unsigned,
        16> RelevantConditions;

    collectChoiceConditions(
        Final,
        RelevantConditions);

    if (RelevantConditions.empty())
      continue;

    MergeTargetInfo *MT = nullptr;

    for (MergeTargetInfo &Existing :
         MergeTargets) {

      if (Existing.Merge == CI.Merge &&
          Existing.Target == Target) {

        MT = &Existing;
        break;
      }
    }

    if (!MT) {

      MergeTargetInfo NewMT;
      NewMT.Merge = CI.Merge;
      NewMT.Target = Target;

      NewMT.IDs.append(
          RelevantConditions.begin(),
          RelevantConditions.end());

      MergeTargets.push_back(
          NewMT);

    } else {

      appendUniqueIDs(
          MT->IDs,
          RelevantConditions);
    }

    recordStoreDependencies(
        Region,
        Target,
        RelevantConditions,
        StoreDependencies);
  }
}

/*
 * Apply implicit labels at actual writes.
 *
 * This is important for loops:
 *
 *   while (secret) {
 *       x = 1;
 *       ...
 *   }
 *
 * A previous iteration may have been tainted even
 * if the condition becomes untainted before loop exit.
 */
for (const auto &KV : StoreDependencies) {

  StoreInst *S = KV.first;

  if (!S ||
      KV.second.empty()) {

    continue;
  }

  AllocaInst *Target =
      baseAlloca(
          S->getPointerOperand());

  if (!Target ||
      Target->getParent() !=
          &F.getEntryBlock()) {

    continue;
  }

  addRuntimeLabelsForWrite(
      *M,
      S,
      Target,
      Conditions,
      KV.second);
}

/*
 * Capture runtime DFSan labels only after all
 * static analysis has completed.
 *
 * This prevents these bookkeeping stores from
 * being discovered as application-variable writes
 * when a loop back-edge revisits a condition block.
 */
for (const ConditionInfo &CI :
     Conditions) {

  IRBuilder<> B(CI.Term);

  Value *Label =
      runtimeConditionLabel(
          *M,
          B,
          CI.Term);

  B.CreateStore(
      Label,
      CI.RuntimeSlot);
}

/*
 * At every post-dominator:
 *
 *   1. Apply the labels required by the
 *      all-path/static analysis.
 *
 *   2. Clear every runtime condition slot whose
 *      control context has ended.
 *
 * The clearing is necessary for:
 *
 *   - loops
 *   - nested conditions
 *   - else-if chains
 *   - later paths that skip a previous condition
 */
addRuntimeLabelsAtMerge(
    *M,
    MergeTargets,
    Conditions);
    
    return PreservedAnalyses::none();
  }
};

} // namespace

extern "C"
LLVM_ATTRIBUTE_WEAK
PassPluginLibraryInfo
llvmGetPassPluginInfo() {

  return {
      LLVM_PLUGIN_API_VERSION,
      "ImplicitTaintPropagation",
      LLVM_VERSION_STRING,

      [](PassBuilder &PB) {

        PB.registerPipelineParsingCallback(
            [](StringRef Name,
               ModulePassManager &MPM,
               ArrayRef<
                   PassBuilder::PipelineElement>) {

              if (Name ==
                  "implicit-taint") {

                MPM.addPass(
                    createModuleToFunctionPassAdaptor(
                        ImplicitTaintPropagation()));

                return true;
              }

              return false;
            });

        PB.registerPipelineParsingCallback(
            [](StringRef Name,
               FunctionPassManager &FPM,
               ArrayRef<
                   PassBuilder::PipelineElement>) {

              if (Name ==
                  "implicit-taint") {

                FPM.addPass(
                    ImplicitTaintPropagation());

                return true;
              }

              return false;
            });
      }
  };
}