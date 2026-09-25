#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <set>
#include <string>
#include <vector>

using namespace llvm;

namespace {

struct VariableInfo {
    std::string Name;
    Value *Address;
    uint64_t Size;
};

static void addUniqueVariable(
    std::vector<VariableInfo> &Variables,
    const std::string &Name,
    Value *Address,
    uint64_t Size) {

    if (Name.empty() || !Address || Size == 0)
        return;

    for (const auto &V : Variables) {
        if (V.Name == Name && V.Address == Address)
            return;
    }

    Variables.push_back({Name, Address, Size});
}

static Value *getMemoryAddress(Value *V) {
    if (!V)
        return nullptr;

    V = V->stripPointerCasts();

    if (isa<AllocaInst>(V))
        return V;

    if (isa<GlobalVariable>(V))
        return V;

    if (auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
        Value *Base =
            GEP->getPointerOperand()->stripPointerCasts();

        if (isa<AllocaInst>(Base) ||
            isa<GlobalVariable>(Base))
            return Base;
    }

    return nullptr;
}

static uint64_t getMemorySize(
    Module &M,
    Value *Address) {

    if (auto *AI = dyn_cast<AllocaInst>(Address)) {
        TypeSize Size =
            M.getDataLayout().getTypeStoreSize(
                AI->getAllocatedType());

        if (Size.isScalable())
            return 0;

        return Size.getFixedValue();
    }

    if (auto *GV = dyn_cast<GlobalVariable>(Address)) {
        TypeSize Size =
            M.getDataLayout().getTypeStoreSize(
                GV->getValueType());

        if (Size.isScalable())
            return 0;

        return Size.getFixedValue();
    }

    return 0;
}

static std::string getDbgName(
    Function &F,
    Value *V) {

    if (!V)
        return "";

    V = V->stripPointerCasts();

    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {

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

                    Value *Loc =
                        DVR->getVariableLocationOp(Op);

                    Loc = Loc->stripPointerCasts();

                    if (Loc == V)
                        return Var->getName().str();
                }
            }
        }
    }

    for (Instruction &I : instructions(F)) {

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

            Loc = Loc->stripPointerCasts();

            if (Loc == V)
                return Var->getName().str();
        }
    }

    return "";
}

static void collectVariables(
    Function &F,
    Module &M,
    Value *V,
    std::vector<VariableInfo> &Variables,
    std::set<Value *> &Visited) {

    if (!V)
        return;

    V = V->stripPointerCasts();

    if (!Visited.insert(V).second)
        return;

    if (auto *LI = dyn_cast<LoadInst>(V)) {

        Value *Address =
            getMemoryAddress(
                LI->getPointerOperand());

        if (Address) {

            std::string Name =
                getDbgName(F, Address);

            uint64_t Size =
                getMemorySize(M, Address);

            if (!Name.empty() && Size != 0) {
                addUniqueVariable(
                    Variables,
                    Name,
                    Address,
                    Size);
            }

            return;
        }
    }

    std::string DirectName =
        getDbgName(F, V);

    if (!DirectName.empty()) {

        Value *Address = nullptr;

        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {

                if (auto *AI =
                        dyn_cast<AllocaInst>(&I)) {

                    std::string Name =
                        getDbgName(F, AI);

                    if (Name == DirectName) {
                        Address = AI;
                        break;
                    }
                }
            }

            if (Address)
                break;
        }

        if (Address) {
            uint64_t Size =
                getMemorySize(M, Address);

            if (Size != 0) {
                addUniqueVariable(
                    Variables,
                    DirectName,
                    Address,
                    Size);
            }
        }
    }

    if (auto *PN = dyn_cast<PHINode>(V)) {

        for (Value *Incoming :
             PN->incoming_values()) {

            collectVariables(
                F,
                M,
                Incoming,
                Variables,
                Visited);
        }

        return;
    }

    if (auto *I = dyn_cast<Instruction>(V)) {

        for (Value *Op : I->operands()) {

            if (isa<Constant>(Op))
                continue;

            collectVariables(
                F,
                M,
                Op,
                Variables,
                Visited);
        }
    }
}

static bool isConditionalCallback(
    CallInst *CI) {

    if (!CI)
        return false;

    Value *Called =
        CI->getCalledOperand()
            ->stripPointerCasts();

    auto *F =
        dyn_cast<Function>(Called);

    if (!F)
        return false;

    return F->getName() ==
               "__dfsan_conditional_callback" ||
           F->getName() ==
               "__dfsan_conditional_callback_origin";
}

class ImplicitTaintPass
    : public PassInfoMixin<ImplicitTaintPass> {

public:

    PreservedAnalyses run(
        Module &M,
        ModuleAnalysisManager &) {

        LLVMContext &Ctx =
            M.getContext();

        Type *VoidTy =
            Type::getVoidTy(Ctx);

        Type *I8Ty =
            Type::getInt8Ty(Ctx);

        Type *I32Ty =
            Type::getInt32Ty(Ctx);

        Type *IntPtrTy =
            M.getDataLayout()
                .getIntPtrType(Ctx);

        PointerType *PtrTy =
            PointerType::get(Ctx, 0);

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

        FunctionCallee Callback =
            M.getOrInsertFunction(
                "__implicit_branch_callback",
                CallbackTy);

        bool Changed = false;

        for (Function &F : M) {

            if (F.isDeclaration())
                continue;

            std::vector<CallInst *>
                ConditionalCallbacks;

            for (BasicBlock &BB : F) {

                for (Instruction &I : BB) {

                    auto *CI =
                        dyn_cast<CallInst>(&I);

                    if (isConditionalCallback(CI))
                        ConditionalCallbacks.push_back(CI);
                }
            }

            for (CallInst *DFSanCB :
                 ConditionalCallbacks) {

                auto *BR =
                    dyn_cast_or_null<BranchInst>(
                        DFSanCB->getNextNode());

                if (!BR || !BR->isConditional())
                    continue;

                DebugLoc DL =
                    BR->getDebugLoc();

                if (!DL)
                    continue;

                unsigned Line =
                    DL.getLine();

                unsigned Column =
                    DL.getCol();

                if (Line == 0)
                    continue;

                Value *Condition =
                    BR->getCondition();

                Value *Label =
                    DFSanCB->getArgOperand(0);

                if (Label->getType() != I8Ty) {

                    IRBuilder<> B(DFSanCB);

                    Label =
                        B.CreateIntCast(
                            Label,
                            I8Ty,
                            false);
                }

                std::vector<VariableInfo>
                    Variables;

                std::set<Value *> Visited;

                collectVariables(
                    F,
                    M,
                    Condition,
                    Variables,
                    Visited);

                if (Variables.empty())
                    continue;

                IRBuilder<> B(BR);

                for (const auto &V : Variables) {

                    Value *NamePtr =
                        B.CreateGlobalString(
                            V.Name);

                    B.CreateCall(
                        Callback,
                        {
                            Label,

                            ConstantInt::get(
                                I32Ty,
                                Line),

                            ConstantInt::get(
                                I32Ty,
                                Column),

                            NamePtr,

                            V.Address,

                            ConstantInt::get(
                                IntPtrTy,
                                V.Size)
                        });
                }

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