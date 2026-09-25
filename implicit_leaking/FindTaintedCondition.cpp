#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace {

struct VariableInfo {
    AllocaInst *Address = nullptr;
    std::string Name;
    uint64_t Size = 0;
};

static void addUnique(
    std::vector<VariableInfo> &Vars,
    AllocaInst *Address,
    const std::string &Name,
    uint64_t Size) {

    if (!Address || Name.empty() || Size == 0)
        return;

    for (const auto &V : Vars) {
        if (V.Address == Address)
            return;
    }

    Vars.push_back({Address, Name, Size});
}

static void collectDebugVariables(
    Function &F,
    std::unordered_map<AllocaInst *, std::string> &Names) {

    for (Instruction &I : instructions(F)) {

        if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var = DDI->getVariable();
            Value *Addr = DDI->getAddress();

            if (!Var || !Addr)
                continue;

            Addr = Addr->stripPointerCasts();

            if (auto *AI = dyn_cast<AllocaInst>(Addr))
                Names[AI] = Var->getName().str();
        }

        if (!I.hasDbgRecords())
            continue;

        for (DbgRecord &DR : I.getDbgRecordRange()) {

            auto *DVR =
                dyn_cast<DbgVariableRecord>(&DR);

            if (!DVR)
                continue;

            DILocalVariable *Var =
                DVR->getVariable();

            if (!Var)
                continue;

            const std::string Name =
                Var->getName().str();

            for (unsigned Op = 0;
                 Op < DVR->getNumVariableLocationOps();
                 ++Op) {

                Value *V =
                    DVR->getVariableLocationOp(Op);

                if (!V)
                    continue;

                V = V->stripPointerCasts();

                if (auto *AI =
                        dyn_cast<AllocaInst>(V)) {

                    Names[AI] = Name;
                }
            }
        }
    }
}

static void collectConditionVariables(
    Value *V,
    const std::unordered_map<
        AllocaInst *,
        std::string> &Names,
    Module &M,
    std::vector<VariableInfo> &Vars,
    std::set<const Value *> &Visited) {

    if (!V)
        return;

    if (!Visited.insert(V).second)
        return;

    /*
     * load -> alloca -> source variable
     */
    if (auto *LI = dyn_cast<LoadInst>(V)) {

        Value *Ptr =
            LI->getPointerOperand()
               ->stripPointerCasts();

        if (auto *AI = dyn_cast<AllocaInst>(Ptr)) {

            auto It = Names.find(AI);

            if (It != Names.end()) {

                TypeSize Size =
                    M.getDataLayout()
                     .getTypeStoreSize(
                         AI->getAllocatedType());

                if (!Size.isScalable()) {

                    addUnique(
                        Vars,
                        AI,
                        It->second,
                        Size.getFixedValue());
                }
            }
        }
    }

    /*
     * alloca directly
     */
    if (auto *AI = dyn_cast<AllocaInst>(V)) {

        auto It = Names.find(AI);

        if (It != Names.end()) {

            TypeSize Size =
                M.getDataLayout()
                 .getTypeStoreSize(
                     AI->getAllocatedType());

            if (!Size.isScalable()) {

                addUnique(
                    Vars,
                    AI,
                    It->second,
                    Size.getFixedValue());
            }
        }
    }

    /*
     * Recursively traverse the complete
     * data-flow graph.
     */
    if (auto *I = dyn_cast<Instruction>(V)) {

        for (Value *Op : I->operands()) {

            if (isa<Constant>(Op))
                continue;

            collectConditionVariables(
                Op,
                Names,
                M,
                Vars,
                Visited);
        }
    }
}

static CallInst *findDFSanCallback(
    BranchInst *BR) {

    Instruction *Cur =
        BR->getPrevNode();

    for (unsigned I = 0;
         Cur && I < 32;
         ++I) {

        if (auto *CI =
                dyn_cast<CallInst>(Cur)) {

            Value *Called =
                CI->getCalledOperand()
                   ->stripPointerCasts();

            if (auto *F =
                    dyn_cast<Function>(Called)) {

                StringRef Name =
                    F->getName();

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

        Type *I64Ty =
            Type::getInt64Ty(Ctx);

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
                    I64Ty
                },
                false);

        FunctionCallee RuntimeCallback =
            M.getOrInsertFunction(
                "__implicit_branch_callback",
                CallbackTy);

        bool Changed = false;

        for (Function &F : M) {

            if (F.isDeclaration())
                continue;

            std::unordered_map<
                AllocaInst *,
                std::string> Names;

            collectDebugVariables(
                F,
                Names);

            /*
             * First collect all conditional branches.
             * This avoids iterator invalidation while
             * inserting callbacks.
             */
            std::vector<BranchInst *> Branches;

            for (BasicBlock &BB : F) {

                for (Instruction &I : BB) {

                    auto *BR =
                        dyn_cast<BranchInst>(&I);

                    if (!BR)
                        continue;

                    if (!BR->isConditional())
                        continue;

                    Branches.push_back(BR);
                }
            }

            for (BranchInst *BR : Branches) {

                CallInst *DFCall =
                    findDFSanCallback(BR);

                if (!DFCall)
                    continue;

                Value *Condition =
                    BR->getCondition();

                DebugLoc DL =
                    BR->getDebugLoc();

                if (!DL) {

                    if (auto *CondI =
                            dyn_cast<Instruction>(
                                Condition)) {

                        DL =
                            CondI->getDebugLoc();
                    }
                }

                if (!DL)
                    continue;

                unsigned Line =
                    DL.getLine();

                unsigned Column =
                    DL.getCol();

                if (Line == 0)
                    continue;

                /*
                 * DFSan's conditional callback
                 * argument 0 is the condition label.
                 */
                Value *ConditionLabel =
                    DFCall->getArgOperand(0);

                if (ConditionLabel->getType() !=
                    I8Ty) {

                    IRBuilder<> CastBuilder(
                        DFCall);

                    ConditionLabel =
                        CastBuilder.CreateIntCast(
                            ConditionLabel,
                            I8Ty,
                            false);
                }

                std::vector<VariableInfo>
                    Variables;

                std::set<const Value *>
                    Visited;

                collectConditionVariables(
                    Condition,
                    Names,
                    M,
                    Variables,
                    Visited);

                if (Variables.empty())
                    continue;

                /*
                 * Insert AFTER the DFSan callback,
                 * immediately before the branch.
                 */
                IRBuilder<> Builder(BR);

                for (const auto &V : Variables) {

                    Value *NamePtr =
                        Builder.CreateGlobalString(
                            V.Name);

                    Builder.CreateCall(
                        RuntimeCallback,
                        {
                            ConditionLabel,

                            ConstantInt::get(
                                I32Ty,
                                Line),

                            ConstantInt::get(
                                I32Ty,
                                Column),

                            NamePtr,

                            V.Address,

                            ConstantInt::get(
                                I64Ty,
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