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

static void buildVariableInfo(
    Function &F,
    VariableInfo &Info) {

    for (Instruction &I : instructions(F)) {

        // LLVM 21 new debug-info records.
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

        // Old dbg.declare.
        if (auto *DDI =
                dyn_cast<DbgDeclareInst>(&I)) {

            DILocalVariable *Var =
                DDI->getVariable();

            Value *Addr =
                DDI->getAddress();

            if (Var && Addr) {

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
        }

        // Old dbg.value.
        if (auto *DVI =
                dyn_cast<DbgValueInst>(&I)) {

            DILocalVariable *Var =
                DVI->getVariable();

            Value *V =
                DVI->getValue();

            if (Var && V) {

                Info.DebugValueNames[V] =
                    Var->getName().str();
            }
        }
    }
}

static void addUnique(
    std::vector<std::string> &Vars,
    const std::string &Name) {

    if (Name.empty())
        return;

    for (const std::string &V : Vars) {
        if (V == Name)
            return;
    }

    Vars.push_back(Name);
}

static void collectVariables(
    Value *V,
    const VariableInfo &Info,
    std::vector<std::string> &Vars,
    std::set<const Value *> &Visited) {

    if (!V)
        return;

    if (!Visited.insert(V).second)
        return;

    // Direct debug-value mapping.
    auto DbgIt =
        Info.DebugValueNames.find(V);

    if (DbgIt != Info.DebugValueNames.end())
        addUnique(Vars, DbgIt->second);

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

            if (It != Info.AllocaNames.end()) {

                addUnique(
                    Vars,
                    It->second);

                return;
            }

            // Sometimes LLVM attaches the debug name
            // directly to the load rather than the alloca.
            auto LoadDbg =
                Info.DebugValueNames.find(LI);

            if (LoadDbg !=
                Info.DebugValueNames.end()) {

                addUnique(
                    Vars,
                    LoadDbg->second);

                return;
            }
        }
    }

    // alloca -> source variable
    if (auto *AI =
            dyn_cast<AllocaInst>(V)) {

        auto It =
            Info.AllocaNames.find(AI);

        if (It != Info.AllocaNames.end()) {

            addUnique(
                Vars,
                It->second);

            return;
        }
    }

    /*
     * Recursively walk the complete data-flow:
     *
     * icmp
     *   -> add
     *      -> load x
     *      -> load y
     *      -> load z
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

static AllocaInst *findVariableAddress(
    Value *V,
    const VariableInfo &Info,
    const std::string &Target,
    std::set<const Value *> &Visited) {

    if (!V)
        return nullptr;

    if (!Visited.insert(V).second)
        return nullptr;

    /*
     * load -> alloca
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

            if (It != Info.AllocaNames.end() &&
                It->second == Target) {

                return AI;
            }
        }
    }

    /*
     * alloca
     */
    if (auto *AI =
            dyn_cast<AllocaInst>(V)) {

        auto It =
            Info.AllocaNames.find(AI);

        if (It != Info.AllocaNames.end() &&
            It->second == Target) {

            return AI;
        }
    }

    if (auto *I =
            dyn_cast<Instruction>(V)) {

        for (Value *Op :
             I->operands()) {

            if (isa<Constant>(Op))
                continue;

            if (AllocaInst *AI =
                    findVariableAddress(
                        Op,
                        Info,
                        Target,
                        Visited)) {

                return AI;
            }
        }
    }

    return nullptr;
}

static CallInst *
findDFSanConditionalCallback(
    BranchInst *BR) {

    Instruction *Cur =
        BR->getPrevNode();

    for (unsigned I = 0;
         Cur && I < 16;
         ++I) {

        if (auto *CI =
                dyn_cast<CallInst>(Cur)) {

            Function *Callee =
                CI->getCalledFunction();

            if (!Callee)
                continue;

            StringRef Name =
                Callee->getName();

            if (Name ==
                    "__dfsan_conditional_callback" ||
                Name ==
                    "__dfsan_conditional_callback_origin") {

                return CI;
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

        /*
         * void __implicit_branch_callback(
         *     dfsan_label,
         *     line,
         *     column,
         *     variable_name,
         *     variable_address,
         *     variable_size
         * );
         */
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

            VariableInfo Info;

            buildVariableInfo(
                F,
                Info);

            for (BasicBlock &BB : F) {

                for (Instruction &I : BB) {

                    auto *BR =
                        dyn_cast<BranchInst>(&I);

                    if (!BR ||
                        !BR->isConditional())
                        continue;

                    CallInst *DFCall =
                        findDFSanConditionalCallback(
                            BR);

                    if (!DFCall)
                        continue;

                    /*
                     * DFSan passes the condition label
                     * as argument 0.
                     */
                    Value *Label =
                        DFCall->getArgOperand(0);

                    if (Label->getType() != I8Ty) {

                        IRBuilder<> CastBuilder(
                            DFCall);

                        Label =
                            CastBuilder.CreateIntCast(
                                Label,
                                I8Ty,
                                false);
                    }

                    Value *Condition =
                        BR->getCondition();

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

                    /*
                     * Collect source variables which
                     * participate in the condition.
                     */
                    std::vector<std::string>
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

                    IRBuilder<> Builder(DFCall);

                    /*
                     * One runtime callback per candidate.
                     * Runtime checks whether its DFSan label
                     * is actually contained in the condition label.
                     */
                    for (const std::string &Name :
                         Variables) {

                        std::set<const Value *>
                            AddressVisited;

                        AllocaInst *Address =
                            findVariableAddress(
                                Condition,
                                Info,
                                Name,
                                AddressVisited);

                        if (!Address)
                            continue;

                        uint64_t Size =
                            M.getDataLayout()
                             .getTypeStoreSize(
                                 Address
                                     ->getAllocatedType())
                             .getFixedValue();

                        Value *NamePtr =
                            Builder.CreateGlobalString(
                                Name,
                                "__implicit_var_" +
                                    Name);

                        Builder.CreateCall(
                            RuntimeCallback,
                            {
                                Label,

                                ConstantInt::get(
                                    I32Ty,
                                    Line),

                                ConstantInt::get(
                                    I32Ty,
                                    Column),

                                NamePtr,

                                Address,

                                ConstantInt::get(
                                    I64Ty,
                                    Size)
                            });
                    }

                    /*
                     * Replace DFSan's normal conditional
                     * callback with our reporting callback.
                     */
                    DFCall->eraseFromParent();

                    Changed = true;
                }
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