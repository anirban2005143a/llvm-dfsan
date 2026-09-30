// File: FindTaintedCondition.cpp

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugProgramInstruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/PostDominators.h"
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
    Value *Label = nullptr;
    unsigned Line = 0;
    unsigned Column = 0;
};

struct DefinitionState {
    bool Initialized = false;
    bool Unknown = false;
    SmallVector<StoreInst *, 8> Stores;
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

        if (I.hasDbgRecords()) {

            for (DbgRecord &DR : I.getDbgRecordRange()) {

                auto *DVR =
                    dyn_cast<DbgVariableRecord>(&DR);

                if (!DVR)
                    continue;

                DILocalVariable *Var =
                    DVR->getVariable();

                if (!Var)
                    continue;

                std::string Name =
                    Var->getName().str();

                for (unsigned Op = 0;
                     Op < DVR->getNumVariableLocationOps();
                     ++Op) {

                    Value *V =
                        DVR->getVariableLocationOp(Op);

                    if (!V)
                        continue;

                    Value *Base =
                        getBasePointer(V);

                    if (auto *AI =
                            dyn_cast_or_null<AllocaInst>(
                                Base)) {

                        Info.Names[AI] = Name;
                    }
                }
            }
        }

        if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var =
                DDI->getVariable();

            Value *Addr =
                DDI->getAddress();

            if (!Var || !Addr)
                continue;

            Value *Base =
                getBasePointer(Addr);

            if (auto *AI =
                    dyn_cast_or_null<AllocaInst>(Base)) {

                Info.Names[AI] =
                    Var->getName().str();
            }
        }
    }
}

static std::string getVariableName(
    AllocaInst *AI,
    const VariableInfo &Info) {

    if (!AI)
        return "unknown";

    auto It = Info.Names.find(AI);

    if (It != Info.Names.end())
        return It->second;

    return "unknown";
}

