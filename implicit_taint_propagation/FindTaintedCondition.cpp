// File: FindTaintedCondition.cpp

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <string>
#include <vector>

using namespace llvm;

namespace {

struct VariableInfo {
    DenseMap<AllocaInst *, std::string> Names;
};

struct ConditionInfo {
    BranchInst *Branch = nullptr;
    CallInst *Callback = nullptr;

    // DFSan label generated for the condition at the actual branch.
    Value *DynamicLabel = nullptr;

    unsigned Line = 0;
    unsigned Column = 0;
};

struct DefinitionState {
    bool Initialized = false;
    bool Unknown = false;

    SmallVector<StoreInst *, 8> Stores;
};

struct LabelSource {
    Value *Pointer = nullptr;
    Value *SSAValue = nullptr;
};

static Value *getBasePointer(Value *V) {
    if (!V)
        return nullptr;

    V = V->stripPointerCasts();

    while (auto *GEP = dyn_cast<GEPOperator>(V)) {
        V = GEP->getPointerOperand();

        if (!V)
            return nullptr;

        V = V->stripPointerCasts();
    }

    return V;
}

static void buildVariableInfo(
    Function &F,
    VariableInfo &Info) {

    for (Instruction &I : instructions(F)) {

        if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var =
                DDI->getVariable();

            Value *Addr =
                DDI->getAddress();

            if (!Var || !Addr)
                continue;

            auto *AI =
                dyn_cast_or_null<AllocaInst>(
                    getBasePointer(Addr));

            if (AI) {
                Info.Names[AI] =
                    Var->getName().str();
            }
        }

        if (!I.hasDbgRecords())
            continue;

        for (DbgRecord &DR :
             I.getDbgRecordRange()) {

            auto *DVR =
                dyn_cast<DbgVariableRecord>(&DR);

            if (!DVR)
                continue;

            DILocalVariable *Var =
                DVR->getVariable();

            if (!Var)
                continue;

            for (unsigned Op = 0;
                 Op < DVR->getNumVariableLocationOps();
                 ++Op) {

                Value *V =
                    DVR->getVariableLocationOp(Op);

                if (!V)
                    continue;

                auto *AI =
                    dyn_cast_or_null<AllocaInst>(
                        getBasePointer(V));

                if (AI) {
                    Info.Names[AI] =
                        Var->getName().str();
                }
            }
        }
    }
}

static std::string getVariableName(
    AllocaInst *AI,
    const VariableInfo &Info) {

    auto It =
        Info.Names.find(AI);

    if (It != Info.Names.end())
        return It->second;

    if (AI && AI->hasName())
        return AI->getName().str();

    return "unknown";
}

static CallInst *findDFSanConditionalCallback(
    BranchInst *BR) {

    if (!BR)
        return nullptr;

    Instruction *Cur =
        BR->getPrevNode();

    for (unsigned I = 0;
         Cur && I < 32;
         ++I) {

        auto *CI =
            dyn_cast<CallInst>(Cur);

        if (CI) {

            Function *Callee =
                CI->getCalledFunction();

            if (Callee) {

                StringRef Name =
                    Callee->getName();

                if (Name ==
                        "__dfsan_conditional_callback" ||
                    Name ==
                        "__dfsan_conditional_callback_origin") {

                    return CI;
                }
            }
        }

        Cur =
            Cur->getPrevNode();
    }

    return nullptr;
}

static BasicBlock *getMergeBlock(
    BranchInst *BR,
    PostDominatorTree &PDT) {

    if (!BR)
        return nullptr;

    auto *Node =
        PDT.getNode(BR->getParent());

    if (!Node)
        return nullptr;

    auto *IDom =
        Node->getIDom();

    if (!IDom)
        return nullptr;

    return IDom->getBlock();
}

static void collectRegion(
    BasicBlock *Start,
    BasicBlock *Stop,
    SmallVectorImpl<BasicBlock *> &Result) {

    if (!Start || Start == Stop)
        return;

    SmallVector<BasicBlock *, 32>
        Worklist;

    SmallPtrSet<BasicBlock *, 32>
        Visited;

    Worklist.push_back(Start);

    while (!Worklist.empty()) {

        BasicBlock *BB =
            Worklist.pop_back_val();

        if (!BB)
            continue;

        if (BB == Stop)
            continue;

        if (!Visited.insert(BB).second)
            continue;

        Result.push_back(BB);

        for (BasicBlock *Succ :
             successors(BB)) {

            if (Succ == Stop)
                continue;

            Worklist.push_back(Succ);
        }
    }
}

static void addUniqueAlloca(
    SmallVectorImpl<AllocaInst *> &List,
    AllocaInst *AI) {

    if (!AI)
        return;

    for (AllocaInst *Existing :
         List) {

        if (Existing == AI)
            return;
    }

    List.push_back(AI);
}

