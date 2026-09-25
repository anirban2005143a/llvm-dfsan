#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugProgramInstruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <string>

using namespace llvm;

namespace {

static std::string getDebugName(Function &F, Value *V) {
    if (!V)
        return "";

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

        std::string Name =
            getDebugName(F, Cur);

        if (!Name.empty())
            return Name;

        if (auto *LI =
                dyn_cast<LoadInst>(Cur)) {

            Value *Ptr =
                LI->getPointerOperand()
                   ->stripPointerCasts();

            if (auto *AI =
                    dyn_cast<AllocaInst>(Ptr)) {

                Name =
                    getDebugName(F, AI);

                if (!Name.empty())
                    return Name;
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

static void collectConditionVariables(
    Function &F,
    Value *V,
    SmallVectorImpl<std::string> &Names,
    SmallPtrSetImpl<Value *> &Visited) {

    if (!V ||
        !Visited.insert(V).second)
        return;

    if (auto *LI =
            dyn_cast<LoadInst>(V)) {

        Value *Ptr =
            LI->getPointerOperand()
               ->stripPointerCasts();

        if (auto *AI =
                dyn_cast<AllocaInst>(Ptr)) {

            std::string Name =
                getDebugName(F, AI);

            if (!Name.empty())
                Names.push_back(Name);

            return;
        }
    }

    if (auto *PN =
            dyn_cast<PHINode>(V)) {

        std::string Name =
            getDebugName(F, PN);

        if (!Name.empty())
            Names.push_back(Name);
    }

    if (auto *I =
            dyn_cast<Instruction>(V)) {

        for (Value *Op : I->operands()) {

            if (isa<Constant>(Op))
                continue;

            collectConditionVariables(
                F,
                Op,
                Names,
                Visited);
        }
    }
}

static void addUnique(
    SmallVectorImpl<std::string> &Names,
    const std::string &Name) {

    if (Name.empty())
        return;

    for (const std::string &Existing : Names) {

        if (Existing == Name)
            return;
    }

    Names.push_back(Name);
}

static bool isDFSanConditionalCallback(
    CallInst *CI) {

    if (!CI)
        return false;

    Value *Called =
        CI->getCalledOperand()
           ->stripPointerCasts();

    Function *F =
        dyn_cast<Function>(Called);

    if (!F)
        return false;

    return F->getName() ==
           "__dfsan_conditional_callback";
}

struct FindTaintedConditionPass
    : public PassInfoMixin<FindTaintedConditionPass> {

    PreservedAnalyses run(
        Function &F,
        FunctionAnalysisManager &) {

        Module *M =
            F.getParent();

        LLVMContext &Ctx =
            M->getContext();

        Type *VoidTy =
            Type::getVoidTy(Ctx);

        Type *I8Ty =
            Type::getInt8Ty(Ctx);

        Type *I32Ty =
            Type::getInt32Ty(Ctx);

        Type *PtrTy =
            PointerType::get(Ctx, 0);

        Type *IntPtrTy =
            M->getDataLayout()
             .getIntPtrType(Ctx);

        FunctionType *CallbackTy =
            FunctionType::get(
                VoidTy,
                {
                    I8Ty,
                    I32Ty,
                    I32Ty,
                    PtrTy,
                    PtrTy,
                    IntPtrTy
                },
                false);

        FunctionCallee RuntimeCallback =
            M->getOrInsertFunction(
                "__implicit_branch_callback",
                CallbackTy);

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

        for (CallInst *DFSanCB :
             DFSanCallbacks) {

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

            Value *Condition =
                BI->getCondition();

            unsigned Line = 0;
            unsigned Column = 0;

            DebugLoc DL =
                BI->getDebugLoc();

            if (DL) {

                Line = DL.getLine();
                Column = DL.getCol();

            } else if (auto *CondI =
                           dyn_cast<Instruction>(
                               Condition)) {

                DebugLoc CondDL =
                    CondI->getDebugLoc();

                if (CondDL) {

                    Line =
                        CondDL.getLine();

                    Column =
                        CondDL.getCol();
                }
            }

            SmallVector<std::string, 16>
                Variables;

            SmallPtrSet<Value *, 32>
                Visited;

            collectConditionVariables(
                F,
                Condition,
                Variables,
                Visited);

            SmallVector<std::string, 16>
                UniqueVariables;

            for (const std::string &Name :
                 Variables) {

                addUnique(
                    UniqueVariables,
                    Name);
            }

            if (UniqueVariables.empty())
                continue;

            IRBuilder<> IRB(DFSanCB);

            for (const std::string &Name :
                 UniqueVariables) {

                Value *Address = nullptr;

                SmallVector<Value *, 32> Search;
                SmallPtrSet<Value *, 32> Seen;

                Search.push_back(Condition);

                while (!Search.empty()) {

                    Value *Cur =
                        Search.pop_back_val();

                    if (!Cur ||
                        !Seen.insert(Cur).second)
                        continue;

                    if (auto *LI =
                            dyn_cast<LoadInst>(Cur)) {

                        Value *Ptr =
                            LI->getPointerOperand()
                               ->stripPointerCasts();

                        if (auto *AI =
                                dyn_cast<AllocaInst>(Ptr)) {

                            std::string FoundName =
                                getDebugName(F, AI);

                            if (FoundName == Name) {
                                Address = AI;
                                break;
                            }
                        }
                    }

                    if (auto *PN =
                            dyn_cast<PHINode>(Cur)) {

                        if (getDebugName(F, PN) == Name) {

                            Address = PN;
                            break;
                        }
                    }

                    if (auto *I =
                            dyn_cast<Instruction>(Cur)) {

                        for (Value *Op :
                             I->operands()) {

                            if (!isa<Constant>(Op))
                                Search.push_back(Op);
                        }
                    }
                }

                if (!Address)
                    continue;

                uint64_t Size = 0;

                if (auto *AI =
                        dyn_cast<AllocaInst>(Address)) {

                    Size =
                        M->getDataLayout()
                         .getTypeStoreSize(
                             AI->getAllocatedType())
                         .getFixedValue();

                } else if (auto *PN =
                               dyn_cast<PHINode>(Address)) {

                    Size =
                        M->getDataLayout()
                         .getTypeStoreSize(
                             PN->getType())
                         .getFixedValue();
                }

                if (Size == 0)
                    continue;

                Value *VariableName =
                    IRB.CreateGlobalString(
                        Name,
                        "__implicit_variable_" + Name);

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

                        VariableName,

                        Address,

                        ConstantInt::get(
                            IntPtrTy,
                            Size)
                    });
            }
        }

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