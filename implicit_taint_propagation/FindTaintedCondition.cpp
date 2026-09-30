#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugProgramInstruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace {

struct VariableInfo {
    std::unordered_map<const AllocaInst *, std::string> Names;
};

struct ConditionInfo {
    BranchInst *Branch = nullptr;
    Value *Label = nullptr;
    unsigned Line = 0;
    unsigned Column = 0;
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

                        Info.Names[AI] =
                            Name;
                    }
                }
            }
        }

        if (auto *DDI =
                dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var =
                DDI->getVariable();

            Value *Addr =
                DDI->getAddress();

            if (!Var || !Addr)
                continue;

            Value *Base =
                getBasePointer(Addr);

            if (auto *AI =
                    dyn_cast_or_null<AllocaInst>(
                        Base)) {

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

    auto It =
        Info.Names.find(AI);

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

    SmallVector<BasicBlock *, 32> Worklist;

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

        for (BasicBlock *Succ :
             successors(BB)) {

            if (Succ == Stop)
                continue;

            if (!Visited.contains(Succ))
                Worklist.push_back(Succ);
        }
    }
}

static void addUniqueTarget(
    SmallVectorImpl<AllocaInst *> &Targets,
    AllocaInst *AI) {

    if (!AI)
        return;

    for (AllocaInst *Existing : Targets) {

        if (Existing == AI)
            return;
    }

    Targets.push_back(AI);
}

static void collectTargetsFromRegion(
    ArrayRef<BasicBlock *> Region,
    SmallVectorImpl<AllocaInst *> &Targets) {

    for (BasicBlock *BB : Region) {

        for (Instruction &I : *BB) {

            auto *Store =
                dyn_cast<StoreInst>(&I);

            if (!Store)
                continue;

            Value *Base =
                getBasePointer(
                    Store->getPointerOperand());

            if (auto *AI =
                    dyn_cast_or_null<AllocaInst>(
                        Base)) {

                addUniqueTarget(
                    Targets,
                    AI);
            }
        }
    }
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

    if (auto Count =
            dyn_cast<ConstantInt>(
                AI->getArraySize())) {

        uint64_t ElementSize =
            DL.getTypeAllocSize(
                AllocatedType);

        return ElementSize *
               Count->getZExtValue();
    }

    /*
     * For the current O0 test cases alloca arrays
     * are not needed. Use one element size here.
     */
    return DL.getTypeAllocSize(
        AllocatedType);
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

/*
 * IMPORTANT:
 *
 * The implicit label is added BEFORE the branch executes.
 *
 * This means BOTH sides of:
 *
 *     if (condition) {
 *         x = 10;
 *     } else {
 *         y = 20;
 *     }
 *
 * receive the label, even though only one store executes.
 *
 * Because DFSan's normal store instrumentation may later overwrite
 * the shadow when the selected store executes, we also add the same
 * label AFTER every store in the conditional region.
 */
static void instrumentConditional(
    Module &M,
    ConditionInfo &Condition,
    ArrayRef<AllocaInst *> Targets,
    const VariableInfo &Variables) {

    if (!Condition.Branch)
        return;

    if (!Condition.Label)
        return;

    LLVMContext &Ctx =
        M.getContext();

    const DataLayout &DL =
        M.getDataLayout();

    FunctionCallee AddLabel =
        getDFSanAddLabel(M);

    FunctionCallee ReadLabel =
        getDFSanReadLabel(M);

    FunctionCallee Callback =
        getRuntimeCallback(M);

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);


    (void)PtrTy;

    Type *SizeTy =
        DL.getIntPtrType(Ctx);

    /*
     * ---------------------------------------------------------
     * STEP 1
     *
     * Add the condition label to EVERY variable participating
     * in ANY possible branch path.
     *
     * Insert directly before the branch.
     * ---------------------------------------------------------
     */
    IRBuilder<> BeforeBranch(
        Condition.Branch);

    for (AllocaInst *AI : Targets) {

        uint64_t Size =
            getAllocaSize(AI, DL);

        if (Size == 0)
            continue;

        Value *Addr =
            BeforeBranch.CreatePointerCast(
                AI,
                PointerType::get(Ctx, 0),
                "implicit.target.address");

        Value *SizeValue =
            ConstantInt::get(
                SizeTy,
                Size);

        BeforeBranch.CreateCall(
            AddLabel,
            {
                Condition.Label,
                Addr,
                SizeValue
            });
    }

    /*
     * Report ALL possible-path variables here.
     *
     * This reporting is intentionally done before the branch so
     * variables in the non-executed path are also visible.
     */
    for (AllocaInst *AI : Targets) {

        uint64_t Size =
            getAllocaSize(AI, DL);

        if (Size == 0)
            continue;

        Value *Addr =
            BeforeBranch.CreatePointerCast(
                AI,
                PointerType::get(Ctx, 0),
                "implicit.report.address");

        Value *SizeValue =
            ConstantInt::get(
                SizeTy,
                Size);

        Value *FinalLabel =
            BeforeBranch.CreateCall(
                ReadLabel,
                {
                    Addr,
                    SizeValue
                },
                "implicit.report.label");

        std::string Name =
            getVariableName(
                AI,
                Variables);

        Value *NamePtr =
            BeforeBranch.CreateGlobalString(
                Name,
                "implicit.variable.name");

        BeforeBranch.CreateCall(
            Callback,
            {
                Condition.Label,
                FinalLabel,
                ConstantInt::get(
                    I32Ty,
                    Condition.Line),
                ConstantInt::get(
                    I32Ty,
                    Condition.Column),
                NamePtr
            });
    }
}