static void addUniqueStore(
    SmallVectorImpl<StoreInst *> &List,
    StoreInst *Store) {

    if (!Store)
        return;

    for (StoreInst *Existing :
         List) {

        if (Existing == Store)
            return;
    }

    List.push_back(Store);
}

static AllocaInst *getStoredAlloca(
    StoreInst *Store) {

    if (!Store)
        return nullptr;

    return dyn_cast_or_null<AllocaInst>(
        getBasePointer(
            Store->getPointerOperand()));
}

static void collectTargets(
    ArrayRef<BasicBlock *> Region,
    SmallVectorImpl<AllocaInst *> &Targets) {

    for (BasicBlock *BB :
         Region) {

        for (Instruction &I :
             *BB) {

            auto *Store =
                dyn_cast<StoreInst>(&I);

            if (!Store)
                continue;

            addUniqueAlloca(
                Targets,
                getStoredAlloca(Store));
        }
    }
}

static bool sameStoredValue(
    StoreInst *A,
    StoreInst *B) {

    if (!A || !B)
        return false;

    Value *VA =
        A->getValueOperand();

    Value *VB =
        B->getValueOperand();

    if (VA == VB)
        return true;

    auto *CA =
        dyn_cast<Constant>(VA);

    auto *CB =
        dyn_cast<Constant>(VB);

    if (!CA || !CB)
        return false;

    if (CA->getType() !=
        CB->getType()) {

        return false;
    }

    return CA->isElementWiseEqual(CB);
}

static bool sameDefinitionState(
    const DefinitionState &A,
    const DefinitionState &B) {

    if (A.Initialized !=
        B.Initialized) {

        return false;
    }

    if (!A.Initialized)
        return true;

    if (A.Unknown !=
        B.Unknown) {

        return false;
    }

    if (A.Stores.size() !=
        B.Stores.size()) {

        return false;
    }

    for (StoreInst *SA :
         A.Stores) {

        bool Found = false;

        for (StoreInst *SB :
             B.Stores) {

            if (sameStoredValue(
                    SA,
                    SB)) {

                Found = true;
                break;
            }
        }

        if (!Found)
            return false;
    }

    return true;
}

static void mergeDefinitionState(
    DefinitionState &Dst,
    const DefinitionState &Src) {

    if (!Src.Initialized)
        return;

    if (!Dst.Initialized) {
        Dst = Src;
        return;
    }

    Dst.Unknown |=
        Src.Unknown;

    for (StoreInst *Store :
         Src.Stores) {

        addUniqueStore(
            Dst.Stores,
            Store);
    }
}

static bool collectFinalDefinitions(
    ArrayRef<BasicBlock *> Region,
    BasicBlock *Start,
    BasicBlock *Stop,
    AllocaInst *Target,
    DefinitionState &ExitState) {

    ExitState =
        DefinitionState();

    if (!Start ||
        !Stop ||
        !Target ||
        Region.empty()) {

        return false;
    }

    SmallPtrSet<BasicBlock *, 32>
        RegionSet;

    for (BasicBlock *BB :
         Region) {

        RegionSet.insert(BB);
    }

    if (!RegionSet.contains(Start))
        return false;

    DenseMap<
        BasicBlock *,
        DefinitionState>
        InStates;

    SmallVector<BasicBlock *, 64>
        Worklist;

    SmallPtrSet<BasicBlock *, 32>
        InWorklist;

    DefinitionState Initial;

    Initial.Initialized = true;
    Initial.Unknown = true;

    InStates[Start] =
        Initial;

    Worklist.push_back(Start);
    InWorklist.insert(Start);

    bool HasExit = false;

    while (!Worklist.empty()) {

        BasicBlock *BB =
            Worklist.pop_back_val();

        InWorklist.erase(BB);

        DefinitionState State =
            InStates.lookup(BB);

        for (Instruction &I :
             *BB) {

            auto *Store =
                dyn_cast<StoreInst>(&I);

            if (!Store)
                continue;

            if (getStoredAlloca(Store) !=
                Target) {

                continue;
            }

            State.Initialized = true;
            State.Unknown = false;
            State.Stores.clear();
            State.Stores.push_back(Store);
        }

        for (BasicBlock *Succ :
             successors(BB)) {

            if (Succ == Stop) {

                if (!HasExit) {

                    ExitState =
                        State;

                    HasExit = true;

                } else {

                    mergeDefinitionState(
                        ExitState,
                        State);
                }

                continue;
            }

            if (!RegionSet.contains(Succ))
                continue;

            DefinitionState Old =
                InStates.lookup(Succ);

            DefinitionState New =
                Old;

            mergeDefinitionState(
                New,
                State);

            if (!sameDefinitionState(
                    Old,
                    New)) {

                InStates[Succ] =
                    New;

                if (InWorklist.insert(
                        Succ)
                        .second) {

                    Worklist.push_back(
                        Succ);
                }
            }
        }
    }

    return HasExit;
}

