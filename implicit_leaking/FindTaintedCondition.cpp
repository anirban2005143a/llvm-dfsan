#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugProgramInstruction.h"
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
    std::unordered_map<const AllocaInst *, std::string> AllocaNames;
    std::unordered_map<const Value *, std::string> DebugValueNames;
};

struct SourceVariable {
    std::string Name;
    AllocaInst *Address = nullptr;
};

static void buildVariableInfo(
    Function &F,
    VariableInfo &Info) {

    for (Instruction &I : instructions(F)) {

        /*
         * LLVM 21 debug records.
         */
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

                    Info.DebugValueNames[V] =
                        Name;

                    Value *Base =
                        V->stripPointerCasts();

                    if (auto *AI =
                            dyn_cast<AllocaInst>(Base)) {

                        Info.AllocaNames[AI] =
                            Name;
                    }
                }
            }
        }

        /*
         * Legacy dbg.declare.
         */
        if (auto *DDI =
                dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var =
                DDI->getVariable();

            Value *Addr =
                DDI->getAddress();

            if (!Var || !Addr)
                continue;

            std::string Name =
                Var->getName().str();

            Value *Base =
                Addr->stripPointerCasts();

            if (auto *AI =
                    dyn_cast<AllocaInst>(Base)) {

                Info.AllocaNames[AI] =
                    Name;
            }
        }

        /*
         * Legacy dbg.value.
         */
        if (auto *DVI =
                dyn_cast<DbgValueInst>(&I)) {

            DILocalVariable *Var =
                DVI->getVariable();

            Value *V =
                DVI->getValue();

            if (!Var || !V)
                continue;

            Info.DebugValueNames[V] =
                Var->getName().str();
        }
    }
}

static void addUniqueVariable(
    std::vector<SourceVariable> &Vars,
    const std::string &Name,
    AllocaInst *Address) {

    if (Name.empty() || !Address)
        return;

    for (const SourceVariable &V :
         Vars) {

        if (V.Name == Name &&
            V.Address == Address)
            return;
    }

    Vars.push_back(
        {Name, Address});
}

static AllocaInst *findAllocaByName(
    const VariableInfo &Info,
    const std::string &Name) {

    for (const auto &Entry :
         Info.AllocaNames) {

        if (Entry.second == Name)
            return const_cast<AllocaInst *>(
                Entry.first);
    }

    return nullptr;
}

static void collectVariables(
    Value *V,
    const VariableInfo &Info,
    std::vector<SourceVariable> &Vars,
    std::set<const Value *> &Visited) {

    if (!V)
        return;

    if (!Visited.insert(V).second)
        return;

    /*
     * Direct dbg.value mapping.
     */
    auto DbgIt =
        Info.DebugValueNames.find(V);

    if (DbgIt != Info.DebugValueNames.end()) {

        AllocaInst *AI =
            findAllocaByName(
                Info,
                DbgIt->second);

        if (AI) {
            addUniqueVariable(
                Vars,
                DbgIt->second,
                AI);
        }
    }

    /*
     * load -> alloca -> source variable
     */
    if (auto *LI =
            dyn_cast<LoadInst>(V)) {

        Value *Ptr =
            LI->getPointerOperand()
               ->stripPointerCasts();

        if (auto *AI =
                dyn_cast<AllocaInst>(Ptr)) {

            auto It =
                Info.AllocaNames.find(AI);

            if (It !=
                Info.AllocaNames.end()) {

                addUniqueVariable(
                    Vars,
                    It->second,
                    AI);

                return;
            }
        }
    }

    /*
     * alloca -> source variable
     */
    if (auto *AI =
            dyn_cast<AllocaInst>(V)) {

        auto It =
            Info.AllocaNames.find(AI);

        if (It !=
            Info.AllocaNames.end()) {

            addUniqueVariable(
                Vars,
                It->second,
                AI);

            return;
        }
    }

    /*
     * Recursively walk:
     *
     * icmp
     *   |
     *   +-- add
     *       |
     *       +-- load x
     *       +-- load y
     *       +-- load z
     */
    if (auto *I =
            dyn_cast<Instruction>(V)) {

        for (Value *Op :
             I->operands()) {

            if (isa<Constant>(Op))
                continue;

            collectVariables(
                Op,
                Info,
                Vars,
                Visited);
        }
    }
}