static CallInst *findDFSanConditionalCallback(
    BranchInst *BR) {

    if (!BR)
        return nullptr;

    Instruction *Cur =
        BR->getPrevNode();

    while (Cur) {

        if (auto *CI = dyn_cast<CallInst>(Cur)) {

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

        Cur = Cur->getPrevNode();
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

    SmallVector<BasicBlock *, 64> Worklist;

    SmallPtrSet<BasicBlock *, 32> Visited;

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

        for (BasicBlock *Succ : successors(BB)) {

            if (Succ == Stop)
                continue;

            if (!Visited.contains(Succ))
                Worklist.push_back(Succ);
        }
    }
}

static AllocaInst *getStoredAlloca(
    StoreInst *Store) {

    if (!Store)
        return nullptr;

    Value *Base =
        getBasePointer(
            Store->getPointerOperand());

    return dyn_cast_or_null<AllocaInst>(Base);
}

static bool addStoreUnique(
    SmallVectorImpl<StoreInst *> &Stores,
    StoreInst *Store) {

    for (StoreInst *Existing : Stores) {

        if (Existing == Store)
            return false;
    }

    Stores.push_back(Store);
    return true;
}

static bool mergeDefinitionState(
    DefinitionState &Dst,
    const DefinitionState &Src) {

    if (!Src.Initialized)
        return false;

    if (!Dst.Initialized) {

        Dst = Src;
        return true;
    }

    bool Changed = false;

    if (Src.Unknown && !Dst.Unknown) {
        Dst.Unknown = true;
        Changed = true;
    }

    for (StoreInst *Store : Src.Stores) {

        if (addStoreUnique(Dst.Stores, Store))
            Changed = true;
    }

    return Changed;
}

static bool sameStoredValue(
    StoreInst *A,
    StoreInst *B) {

    if (!A || !B)
        return false;

    if (A == B)
        return true;

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

    if (CA->getType() != CB->getType())
        return false;

    return CA->isElementWiseEqual(CB);
}

static bool sameDefinitionState(
    const DefinitionState &A,
    const DefinitionState &B) {

    if (!A.Initialized || !B.Initialized)
        return A.Initialized == B.Initialized;

    if (A.Unknown != B.Unknown)
        return false;

    for (StoreInst *SA : A.Stores) {

        bool Found = false;

        for (StoreInst *SB : B.Stores) {

            if (sameStoredValue(SA, SB)) {
                Found = true;
                break;
            }
        }

        if (!Found)
            return false;
    }

    for (StoreInst *SB : B.Stores) {

        bool Found = false;

        for (StoreInst *SA : A.Stores) {

            if (sameStoredValue(SA, SB)) {
                Found = true;
                break;
            }
        }

        if (!Found)
            return false;
    }

    return true;
}

static bool collectFinalDefinitions(
    ArrayRef<BasicBlock *> Region,
    BasicBlock *Start,
    BasicBlock *Stop,
    AllocaInst *Target,
    DefinitionState &ExitState) {

    ExitState = DefinitionState();

    if (!Start || !Stop || !Target)
        return false;

    if (Region.empty())
        return false;

    SmallPtrSet<BasicBlock *, 32> RegionSet;

    for (BasicBlock *BB : Region)
        RegionSet.insert(BB);

    if (!RegionSet.contains(Start))
        return false;

    DenseMap<BasicBlock *, DefinitionState> InStates;

    SmallVector<BasicBlock *, 64> Worklist;

    DefinitionState Initial;
    Initial.Initialized = true;
    Initial.Unknown = true;

    InStates[Start] = Initial;
    Worklist.push_back(Start);

    SmallPtrSet<BasicBlock *, 32> InWorklist;

    InWorklist.insert(Start);

    bool HasExit = false;

    while (!Worklist.empty()) {

        BasicBlock *BB =
            Worklist.pop_back_val();

        InWorklist.erase(BB);

        auto InIt =
            InStates.find(BB);

        if (InIt == InStates.end())
            continue;

        DefinitionState State =
            InIt->second;

        for (Instruction &I : *BB) {

            auto *Store =
                dyn_cast<StoreInst>(&I);

            if (!Store)
                continue;

            AllocaInst *StoredAI =
                getStoredAlloca(Store);

            if (StoredAI != Target)
                continue;

            State.Initialized = true;
            State.Unknown = false;
            State.Stores.clear();
            State.Stores.push_back(Store);
        }

        Instruction *Term =
            BB->getTerminator();

        if (!Term)
            continue;

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

            bool Changed =
                mergeDefinitionState(
                    InStates[Succ],
                    State);

            if (Changed &&
                !InWorklist.contains(Succ)) {

                Worklist.push_back(Succ);
                InWorklist.insert(Succ);
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

        return true;
    }

    return !sameDefinitionState(
        TrueState,
        FalseState);
}

static void addUniqueConditionIndex(
    SmallVectorImpl<unsigned> &Indices,
    unsigned Index) {

    for (unsigned Existing : Indices) {

        if (Existing == Index)
            return;
    }

    Indices.push_back(Index);
}

static uint64_t getAllocaSize(
    AllocaInst *AI,
    const DataLayout &DL) {

    if (!AI)
        return 0;

    Type *AllocatedType =
        AI->getAllocatedType();

    if (!AllocatedType)
        return 0;

    if (auto *Count =
            dyn_cast<ConstantInt>(
                AI->getArraySize())) {

        TypeSize ElementSize =
            DL.getTypeAllocSize(
                AllocatedType);

        if (ElementSize.isScalable())
            return 0;

        return ElementSize.getFixedValue() *
               Count->getZExtValue();
    }

    TypeSize Size =
        DL.getTypeAllocSize(
            AllocatedType);

    if (Size.isScalable())
        return 0;

    return Size.getFixedValue();
}

static FunctionCallee getDFSanAddLabel(
    Module &M) {

    LLVMContext &Ctx =
        M.getContext();

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Type *VoidTy =
        Type::getVoidTy(Ctx);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *SizeTy =
        M.getDataLayout().getIntPtrType(Ctx);

    FunctionType *Ty =
        FunctionType::get(
            VoidTy,
            {
                I8Ty,
                PtrTy,
                SizeTy
            },
            false);

    return M.getOrInsertFunction(
        "dfsan_add_label",
        Ty);
}

static FunctionCallee getDFSanReadLabel(
    Module &M) {

    LLVMContext &Ctx =
        M.getContext();

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *SizeTy =
        M.getDataLayout().getIntPtrType(Ctx);

    FunctionType *Ty =
        FunctionType::get(
            I8Ty,
            {
                PtrTy,
                SizeTy
            },
            false);

    return M.getOrInsertFunction(
        "dfsan_read_label",
        Ty);
}

static FunctionCallee getRuntimeCallback(
    Module &M) {

    LLVMContext &Ctx =
        M.getContext();

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    Type *VoidTy =
        Type::getVoidTy(Ctx);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    FunctionType *Ty =
        FunctionType::get(
            VoidTy,
            {
                I8Ty,
                I8Ty,
                I32Ty,
                I32Ty,
                PtrTy
            },
            false);

    return M.getOrInsertFunction(
        "__implicit_branch_callback",
        Ty);
}

static void addConditionLabelToVariable(
    Module &M,
    Instruction *InsertBefore,
    Value *ConditionLabel,
    AllocaInst *AI,
    const VariableInfo &Variables,
    unsigned Line,
    unsigned Column,
    bool Report) {

    if (!InsertBefore)
        return;

    if (!ConditionLabel)
        return;

    if (!AI)
        return;

    const DataLayout &DL =
        M.getDataLayout();

    uint64_t Size =
        getAllocaSize(AI, DL);

    if (Size == 0)
        return;

    LLVMContext &Ctx =
        M.getContext();

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Type *SizeTy =
        DL.getIntPtrType(Ctx);

    IRBuilder<> Builder(
        InsertBefore);

    if (ConditionLabel->getType() != I8Ty) {

        ConditionLabel =
            Builder.CreateIntCast(
                ConditionLabel,
                I8Ty,
                false,
                "implicit.condition.label");
    }

    FunctionCallee AddLabel =
        getDFSanAddLabel(M);

    Value *Address =
        Builder.CreatePointerCast(
            AI,
            PtrTy,
            "implicit.target.address");

    Value *SizeValue =
        ConstantInt::get(
            SizeTy,
            Size);

    Builder.CreateCall(
        AddLabel,
        {
            ConditionLabel,
            Address,
            SizeValue
        });

    if (!Report)
        return;

    FunctionCallee ReadLabel =
        getDFSanReadLabel(M);

    FunctionCallee Callback =
        getRuntimeCallback(M);

    Value *FinalLabel =
        Builder.CreateCall(
            ReadLabel,
            {
                Address,
                SizeValue
            },
            "implicit.final.label");

    std::string Name =
        getVariableName(
            AI,
            Variables);

    Value *NamePtr =
        Builder.CreateGlobalString(
            Name,
            "implicit.variable.name");

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    Builder.CreateCall(
        Callback,
        {
            ConditionLabel,
            FinalLabel,
            ConstantInt::get(
                I32Ty,
                Line),
            ConstantInt::get(
                I32Ty,
                Column),
            NamePtr
        });
}

static void restoreConditionLabelsAfterStore(
    Module &M,
    StoreInst *Store,
    const std::vector<ConditionInfo> &Conditions,
    const DenseMap<
        StoreInst *,
        SmallVector<unsigned, 8>> &Controllers) {

    if (!Store)
        return;

    auto It =
        Controllers.find(Store);

    if (It == Controllers.end())
        return;

    if (It->second.empty())
        return;

    TypeSize StoreSize =
        M.getDataLayout().getTypeStoreSize(
            Store->getValueOperand()->getType());

    if (StoreSize.isScalable())
        return;

    uint64_t Size =
        StoreSize.getFixedValue();

    if (Size == 0)
        return;

    Instruction *InsertBefore =
        Store->getNextNode();

    if (!InsertBefore)
        return;

    LLVMContext &Ctx =
        M.getContext();

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *SizeTy =
        M.getDataLayout().getIntPtrType(Ctx);

    FunctionCallee AddLabel =
        getDFSanAddLabel(M);

    IRBuilder<> Builder(
        InsertBefore);

    Value *Address =
        Builder.CreatePointerCast(
            Store->getPointerOperand(),
            PtrTy,
            "implicit.restore.address");

    Value *SizeValue =
        ConstantInt::get(
            SizeTy,
            Size);

    for (unsigned Index :
         It->second) {

        if (Index >= Conditions.size())
            continue;

        Value *Label =
            Conditions[Index].Label;

        if (!Label)
            continue;

        if (Label->getType() != I8Ty) {

            Label =
                Builder.CreateIntCast(
                    Label,
                    I8Ty,
                    false,
                    "implicit.restore.label");
        }

        Builder.CreateCall(
            AddLabel,
            {
                Label,
                Address,
                SizeValue
            });
    }
}

class ImplicitTaintPass
    : public PassInfoMixin<ImplicitTaintPass> {

public:

    PreservedAnalyses run(
        Module &M,
        ModuleAnalysisManager &) {

        bool Changed = false;

        VariableInfo Variables;

        std::vector<ConditionInfo>
            Conditions;

        DenseMap<
            StoreInst *,
            SmallVector<unsigned, 8>>
            Controllers;

        DenseMap<
            unsigned,
            SmallVector<AllocaInst *, 16>>
            DependentTargets;

        for (Function &F : M) {

            if (F.isDeclaration())
                continue;

            buildVariableInfo(
                F,
                Variables);

            PostDominatorTree PDT;
            PDT.recalculate(F);

            SmallVector<BranchInst *, 64>
                Branches;

            for (BasicBlock &BB : F) {

                auto *BR =
                    dyn_cast<BranchInst>(
                        BB.getTerminator());

                if (!BR)
                    continue;

                if (!BR->isConditional())
                    continue;

                Branches.push_back(BR);
            }

            SmallVector<unsigned, 64>
                FunctionConditionIndices;

            for (BranchInst *BR : Branches) {

                CallInst *DFCall =
                    findDFSanConditionalCallback(
                        BR);

                if (!DFCall)
                    continue;

                if (DFCall->arg_size() == 0)
                    continue;

                Value *ConditionLabel =
                    DFCall->getArgOperand(0);

                if (!ConditionLabel)
                    continue;

                LLVMContext &Ctx =
                    M.getContext();

                Type *I8Ty =
                    Type::getInt8Ty(Ctx);

                if (ConditionLabel->getType() !=
                    I8Ty) {

                    IRBuilder<> Builder(
                        DFCall);

                    ConditionLabel =
                        Builder.CreateIntCast(
                            ConditionLabel,
                            I8Ty,
                            false,
                            "implicit.condition.label");
                }

                unsigned Line = 0;
                unsigned Column = 0;

                DebugLoc DL =
                    BR->getDebugLoc();

                if (DL) {

                    Line =
                        DL.getLine();

                    Column =
                        DL.getCol();

                } else if (
                    auto *CondI =
                        dyn_cast<Instruction>(
                            BR->getCondition())) {

                    DebugLoc CondDL =
                        CondI->getDebugLoc();

                    if (CondDL) {

                        Line =
                            CondDL.getLine();

                        Column =
                            CondDL.getCol();
                    }
                }

                unsigned Index =
                    static_cast<unsigned>(
                        Conditions.size());

                Conditions.push_back(
                    {
                        BR,
                        ConditionLabel,
                        Line,
                        Column
                    });

                FunctionConditionIndices.push_back(
                    Index);
            }

            for (unsigned Index :
                 FunctionConditionIndices) {

                ConditionInfo &Condition =
                    Conditions[Index];

                BranchInst *BR =
                    Condition.Branch;

                if (!BR)
                    continue;

                BasicBlock *Merge =
                    getMergeBlock(
                        BR,
                        PDT);

                if (!Merge)
                    continue;

                SmallVector<BasicBlock *, 64>
                    TrueRegion;

                SmallVector<BasicBlock *, 64>
                    FalseRegion;

                collectRegion(
                    BR->getSuccessor(0),
                    Merge,
                    TrueRegion);

                collectRegion(
                    BR->getSuccessor(1),
                    Merge,
                    FalseRegion);

                SmallVector<AllocaInst *, 32>
                    Targets;

                for (BasicBlock *BB :
                     TrueRegion) {

                    for (Instruction &I :
                         *BB) {

                        auto *Store =
                            dyn_cast<StoreInst>(&I);

                        if (!Store)
                            continue;

                        AllocaInst *AI =
                            getStoredAlloca(Store);

                        if (!AI)
                            continue;

                        bool Exists = false;

                        for (AllocaInst *Existing :
                             Targets) {

                            if (Existing == AI) {
                                Exists = true;
                                break;
                            }
                        }

                        if (!Exists)
                            Targets.push_back(AI);
                    }
                }

                for (BasicBlock *BB :
                     FalseRegion) {

                    for (Instruction &I :
                         *BB) {

                        auto *Store =
                            dyn_cast<StoreInst>(&I);

                        if (!Store)
                            continue;

                        AllocaInst *AI =
                            getStoredAlloca(Store);

                        if (!AI)
                            continue;

                        bool Exists = false;

                        for (AllocaInst *Existing :
                             Targets) {

                            if (Existing == AI) {
                                Exists = true;
                                break;
                            }
                        }

                        if (!Exists)
                            Targets.push_back(AI);
                    }
                }

                for (AllocaInst *AI :
                     Targets) {

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

                    if (!HasTrue || !HasFalse)
                        continue;

                    if (!isConditionValueDependent(
                            TrueState,
                            FalseState)) {

                        continue;
                    }

                    bool AlreadyDependent = false;

                    for (AllocaInst *Existing :
                         DependentTargets[Index]) {

                        if (Existing == AI) {
                            AlreadyDependent = true;
                            break;
                        }
                    }

                    if (!AlreadyDependent) {

                        DependentTargets[Index]
                            .push_back(AI);
                    }
                }

                for (BasicBlock *BB :
                     TrueRegion) {

                    for (Instruction &I :
                         *BB) {

                        auto *Store =
                            dyn_cast<StoreInst>(&I);

                        if (!Store)
                            continue;

                        AllocaInst *AI =
                            getStoredAlloca(Store);

                        if (!AI)
                            continue;

                        bool Dependent = false;

                        for (AllocaInst *Target :
                             DependentTargets[Index]) {

                            if (Target == AI) {
                                Dependent = true;
                                break;
                            }
                        }

                        if (Dependent) {

                            addUniqueConditionIndex(
                                Controllers[Store],
                                Index);
                        }
                    }
                }

                for (BasicBlock *BB :
                     FalseRegion) {

                    for (Instruction &I :
                         *BB) {

                        auto *Store =
                            dyn_cast<StoreInst>(&I);

                        if (!Store)
                            continue;

                        AllocaInst *AI =
                            getStoredAlloca(Store);

                        if (!AI)
                            continue;

                        bool Dependent = false;

                        for (AllocaInst *Target :
                             DependentTargets[Index]) {

                            if (Target == AI) {
                                Dependent = true;
                                break;
                            }
                        }

                        if (Dependent) {

                            addUniqueConditionIndex(
                                Controllers[Store],
                                Index);
                        }
                    }
                }
            }
        }

        /*
         * Add the implicit condition label to every variable
         * whose possible final value differs between the
         * two paths of the condition.
         *
         * This happens BEFORE the branch, so a variable such as:
         *
         *     if (secret2)
         *         y = 20;
         *     else
         *         z = 30;
         *
         * causes y to be tainted even when the y store is not
         * executed on the current path.
         */
        for (unsigned Index = 0;
             Index < Conditions.size();
             ++Index) {

            ConditionInfo &Condition =
                Conditions[Index];

            BranchInst *BR =
                Condition.Branch;

            if (!BR)
                continue;

            auto It =
                DependentTargets.find(Index);

            if (It == DependentTargets.end())
                continue;

            if (It->second.empty())
                continue;

            for (AllocaInst *AI :
                 It->second) {

                addConditionLabelToVariable(
                    M,
                    BR,
                    Condition.Label,
                    AI,
                    Variables,
                    Condition.Line,
                    Condition.Column,
                    true);

                Changed = true;
            }
        }

        /*
         * A normal DFSan store can overwrite an implicit label
         * with the explicit value's label. Restore the implicit
         * labels after every controlled store.
         */
        for (auto &Entry :
             Controllers) {

            StoreInst *Store =
                Entry.first;

            if (!Store)
                continue;

            restoreConditionLabelsAfterStore(
                M,
                Store,
                Conditions,
                Controllers);

            Changed = true;
        }

        /*
         * Remove DFSan's experimental conditional callbacks.
         * Their labels were already captured above.
         */
        for (ConditionInfo &Condition :
             Conditions) {

            if (!Condition.Branch)
                continue;

            CallInst *CI =
                findDFSanConditionalCallback(
                    Condition.Branch);

            if (!CI)
                continue;

            if (CI->getParent()) {

                CI->eraseFromParent();
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