static bool isConditionValueDependent(
    const DefinitionState &TrueState,
    const DefinitionState &FalseState) {

    if (!TrueState.Initialized ||
        !FalseState.Initialized) {

        return false;
    }

    return !sameDefinitionState(
        TrueState,
        FalseState);
}

static uint64_t getAllocaSize(
    AllocaInst *AI,
    const DataLayout &DL) {

    if (!AI)
        return 0;

    TypeSize ElementSize =
        DL.getTypeAllocSize(
            AI->getAllocatedType());

    if (ElementSize.isScalable())
        return 0;

    auto *Count =
        dyn_cast<ConstantInt>(
            AI->getArraySize());

    if (!Count)
        return ElementSize.getFixedValue();

    return ElementSize.getFixedValue() *
           Count->getZExtValue();
}

/*
 * ------------------------------------------------------------
 * Condition-source analysis
 * ------------------------------------------------------------
 *
 * Example:
 *
 *     if (secret2)
 *
 * LLVM condition:
 *
 *     load secret2
 *     icmp ...
 *
 * We recover "secret2" as the source of the condition label.
 *
 * This is important for:
 *
 *     if (secret1) {
 *         x = 10;
 *     } else if (secret2) {
 *         y = 40;
 *     } else {
 *         z = 50;
 *     }
 *
 * The else-if condition is not dynamically executed when
 * secret1 is true. Nevertheless, its source label (2) must still
 * be available so that y/z receive:
 *
 *     label(secret1) U label(secret2)
 *
 * ------------------------------------------------------------
 */

static void addUniqueLabelSource(
    SmallVectorImpl<LabelSource> &Sources,
    Value *Pointer,
    Value *SSAValue) {

    for (const LabelSource &Existing :
         Sources) {

        if (Existing.Pointer == Pointer &&
            Existing.SSAValue == SSAValue) {

            return;
        }
    }

    Sources.push_back(
        {Pointer, SSAValue});
}

static void collectConditionSources(
    Value *V,
    SmallPtrSetImpl<Value *> &Visited,
    SmallVectorImpl<LabelSource> &Sources) {

    if (!V)
        return;

    if (isa<Constant>(V))
        return;

    V =
        V->stripPointerCasts();

    if (!Visited.insert(V).second)
        return;

    if (auto *Load =
            dyn_cast<LoadInst>(V)) {

        Value *Base =
            getBasePointer(
                Load->getPointerOperand());

        if (Base) {

            addUniqueLabelSource(
                Sources,
                Base,
                nullptr);

        } else {

            addUniqueLabelSource(
                Sources,
                Load->getPointerOperand(),
                nullptr);
        }

        return;
    }

    if (auto *PHI =
            dyn_cast<PHINode>(V)) {

        for (Value *Incoming :
             PHI->incoming_values()) {

            collectConditionSources(
                Incoming,
                Visited,
                Sources);
        }

        return;
    }

    if (auto *Select =
            dyn_cast<SelectInst>(V)) {

        collectConditionSources(
            Select->getCondition(),
            Visited,
            Sources);

        collectConditionSources(
            Select->getTrueValue(),
            Visited,
            Sources);

        collectConditionSources(
            Select->getFalseValue(),
            Visited,
            Sources);

        return;
    }

    if (auto *GEP =
            dyn_cast<GetElementPtrInst>(V)) {

        collectConditionSources(
            GEP->getPointerOperand(),
            Visited,
            Sources);

        for (Value *Index :
             GEP->indices()) {

            collectConditionSources(
                Index,
                Visited,
                Sources);
        }

        return;
    }

    if (auto *Call =
            dyn_cast<CallInst>(V)) {

        for (unsigned I = 0;
             I < Call->arg_size();
             ++I) {

            collectConditionSources(
                Call->getArgOperand(I),
                Visited,
                Sources);
        }

        return;
    }

    if (auto *I =
            dyn_cast<Instruction>(V)) {

        for (Value *Operand :
             I->operands()) {

            collectConditionSources(
                Operand,
                Visited,
                Sources);
        }

        return;
    }

    /*
     * Function argument or another non-instruction
     * SSA value.
     */
    addUniqueLabelSource(
        Sources,
        nullptr,
        V);
}