static void restoreLabelAfterStore(
    Module &M,
    StoreInst *Store,
    const std::vector<ConditionInfo> &Conditions,
    const DenseMap<
        StoreInst *,
        SmallVector<unsigned, 4>> &Controllers) {

    auto It =
        Controllers.find(Store);

    if (It == Controllers.end())
        return;

    if (It->second.empty())
        return;

    LLVMContext &Ctx =
        M.getContext();

    const DataLayout &DL =
        M.getDataLayout();

    FunctionCallee AddLabel =
        getDFSanAddLabel(M);

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Type *SizeTy =
        DL.getIntPtrType(Ctx);

    TypeSize StoreSize =
        DL.getTypeStoreSize(
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

    IRBuilder<> Builder(
        InsertBefore);

    Value *Address =
        Builder.CreatePointerCast(
            Store->getPointerOperand(),
            PointerType::get(Ctx, 0),
            "implicit.restore.address");

    Value *SizeValue =
        ConstantInt::get(
            SizeTy,
            Size);

    /*
     * Re-add every condition label controlling this store.
     *
     * This is necessary because DFSan's normal store instrumentation
     * for x = 10 / y = 20 can write the explicit value's shadow
     * after our pre-branch propagation.
     */
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

        std::vector<ConditionInfo>
            Conditions;

        DenseMap<
            StoreInst *,
            SmallVector<unsigned, 4>>
            Controllers;

        DenseMap<
            Function *,
            VariableInfo>
            FunctionVariables;

        /*
         * ---------------------------------------------------------
         * PASS 1:
         * Find conditional branches and all variables/stores
         * contained in both possible paths.
         * ---------------------------------------------------------
         */
        for (Function &F : M) {

            if (F.isDeclaration())
                continue;

            VariableInfo Info;

            buildVariableInfo(
                F,
                Info);

            FunctionVariables[
                &F] = Info;

            PostDominatorTree PDT;

            PDT.recalculate(F);

            std::vector<BranchInst *>
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

            for (BranchInst *BR :
                 Branches) {

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

                unsigned ConditionIndex =
                    static_cast<unsigned>(
                        Conditions.size());

                Conditions.push_back(
                    {
                        BR,
                        ConditionLabel,
                        Line,
                        Column
                    });

                BasicBlock *Merge =
                    getMergeBlock(
                        BR,
                        PDT);

                if (!Merge)
                    continue;

                SmallVector<BasicBlock *, 32>
                    TrueRegion;

                SmallVector<BasicBlock *, 32>
                    FalseRegion;

                collectRegion(
                    BR->getSuccessor(0),
                    Merge,
                    TrueRegion);

                collectRegion(
                    BR->getSuccessor(1),
                    Merge,
                    FalseRegion);

                /*
                 * Every store on both possible paths is controlled
                 * by this condition.
                 */
                for (BasicBlock *BB :
                     TrueRegion) {

                    for (Instruction &I :
                         *BB) {

                        auto *Store =
                            dyn_cast<StoreInst>(&I);

                        if (!Store)
                            continue;

                        Controllers[Store]
                            .push_back(
                                ConditionIndex);
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

                        bool Exists = false;

                        for (unsigned Existing :
                             Controllers[Store]) {

                            if (Existing ==
                                ConditionIndex) {

                                Exists = true;
                                break;
                            }
                        }

                        if (!Exists) {

                            Controllers[Store]
                                .push_back(
                                    ConditionIndex);
                        }
                    }
                }
            }

            /*
             * -----------------------------------------------------
             * For every condition, collect EVERY destination
             * variable from BOTH possible paths.
             * -----------------------------------------------------
             */
            for (unsigned Index = 0;
                 Index < Conditions.size();
                 ++Index) {

                ConditionInfo &Condition =
                    Conditions[Index];

                if (!Condition.Branch)
                    continue;

                if (Condition.Branch
                        ->getFunction() != &F)
                    continue;

                BasicBlock *Merge =
                    getMergeBlock(
                        Condition.Branch,
                        PDT);

                if (!Merge)
                    continue;

                SmallVector<BasicBlock *, 32>
                    TrueRegion;

                SmallVector<BasicBlock *, 32>
                    FalseRegion;

                collectRegion(
                    Condition.Branch
                        ->getSuccessor(0),
                    Merge,
                    TrueRegion);

                collectRegion(
                    Condition.Branch
                        ->getSuccessor(1),
                    Merge,
                    FalseRegion);

                SmallVector<AllocaInst *, 32>
                    Targets;

                collectTargetsFromRegion(
                    TrueRegion,
                    Targets);

                collectTargetsFromRegion(
                    FalseRegion,
                    Targets);

                if (Targets.empty())
                    continue;

                instrumentConditional(
                    M,
                    Condition,
                    Targets,
                    Info);

                Changed = true;
            }
        }

        /*
         * ---------------------------------------------------------
         * PASS 2:
         *
         * Selected-path stores can overwrite the label with 0
         * because DFSan's normal store instrumentation sees the
         * constant "10" / "20".
         *
         * Restore the implicit label immediately after every
         * controlled store.
         * ---------------------------------------------------------
         */
        for (auto &Entry :
             Controllers) {

            StoreInst *Store =
                Entry.first;

            if (!Store)
                continue;

            restoreLabelAfterStore(
                M,
                Store,
                Conditions,
                Controllers);

            Changed = true;
        }

        /*
         * We used DFSan's original callback only as the source of
         * its SSA condition shadow.
         */
        for (ConditionInfo &Condition :
             Conditions) {

            if (!Condition.Branch)
                continue;

            CallInst *CI =
                findDFSanConditionalCallback(
                    Condition.Branch);

            if (CI && CI->getParent())
                CI->eraseFromParent();
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