/*
 * DFSan creates a shadow alloca immediately before
 * an eligible application alloca.
 *
 * Example:
 *
 *   %5 = alloca i8      <- DFSan shadow
 *   %6 = alloca i32     <- source variable x
 *
 *   store i8 %label, ptr %5
 *   store i32 %value, ptr %6
 *
 * We need %5, not %6.
 */
static AllocaInst *findShadowAlloca(
    AllocaInst *ApplicationAlloca) {

    if (!ApplicationAlloca)
        return nullptr;

    Instruction *Prev =
        ApplicationAlloca->getPrevNode();

    if (!Prev)
        return nullptr;

    auto *Shadow =
        dyn_cast<AllocaInst>(Prev);

    if (!Shadow)
        return nullptr;

    Type *ShadowType =
        Shadow->getAllocatedType();

    if (!ShadowType->isIntegerTy(8))
        return nullptr;

    /*
     * It must be the alloca directly preceding
     * the application's alloca.
     */
    if (Shadow->getNextNode() !=
        ApplicationAlloca)
        return nullptr;

    return Shadow;
}

static CallInst *
findDFSanConditionalCallback(
    BranchInst *BR) {

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

        PointerType *PtrTy =
            PointerType::get(Ctx, 0);

        /*
         * void __implicit_branch_callback(
         *     dfsan_label condition_label,
         *     dfsan_label variable_label,
         *     uint32_t line,
         *     uint32_t column,
         *     const char *variable
         * );
         */
        FunctionType *CallbackTy =
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

        FunctionCallee RuntimeCallback =
            M.getOrInsertFunction(
                "__implicit_branch_callback",
                CallbackTy);

        bool Changed = false;

        for (Function &F : M) {

            if (F.isDeclaration())
                continue;

            VariableInfo Info;

            buildVariableInfo(
                F,
                Info);

            /*
             * Collect branches first because
             * callbacks are inserted later.
             */
            std::vector<BranchInst *>
                Branches;

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

            for (BranchInst *BR :
                 Branches) {

                CallInst *DFCall =
                    findDFSanConditionalCallback(
                        BR);

                if (!DFCall)
                    continue;

                Value *Condition =
                    BR->getCondition();

                /*
                 * Source location.
                 */
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

                if (Line == 0)
                    continue;

                /*
                 * Argument 0 of DFSan's callback
                 * is the complete condition shadow.
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

                /*
                 * Find all source variables used
                 * by this branch condition.
                 */
                std::vector<SourceVariable>
                    Variables;

                std::set<const Value *>
                    Visited;

                collectVariables(
                    Condition,
                    Info,
                    Variables,
                    Visited);

                if (Variables.empty())
                    continue;

                /*
                 * Insert after DFSan's callback and
                 * immediately before the branch.
                 */
                IRBuilder<> Builder(BR);

                for (const SourceVariable &V :
                     Variables) {

                    /*
                     * Locate the DFSan shadow alloca.
                     */
                    AllocaInst *ShadowAlloca =
                        findShadowAlloca(
                            V.Address);

                    if (!ShadowAlloca)
                        continue;

                    /*
                     * Read the LOCAL DFSan shadow.
                     *
                     * This is the key fix.
                     *
                     * Do NOT use:
                     *
                     * dfsan_read_label(V.Address, ...)
                     *
                     * because DFSan keeps the label
                     * of eligible stack variables in
                     * ShadowAlloca.
                     */
                    Value *VariableLabel =
                        Builder.CreateLoad(
                            I8Ty,
                            ShadowAlloca,
                            V.Name + ".dfsan.label");

                    /*
                     * Variable name.
                     */
                    Value *NamePtr =
                        Builder.CreateGlobalString(
                            V.Name);

                    Builder.CreateCall(
                        RuntimeCallback,
                        {
                            ConditionLabel,
                            VariableLabel,

                            ConstantInt::get(
                                I32Ty,
                                Line),

                            ConstantInt::get(
                                I32Ty,
                                Column),

                            NamePtr
                        });
                }

                /*
                 * We don't need the original DFSan
                 * callback anymore.
                 */
                DFCall->eraseFromParent();

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