static bool isDFSanSetLabelCall(
    CallInst *CI) {

    if (!CI)
        return false;

    Function *Callee =
        CI->getCalledFunction();

    if (!Callee)
        return false;

    StringRef Name =
        Callee->getName();

    return Name ==
               "dfsan_set_label" ||
           Name ==
               "__dfsan_set_label";
}

static Instruction *findInitialLabelInsertionPoint(
    Function &F) {

    BasicBlock &Entry =
        F.getEntryBlock();

    Instruction *InsertPoint =
        &*Entry.getFirstInsertionPt();

    /*
     * Put our static implicit-label setup after all
     * dfsan_set_label calls in the entry block.
     *
     * This guarantees that:
     *
     *     dfsan_set_label(1, &secret1, ...)
     *     dfsan_set_label(2, &secret2, ...)
     *
     * have already initialized DFSan shadow memory.
     */
    for (Instruction &I :
         Entry) {

        auto *CI =
            dyn_cast<CallInst>(&I);

        if (!isDFSanSetLabelCall(CI))
            continue;

        InsertPoint =
            I.getNextNode();

        if (!InsertPoint)
            InsertPoint =
                Entry.getTerminator();
    }

    return InsertPoint;
}

static CallInst *findDominatingSetLabel(
    Function &F,
    AllocaInst *AI,
    DominatorTree &DT,
    Instruction *InsertBefore) {

    CallInst *Best =
        nullptr;

    for (Instruction &I :
         instructions(F)) {

        auto *CI =
            dyn_cast<CallInst>(&I);

        if (!isDFSanSetLabelCall(CI))
            continue;

        if (CI->arg_size() < 3)
            continue;

        Value *Address =
            getBasePointer(
                CI->getArgOperand(1));

        if (Address != AI)
            continue;

        if (!DT.dominates(
                CI,
                InsertBefore)) {

            continue;
        }

        Best = CI;
    }

    return Best;
}

static Type *getDFSanLabelType(
    Module &M) {

    if (Function *F =
            M.getFunction(
                "__dfsan_conditional_callback")) {

        if (F->arg_size() > 0)
            return F->getArg(0)->getType();
    }

    if (Function *F =
            M.getFunction(
                "__dfsan_conditional_callback_origin")) {

        if (F->arg_size() > 0)
            return F->getArg(0)->getType();
    }

    return Type::getInt8Ty(
        M.getContext());
}

static FunctionCallee getDFSanReadLabel(
    Module &M,
    Type *LabelTy) {

    LLVMContext &Ctx =
        M.getContext();

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *SizeTy =
        M.getDataLayout()
            .getIntPtrType(Ctx);

    FunctionType *FT =
        FunctionType::get(
            LabelTy,
            {
                PtrTy,
                SizeTy
            },
            false);

    return M.getOrInsertFunction(
        "dfsan_read_label",
        FT);
}

static FunctionCallee getDFSanGetLabel(
    Module &M,
    Type *LabelTy) {

    LLVMContext &Ctx =
        M.getContext();

    Type *DataTy =
        Type::getInt64Ty(Ctx);

    FunctionType *FT =
        FunctionType::get(
            LabelTy,
            {
                DataTy
            },
            false);

    return M.getOrInsertFunction(
        "dfsan_get_label",
        FT);
}

static FunctionCallee getDFSanAddLabel(
    Module &M,
    Type *LabelTy) {

    LLVMContext &Ctx =
        M.getContext();

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *SizeTy =
        M.getDataLayout()
            .getIntPtrType(Ctx);

    FunctionType *FT =
        FunctionType::get(
            Type::getVoidTy(Ctx),
            {
                LabelTy,
                PtrTy,
                SizeTy
            },
            false);

    return M.getOrInsertFunction(
        "dfsan_add_label",
        FT);
}

static FunctionCallee getDFSanUnion(
    Module &M,
    Type *LabelTy) {

    FunctionType *FT =
        FunctionType::get(
            LabelTy,
            {
                LabelTy,
                LabelTy
            },
            false);

    return M.getOrInsertFunction(
        "dfsan_union",
        FT);
}

