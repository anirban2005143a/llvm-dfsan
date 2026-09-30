#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfoMetadata.h"
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
    unsigned ID = 0;
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

static void buildVariableInfo(Function &F, VariableInfo &Info) {
    for (Instruction &I : instructions(F)) {

        if (I.hasDbgRecords()) {
            for (DbgRecord &DR : I.getDbgRecordRange()) {

                auto *DVR = dyn_cast<DbgVariableRecord>(&DR);

                if (!DVR)
                    continue;

                DILocalVariable *Var = DVR->getVariable();

                if (!Var)
                    continue;

                std::string Name = Var->getName().str();

                for (unsigned Op = 0;
                     Op < DVR->getNumVariableLocationOps();
                     ++Op) {

                    Value *V = DVR->getVariableLocationOp(Op);

                    if (!V)
                        continue;

                    Value *Base = getBasePointer(V);

                    if (auto *AI =
                            dyn_cast_or_null<AllocaInst>(Base)) {
                        Info.Names[AI] = Name;
                    }
                }
            }
        }

        if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var = DDI->getVariable();
            Value *Addr = DDI->getAddress();

            if (!Var || !Addr)
                continue;

            Value *Base = getBasePointer(Addr);

            if (auto *AI =
                    dyn_cast_or_null<AllocaInst>(Base)) {
                Info.Names[AI] = Var->getName().str();
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

static CallInst *findDFSanConditionalCallback(BranchInst *BR) {
    if (!BR)
        return nullptr;

    Instruction *Cur = BR->getPrevNode();

    for (unsigned I = 0; Cur && I < 32; ++I) {

        auto *CI = dyn_cast<CallInst>(Cur);

        if (CI) {

            Function *Callee = CI->getCalledFunction();

            if (Callee) {

                StringRef Name = Callee->getName();

                if (Name == "__dfsan_conditional_callback" ||
                    Name == "__dfsan_conditional_callback_origin") {

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

    auto *Node = PDT.getNode(BR->getParent());

    if (!Node)
        return nullptr;

    auto *IDom = Node->getIDom();

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

        BasicBlock *BB = Worklist.pop_back_val();

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

static FunctionCallee getImplicitEnter(Module &M) {
    LLVMContext &Ctx = M.getContext();

    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I8Ty = Type::getInt8Ty(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);

    FunctionType *Ty =
        FunctionType::get(
            VoidTy,
            {
                I32Ty,
                I8Ty
            },
            false);

    return M.getOrInsertFunction(
        "__implicit_enter",
        Ty);
}

static FunctionCallee getImplicitLeave(Module &M) {
    LLVMContext &Ctx = M.getContext();

    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);

    FunctionType *Ty =
        FunctionType::get(
            VoidTy,
            {
                I32Ty
            },
            false);

    return M.getOrInsertFunction(
        "__implicit_leave",
        Ty);
}

static FunctionCallee getImplicitStoreCallback(Module &M) {
    LLVMContext &Ctx = M.getContext();

    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I8Ty = Type::getInt8Ty(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);
    PointerType *PtrTy = PointerType::get(Ctx, 0);
    Type *SizeTy = M.getDataLayout().getIntPtrType(Ctx);

    FunctionType *Ty =
        FunctionType::get(
            VoidTy,
            {
                PtrTy,
                SizeTy,
                I32Ty,
                I32Ty,
                PtrTy
            },
            false);

    return M.getOrInsertFunction(
        "__implicit_store_callback",
        Ty);
}

static void addControlledStore(
    DenseSet<StoreInst *> &ControlledStores,
    StoreInst *Store) {

    if (!Store)
        return;

    Value *Base =
        getBasePointer(Store->getPointerOperand());

    if (!dyn_cast_or_null<AllocaInst>(Base))
        return;

    ControlledStores.insert(Store);
}

static void collectControlledStores(
    BranchInst *BR,
    BasicBlock *Merge,
    DenseSet<StoreInst *> &ControlledStores) {

    if (!BR || !Merge)
        return;

    SmallVector<BasicBlock *, 32> TrueRegion;
    SmallVector<BasicBlock *, 32> FalseRegion;

    collectRegion(
        BR->getSuccessor(0),
        Merge,
        TrueRegion);

    collectRegion(
        BR->getSuccessor(1),
        Merge,
        FalseRegion);

    for (BasicBlock *BB : TrueRegion) {
        for (Instruction &I : *BB) {

            auto *Store =
                dyn_cast<StoreInst>(&I);

            if (!Store)
                continue;

            addControlledStore(
                ControlledStores,
                Store);
        }
    }

    for (BasicBlock *BB : FalseRegion) {
        for (Instruction &I : *BB) {

            auto *Store =
                dyn_cast<StoreInst>(&I);

            if (!Store)
                continue;

            addControlledStore(
                ControlledStores,
                Store);
        }
    }
}

static void instrumentRegionEntry(
    BasicBlock *BB,
    ConditionInfo &Condition,
    FunctionCallee Enter) {

    if (!BB)
        return;

    if (BB == Condition.Branch->getSuccessor(0) &&
        BB == Condition.Branch->getSuccessor(1))
        return;

    Instruction *InsertPoint =
        &*BB->getFirstInsertionPt();

    IRBuilder<> Builder(InsertPoint);

    LLVMContext &Ctx =
        BB->getContext();

    Type *I8Ty =
        Type::getInt8Ty(Ctx);

    Value *Label =
        Condition.Label;

    if (Label->getType() != I8Ty) {
        Label =
            Builder.CreateIntCast(
                Label,
                I8Ty,
                false,
                "implicit.condition.label");
    }

    Builder.CreateCall(
        Enter,
        {
            ConstantInt::get(
                Type::getInt32Ty(Ctx),
                Condition.ID),

            Label
        });
}

static void instrumentMergeExit(
    BasicBlock *Merge,
    ConditionInfo &Condition,
    FunctionCallee Leave) {

    if (!Merge)
        return;

    Instruction *InsertPoint =
        &*Merge->getFirstInsertionPt();

    IRBuilder<> Builder(InsertPoint);

    LLVMContext &Ctx =
        Merge->getContext();

    Builder.CreateCall(
        Leave,
        {
            ConstantInt::get(
                Type::getInt32Ty(Ctx),
                Condition.ID)
        });
}

static void instrumentStore(
    Module &M,
    StoreInst *Store,
    const VariableInfo &Variables,
    FunctionCallee StoreCallback) {

    if (!Store)
        return;

    Value *Base =
        getBasePointer(Store->getPointerOperand());

    auto *AI =
        dyn_cast_or_null<AllocaInst>(Base);

    if (!AI)
        return;

    LLVMContext &Ctx =
        M.getContext();

    const DataLayout &DL =
        M.getDataLayout();

    TypeSize StoreSize =
        DL.getTypeStoreSize(
            Store->getValueOperand()->getType());

    if (StoreSize.isScalable())
        return;

    uint64_t Size =
        StoreSize.getFixedValue();

    if (Size == 0)
        return;

    Instruction *InsertPoint =
        Store->getNextNode();

    if (!InsertPoint)
        return;

    IRBuilder<> Builder(InsertPoint);

    PointerType *PtrTy =
        PointerType::get(Ctx, 0);

    Type *I32Ty =
        Type::getInt32Ty(Ctx);

    Type *SizeTy =
        DL.getIntPtrType(Ctx);

    Value *Address =
        Builder.CreatePointerCast(
            Store->getPointerOperand(),
            PtrTy,
            "implicit.store.address");

    Value *SizeValue =
        ConstantInt::get(
            SizeTy,
            Size);

    unsigned Line = 0;
    unsigned Column = 0;

    DebugLoc DLInfo =
        Store->getDebugLoc();

    if (DLInfo) {

        Line =
            DLInfo.getLine();

        Column =
            DLInfo.getCol();

    } else if (auto *CondI =
                   dyn_cast<Instruction>(
                       Store->getValueOperand())) {

        DebugLoc ValueDL =
            CondI->getDebugLoc();

        if (ValueDL) {

            Line =
                ValueDL.getLine();

            Column =
                ValueDL.getCol();
        }
    }

    std::string Name =
        getVariableName(
            AI,
            Variables);

    Value *NamePtr =
        Builder.CreateGlobalString(
            Name,
            "implicit.variable.name");

    Builder.CreateCall(
        StoreCallback,
        {
            Address,
            SizeValue,
            ConstantInt::get(
                I32Ty,
                Line),
            ConstantInt::get(
                I32Ty,
                Column),
            NamePtr
        });
}

class ImplicitTaintPass
    : public PassInfoMixin<ImplicitTaintPass> {

public:

    PreservedAnalyses run(
        Module &M,
        ModuleAnalysisManager &) {

        bool Changed = false;

        DenseSet<StoreInst *>
            ControlledStores;

        DenseMap<
            Function *,
            VariableInfo>
            FunctionVariables;

        std::vector<ConditionInfo>
            Conditions;

        FunctionCallee Enter =
            getImplicitEnter(M);

        FunctionCallee Leave =
            getImplicitLeave(M);

        FunctionCallee StoreCallback =
            getImplicitStoreCallback(M);

        unsigned NextConditionID = 0;

        /*
         * ---------------------------------------------------------
         * PASS 1
         *
         * Find conditional branches and their DFSan condition labels.
         * Each branch receives a stable runtime ID.
         *
         * Unlike the old implementation:
         *
         *   condition -> label every variable in both regions
         *
         * we now only create runtime branch contexts.
         *
         * Runtime decides which branch was actually entered.
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

                /*
                 * If both successors are identical,
                 * there is no actual control split.
                 */
                if (BR->getSuccessor(0) ==
                    BR->getSuccessor(1))
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

                ConditionInfo Condition;

                Condition.Branch =
                    BR;

                Condition.Label =
                    ConditionLabel;

                Condition.Line =
                    Line;

                Condition.Column =
                    Column;

                Condition.ID =
                    NextConditionID++;

                Conditions.push_back(
                    Condition);

                BasicBlock *Merge =
                    getMergeBlock(
                        BR,
                        PDT);

                if (!Merge)
                    continue;

                collectControlledStores(
                    BR,
                    Merge,
                    ControlledStores);

                /*
                 * Runtime branch context is activated ONLY when
                 * execution enters that successor.
                 *
                 * Therefore a variable in an unexecuted branch
                 * is not tainted during this execution.
                 */
                instrumentRegionEntry(
                    BR->getSuccessor(0),
                    Conditions.back(),
                    Enter);

                instrumentRegionEntry(
                    BR->getSuccessor(1),
                    Conditions.back(),
                    Enter);

                /*
                 * At the post-dominator, this control context
                 * no longer influences subsequent stores.
                 */
                instrumentMergeExit(
                    Merge,
                    Conditions.back(),
                    Leave);

                Changed = true;
            }

            /*
             * Instrument only application stores whose destination
             * is a local alloca and which are inside a conditional
             * region.
             */
            for (StoreInst *Store :
                 ControlledStores) {

                if (!Store)
                    continue;

                if (Store->getFunction() != &F)
                    continue;

                instrumentStore(
                    M,
                    Store,
                    Info,
                    StoreCallback);

                Changed = true;
            }

            ControlledStores.clear();
        }

        /*
         * Remove DFSan's original conditional callback calls.
         *
         * We already extracted their SSA label and use it in
         * __implicit_enter().
         */
        for (ConditionInfo &Condition :
             Conditions) {

            if (!Condition.Branch)
                continue;

            CallInst *CI =
                findDFSanConditionalCallback(
                    Condition.Branch);

            if (CI &&
                CI->getParent()) {

                CI->eraseFromParent();
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