static Value *getSourceLabel(
    Module &M,
    Function &F,
    IRBuilder<> &Builder,
    const LabelSource &Source,
    DominatorTree &DT,
    Instruction *InsertBefore,
    Type *LabelTy) {

    LLVMContext &Ctx =
        M.getContext();

    const DataLayout &DL =
        M.getDataLayout();

    /*
     * Stack/global memory source:
     *
     * Prefer the original dfsan_set_label()
     * label argument. This makes the source label
     * independent of whether the corresponding
     * condition is dynamically executed.
     */
    if (Source.Pointer) {

        Value *Base =
            getBasePointer(
                Source.Pointer);

        if (auto *AI =
                dyn_cast_or_null<AllocaInst>(
                    Base)) {

            if (CallInst *SetLabel =
                    findDominatingSetLabel(
                        F,
                        AI,
                        DT,
                        InsertBefore)) {

                Value *Label =
                    SetLabel->getArgOperand(0);

                if (Label->getType() !=
                    LabelTy) {

                    Label =
                        Builder.CreateIntCast(
                            Label,
                            LabelTy,
                            false,
                            "implicit.source.cast");
                }

                return Label;
            }
        }

        /*
         * No explicit dfsan_set_label() was found.
         * Read the current shadow label.
         */
        Value *Address =
            Builder.CreatePointerCast(
                Source.Pointer,
                PointerType::get(Ctx, 0),
                "implicit.source.address");

        uint64_t Size = 1;

        if (auto *AI =
                dyn_cast_or_null<AllocaInst>(
                    Base)) {

            TypeSize TS =
                DL.getTypeAllocSize(
                    AI->getAllocatedType());

            if (!TS.isScalable())
                Size =
                    TS.getFixedValue();
        }

        return Builder.CreateCall(
            getDFSanReadLabel(
                M,
                LabelTy),
            {
                Address,
                ConstantInt::get(
                    DL.getIntPtrType(Ctx),
                    Size)
            },
            "implicit.source.label");
    }

    /*
     * SSA source such as a function argument.
     */
    if (Source.SSAValue) {

        Value *V =
            Source.SSAValue;

        if (auto *Arg =
                dyn_cast<Argument>(V)) {

            Value *AsI64 =
                Builder.CreateIntCast(
                    Arg,
                    Type::getInt64Ty(Ctx),
                    false,
                    "implicit.argument.value");

            return Builder.CreateCall(
                getDFSanGetLabel(
                    M,
                    LabelTy),
                {
                    AsI64
                },
                "implicit.argument.label");
        }

        /*
         * A condition instruction that dominates the
         * insertion point can be queried directly.
         */
        if (auto *I =
                dyn_cast<Instruction>(V)) {

            if (DT.dominates(
                    I,
                    InsertBefore)) {

                Value *AsI64 =
                    Builder.CreateIntCast(
                        I,
                        Type::getInt64Ty(Ctx),
                        false,
                        "implicit.ssa.value");

                return Builder.CreateCall(
                    getDFSanGetLabel(
                        M,
                        LabelTy),
                    {
                        AsI64
                    },
                    "implicit.ssa.label");
            }
        }
    }

    return ConstantInt::get(
        LabelTy,
        0);
}

static Value *buildStaticConditionLabel(
    Module &M,
    Function &F,
    Instruction *InsertBefore,
    ConditionInfo &Condition,
    DominatorTree &DT,
    Type *LabelTy) {

    LLVMContext &Ctx =
        M.getContext();

    IRBuilder<> Builder(
        InsertBefore);

    SmallVector<
        LabelSource,
        8>
        Sources;

    SmallPtrSet<
        Value *,
        32>
        Visited;

    collectConditionSources(
        Condition.Branch->getCondition(),
        Visited,
        Sources);

    SmallVector<
        Value *,
        8>
        Labels;

    for (const LabelSource &Source :
         Sources) {

        Value *Label =
            getSourceLabel(
                M,
                F,
                Builder,
                Source,
                DT,
                InsertBefore,
                LabelTy);

        if (!Label)
            continue;

        if (Label->getType() !=
            LabelTy) {

            Label =
                Builder.CreateIntCast(
                    Label,
                    LabelTy,
                    false,
                    "implicit.condition.cast");
        }

        Labels.push_back(Label);
    }

    /*
     * For complex cases where the source analysis
     * could not recover a source, use the original
     * dynamic DFSan condition label as fallback.
     */
    if (Labels.empty() &&
        Condition.DynamicLabel) {

        Value *Label =
            Condition.DynamicLabel;

        if (Label->getType() !=
            LabelTy) {

            Label =
                Builder.CreateIntCast(
                    Label,
                    LabelTy,
                    false,
                    "implicit.dynamic.cast");
        }

        return Label;
    }

    if (Labels.empty())
        return ConstantInt::get(
            LabelTy,
            0);

    Value *Result =
        Labels.front();

    FunctionCallee Union =
        getDFSanUnion(
            M,
            LabelTy);

    for (unsigned I = 1;
         I < Labels.size();
         ++I) {

        Result =
            Builder.CreateCall(
                Union,
                {
                    Result,
                    Labels[I]
                },
                "implicit.condition.union");
    }

    return Result;
}

static void addLabelToVariable(
    Module &M,
    Instruction *InsertBefore,
    Value *Label,
    AllocaInst *AI) {

    if (!InsertBefore ||
        !Label ||
        !AI) {

        return;
    }

    const DataLayout &DL =
        M.getDataLayout();

    uint64_t Size =
        getAllocaSize(
            AI,
            DL);

    if (Size == 0)
        return;

    LLVMContext &Ctx =
        M.getContext();

    IRBuilder<> Builder(
        InsertBefore);

    Type *LabelTy =
        Label->getType();

    Value *Address =
        Builder.CreatePointerCast(
            AI,
            PointerType::get(Ctx, 0),
            "implicit.variable.address");

    Value *SizeValue =
        ConstantInt::get(
            DL.getIntPtrType(Ctx),
            Size);

    Builder.CreateCall(
        getDFSanAddLabel(
            M,
            LabelTy),
        {
            Label,
            Address,
            SizeValue
        });
}

static void restoreLabelAfterStore(
    Module &M,
    StoreInst *Store,
    Value *ConditionLabel) {

    if (!Store ||
        !ConditionLabel) {

        return;
    }

    Instruction *InsertBefore =
        Store->getNextNode();

    if (!InsertBefore)
        return;

    TypeSize StoreSize =
        M.getDataLayout()
            .getTypeStoreSize(
                Store->getValueOperand()
                    ->getType());

    if (StoreSize.isScalable())
        return;

    uint64_t Size =
        StoreSize.getFixedValue();

    if (Size == 0)
        return;

    LLVMContext &Ctx =
        M.getContext();

    IRBuilder<> Builder(
        InsertBefore);

    Value *Address =
        Builder.CreatePointerCast(
            Store->getPointerOperand(),
            PointerType::get(Ctx, 0),
            "implicit.restore.address");

    Value *SizeValue =
        ConstantInt::get(
            M.getDataLayout()
                .getIntPtrType(Ctx),
            Size);

    Value *Label =
        ConditionLabel;

    Builder.CreateCall(
        getDFSanAddLabel(
            M,
            Label->getType()),
        {
            Label,
            Address,
            SizeValue
        });
}

static FunctionCallee getRuntimeCallback(
    Module &M,
    Type *LabelTy) {

    LLVMContext &Ctx =
        M.getContext();

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    FunctionType *FT =
        FunctionType::get(
            Type::getVoidTy(Ctx),
            {
                LabelTy,
                LabelTy,
                I32Ty,
                I32Ty,
                PtrTy
            },
            false);

    return M.getOrInsertFunction(
        "__implicit_branch_callback",
        FT);
}

static void reportVariable(
    Module &M,
    Instruction *InsertBefore,
    Value *ConditionLabel,
    AllocaInst *AI,
    const VariableInfo &Variables,
    unsigned Line,
    unsigned Column) {

    if (!InsertBefore ||
        !ConditionLabel ||
        !AI) {

        return;
    }

    LLVMContext &Ctx =
        M.getContext();

    const DataLayout &DL =
        M.getDataLayout();

    uint64_t Size =
        getAllocaSize(
            AI,
            DL);

    if (Size == 0)
        return;

    IRBuilder<> Builder(
        InsertBefore);

    Type *LabelTy =
        ConditionLabel->getType();

    Value *Address =
        Builder.CreatePointerCast(
            AI,
            PointerType::get(Ctx, 0),
            "implicit.report.address");

    Value *SizeValue =
        ConstantInt::get(
            DL.getIntPtrType(Ctx),
            Size);

    Value *FinalLabel =
        Builder.CreateCall(
            getDFSanReadLabel(
                M,
                LabelTy),
            {
                Address,
                SizeValue
            },
            "implicit.final.label");

    Value *Name =
        Builder.CreateGlobalString(
            getVariableName(
                AI,
                Variables),
            "implicit.variable.name");

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    Builder.CreateCall(
        getRuntimeCallback(
            M,
            LabelTy),
        {
            ConditionLabel,
            FinalLabel,
            ConstantInt::get(
                I32Ty,
                Line),
            ConstantInt::get(
                I32Ty,
                Column),
            Name
        });
}

class ImplicitTaintPass
    : public PassInfoMixin<
          ImplicitTaintPass> {

public:

    PreservedAnalyses run(
        Module &M,
        ModuleAnalysisManager &) {

        bool Changed = false;

        Type *LabelTy =
            getDFSanLabelType(M);

        for (Function &F : M) {

            if (F.isDeclaration())
                continue;

            VariableInfo Variables;

            buildVariableInfo(
                F,
                Variables);

            DominatorTree DT;
            DT.recalculate(F);

            PostDominatorTree PDT;
            PDT.recalculate(F);

            SmallVector<
                ConditionInfo,
                16>
                Conditions;

            /*
             * ----------------------------------------------------
             * PASS 1:
             * Collect every conditional branch and its DFSan
             * conditional label.
             * ----------------------------------------------------
             */
            for (BasicBlock &BB :
                 F) {

                auto *BR =
                    dyn_cast<BranchInst>(
                        BB.getTerminator());

                if (!BR ||
                    !BR->isConditional()) {

                    continue;
                }

                CallInst *Callback =
                    findDFSanConditionalCallback(
                        BR);

                if (!Callback)
                    continue;

                if (Callback->arg_size() == 0)
                    continue;

                ConditionInfo Info;

                Info.Branch =
                    BR;

                Info.Callback =
                    Callback;

                Info.DynamicLabel =
                    Callback->getArgOperand(0);

                if (DebugLoc DL =
                        BR->getDebugLoc()) {

                    Info.Line =
                        DL.getLine();

                    Info.Column =
                        DL.getCol();

                } else if (
                    auto *CondI =
                        dyn_cast<Instruction>(
                            BR->getCondition())) {

                    if (DebugLoc DL =
                            CondI->getDebugLoc()) {

                        Info.Line =
                            DL.getLine();

                        Info.Column =
                            DL.getCol();
                    }
                }

                Conditions.push_back(
                    Info);
            }

            if (Conditions.empty())
                continue;

            /*
             * DependentVariables[C] =
             * variables whose final values are affected
             * by condition C.
             */
            SmallVector<
                SmallVector<
                    AllocaInst *,
                    16>,
                16>
                DependentVariables(
                    Conditions.size());

            /*
             * For each variable keep every controlling
             * condition label that must be restored after
             * stores.
             */
            DenseMap<
                AllocaInst *,
                SmallVector<
                    unsigned,
                    8>>
                VariableConditions;

            /*
             * ----------------------------------------------------
             * PASS 2:
             *
             * Determine the ultimate variables affected by every
             * conditional.
             *
             * This preserves the behavior that was already working
             * for the user's previous test cases.
             * ----------------------------------------------------
             */
            for (unsigned Index = 0;
                 Index < Conditions.size();
                 ++Index) {

                ConditionInfo &Condition =
                    Conditions[Index];

                BranchInst *BR =
                    Condition.Branch;

                BasicBlock *Merge =
                    getMergeBlock(
                        BR,
                        PDT);

                if (!Merge)
                    continue;

                SmallVector<
                    BasicBlock *,
                    32>
                    TrueRegion;

                SmallVector<
                    BasicBlock *,
                    32>
                    FalseRegion;

                collectRegion(
                    BR->getSuccessor(0),
                    Merge,
                    TrueRegion);

                collectRegion(
                    BR->getSuccessor(1),
                    Merge,
                    FalseRegion);

                SmallVector<
                    AllocaInst *,
                    32>
                    Candidates;

                collectTargets(
                    TrueRegion,
                    Candidates);

                collectTargets(
                    FalseRegion,
                    Candidates);

                for (AllocaInst *AI :
                     Candidates) {

                    DefinitionState TrueState;
                    DefinitionState FalseState;

                    bool HasTrue =
                        collectFinalDefinitions(
                            TrueRegion,
                            BR->getSuccessor(0),
                            Merge,
                            AI,
                            TrueState);

                    bool HasFalse =
                        collectFinalDefinitions(
                            FalseRegion,
                            BR->getSuccessor(1),
                            Merge,
                            AI,
                            FalseState);

                    if (!HasTrue ||
                        !HasFalse) {

                        continue;
                    }

                    if (!isConditionValueDependent(
                            TrueState,
                            FalseState)) {

                        continue;
                    }

                    DependentVariables[Index]
                        .push_back(AI);

                    VariableConditions[AI]
                        .push_back(Index);
                }
            }

            /*
             * ----------------------------------------------------
             * PASS 3:
             *
             * Build a STATIC label for every condition.
             *
             * This is the critical change.
             *
             * For:
             *
             *     if (secret1) { ... }
             *     else if (secret2) { ... }
             *
             * the second condition is not dynamically executed
             * when secret1 is true.
             *
             * Therefore using only the dynamic callback label is
             * insufficient.
             *
             * Instead:
             *
             *     condition1 -> label(secret1)
             *     condition2 -> label(secret2)
             *
             * are recovered independently.
             * ----------------------------------------------------
             */
            Instruction *LabelInsertPoint =
                findInitialLabelInsertionPoint(F);

            if (!LabelInsertPoint)
                LabelInsertPoint =
                    F.getEntryBlock().getTerminator();

            SmallVector<
                Value *,
                16>
                ConditionLabels(
                    Conditions.size(),
                    nullptr);

            for (unsigned Index = 0;
                 Index < Conditions.size();
                 ++Index) {

                ConditionLabels[Index] =
                    buildStaticConditionLabel(
                        M,
                        F,
                        LabelInsertPoint,
                        Conditions[Index],
                        DT,
                        LabelTy);
            }

            /*
             * ----------------------------------------------------
             * PASS 4:
             *
             * Add EVERY controlling condition label to the
             * dependent variable at the common entry point.
             *
             * Example:
             *
             *     x -> condition1
             *     y -> condition1 + condition2
             *     z -> condition1 + condition2
             *
             * Thus y/z receive label 1 and label 2 even if the
             * condition2 branch is skipped in the current runtime
             * execution.
             *
             * dfsan_add_label() performs a UNION, never a replace.
             * ----------------------------------------------------
             */
            for (unsigned Index = 0;
                 Index < Conditions.size();
                 ++Index) {

                Value *ConditionLabel =
                    ConditionLabels[Index];

                if (!ConditionLabel)
                    continue;

                if (isa<ConstantInt>(
                        ConditionLabel)) {

                    auto *CI =
                        cast<ConstantInt>(
                            ConditionLabel);

                    if (CI->isZero())
                        continue;
                }

                for (AllocaInst *AI :
                     DependentVariables[Index]) {

                    addLabelToVariable(
                        M,
                        LabelInsertPoint,
                        ConditionLabel,
                        AI);

                    Changed = true;
                }
            }

            /*
             * ----------------------------------------------------
             * PASS 5:
             *
             * DFSan stores may overwrite shadow memory with the
             * explicit label of the stored value.
             *
             * Restore the COMPLETE static set of controlling
             * labels after every store.
             *
             * Example:
             *
             *     y = 40;
             *
             * after the store:
             *
             *     label(y) = label(condition1)
             *              U label(condition2)
             * ----------------------------------------------------
             */
            for (auto &Entry :
                 VariableConditions) {

                AllocaInst *AI =
                    Entry.first;

                if (!AI)
                    continue;

                for (Instruction &I :
                     instructions(F)) {

                    auto *Store =
                        dyn_cast<StoreInst>(&I);

                    if (!Store)
                        continue;

                    if (getStoredAlloca(Store) !=
                        AI) {

                        continue;
                    }

                    for (unsigned Index :
                         Entry.second) {

                        if (Index >=
                            ConditionLabels.size()) {

                            continue;
                        }

                        Value *Label =
                            ConditionLabels[Index];

                        if (!Label)
                            continue;

                        if (isa<ConstantInt>(
                                Label)) {

                            auto *CI =
                                cast<ConstantInt>(
                                    Label);

                            if (CI->isZero())
                                continue;
                        }

                        restoreLabelAfterStore(
                            M,
                            Store,
                            Label);

                        Changed = true;
                    }
                }
            }

            /*
             * ----------------------------------------------------
             * PASS 6:
             *
             * Report the final labels at returns.
             * ----------------------------------------------------
             */
            for (auto &Entry :
                 VariableConditions) {

                AllocaInst *AI =
                    Entry.first;

                if (!AI)
                    continue;

                for (Instruction &I :
                     instructions(F)) {

                    auto *Ret =
                        dyn_cast<ReturnInst>(&I);

                    if (!Ret)
                        continue;

                    for (unsigned Index :
                         Entry.second) {

                        if (Index >=
                            ConditionLabels.size()) {

                            continue;
                        }

                        Value *Label =
                            ConditionLabels[Index];

                        if (!Label)
                            continue;

                        if (isa<ConstantInt>(
                                Label)) {

                            auto *CI =
                                cast<ConstantInt>(
                                    Label);

                            if (CI->isZero())
                                continue;
                        }

                        reportVariable(
                            M,
                            Ret,
                            Label,
                            AI,
                            Variables,
                            Conditions[Index].Line,
                            Conditions[Index].Column);

                        Changed = true;
                    }
                }
            }

            /*
             * ----------------------------------------------------
             * PASS 7:
             *
             * Remove DFSan's conditional callbacks after their
             * labels have been captured.
             * ----------------------------------------------------
             */
            for (ConditionInfo &Condition :
                 Conditions) {

                if (!Condition.Callback)
                    continue;

                if (!Condition.Callback->getParent())
                    continue;

                Condition.Callback
                    ->eraseFromParent();

                Changed = true;
            }
        }

        return Changed
                   ? PreservedAnalyses::none()
                   : PreservedAnalyses::all();
    }
};

} // namespace

extern "C"
LLVM_ATTRIBUTE_WEAK
PassPluginLibraryInfo
llvmGetPassPluginInfo() {

    return {
        LLVM_PLUGIN_API_VERSION,
        "ImplicitTaint",
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
                            ImplicitTaintPass());

                        return true;
                    }

                    return false;
                });
        }
    };
}