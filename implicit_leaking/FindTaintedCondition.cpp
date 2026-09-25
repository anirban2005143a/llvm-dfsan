#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugProgramInstruction.h"
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
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>
#include <vector>

using namespace llvm;

namespace {

static std::string getDebugName(Function &F, Value *V) {
    if (!V)
        return "";

    // LLVM 21 new debug-info representation.
    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
            if (!I.hasDbgRecords())
                continue;

            for (DbgRecord &DR : I.getDbgRecordRange()) {
                auto *DVR = dyn_cast<DbgVariableRecord>(&DR);

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

                    if (Loc == V)
                        return Var->getName().str();
                }
            }
        }
    }

    // Compatibility with old debug intrinsics.
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

            if (Loc == V)
                return Var->getName().str();
        }
    }

    return "";
}

static void addUnique(
    std::vector<std::string> &Names,
    const std::string &Name) {

    if (Name.empty())
        return;

    for (const std::string &Existing : Names) {
        if (Existing == Name)
            return;
    }

    Names.push_back(Name);
}

static void collectVariables(
    Function &F,
    Value *V,
    std::vector<std::string> &Names,
    std::set<const Value *> &Visited) {

    if (!V)
        return;

    if (!Visited.insert(V).second)
        return;

    // Load -> alloca -> source variable.
    if (auto *LI = dyn_cast<LoadInst>(V)) {

        Value *Ptr =
            LI->getPointerOperand()
               ->stripPointerCasts();

        if (auto *AI = dyn_cast<AllocaInst>(Ptr)) {

            std::string Name =
                getDebugName(F, AI);

            if (!Name.empty())
                addUnique(Names, Name);

            return;
        }
    }

    // Direct SSA/debug mapping.
    std::string DirectName =
        getDebugName(F, V);

    if (!DirectName.empty())
        addUnique(Names, DirectName);

    // PHI -> incoming values.
    if (auto *PN = dyn_cast<PHINode>(V)) {

        for (Value *Incoming :
             PN->incoming_values()) {

            collectVariables(
                F,
                Incoming,
                Names,
                Visited);
        }

        return;
    }

    // Recursively follow the complete condition data-flow.
    if (auto *I = dyn_cast<Instruction>(V)) {

        for (Value *Op : I->operands()) {

            if (isa<Constant>(Op))
                continue;

            collectVariables(
                F,
                Op,
                Names,
                Visited);
        }
    }
}

static bool isDFSanConditionalCallback(
    CallInst *CI) {

    if (!CI)
        return false;

    Value *Called =
        CI->getCalledOperand()
           ->stripPointerCasts();

    Function *Callee =
        dyn_cast<Function>(Called);

    if (!Callee)
        return false;

    StringRef Name =
        Callee->getName();

    return Name ==
               "__dfsan_conditional_callback" ||
           Name ==
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

        Type *SizeTy =
            M.getDataLayout().getIntPtrType(Ctx);

        PointerType *PtrTy =
            PointerType::get(Ctx, 0);

        FunctionType *CallbackTy =
            FunctionType::get(
                VoidTy,
                {
                    I8Ty,      // DFSan condition label
                    I32Ty,     // line
                    I32Ty,     // column
                    PtrTy,     // variable name
                    PtrTy,     // variable address
                    SizeTy     // variable size
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

            /*
             * Build a snapshot because we modify the IR
             * while walking the function.
             */
            std::vector<CallInst *> DFSanCallbacks;

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

                /*
                 * DFSan inserts:

                     call void @__dfsan_conditional_callback(...)
                     br i1 %condition, ...

                 * Therefore the branch is immediately
                 * after the DFSan callback.
                 */
                Instruction *Next =
                    DFSanCB->getNextNode();

                auto *BR =
                    dyn_cast_or_null<BranchInst>(Next);

                if (!BR ||
                    !BR->isConditional())
                    continue;

                Value *Label =
                    DFSanCB->getArgOperand(0);

                if (Label->getType() != I8Ty) {

                    IRBuilder<> CastBuilder(DFSanCB);

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

                /*
                 * Keep the same source position behaviour
                 * as the working implementation.
                 */
                DebugLoc DL =
                    BR->getDebugLoc();

                if (DL) {

                    Line = DL.getLine();
                    Column = DL.getCol();

                } else if (auto *CondI =
                               dyn_cast<Instruction>(
                                   Condition)) {

                    DebugLoc CondDL =
                        CondI->getDebugLoc();

                    if (CondDL) {
                        Line = CondDL.getLine();
                        Column = CondDL.getCol();
                    }
                }

                std::vector<std::string>
                    Variables;

                std::set<const Value *>
                    Visited;

                collectVariables(
                    F,
                    Condition,
                    Variables,
                    Visited);

                if (Variables.empty())
                    continue;

                IRBuilder<> Builder(DFSanCB);

                unsigned ID = 0;

                for (const std::string &Name :
                     Variables) {

                    Value *NamePtr =
                        Builder.CreateGlobalString(
                            Name,
                            "__implicit_var_" +
                                std::to_string(ID++));

                    /*
                     * Find the alloca belonging to this
                     * source variable.
                     */
                    AllocaInst *VariableAddress =
                        nullptr;

                    std::set<const Value *>
                        SearchVisited;

                    std::vector<Value *>
                        Worklist;

                    Worklist.push_back(Condition);

                    while (!Worklist.empty()) {

                        Value *Cur =
                            Worklist.back();

                        Worklist.pop_back();

                        if (!Cur)
                            continue;

                        if (!SearchVisited.insert(Cur).second)
                            continue;

                        if (auto *LI =
                                dyn_cast<LoadInst>(Cur)) {

                            Value *Ptr =
                                LI->getPointerOperand()
                                   ->stripPointerCasts();

                            if (auto *AI =
                                    dyn_cast<AllocaInst>(Ptr)) {

                                std::string Found =
                                    getDebugName(F, AI);

                                if (Found == Name) {
                                    VariableAddress = AI;
                                    break;
                                }
                            }
                        }

                        if (auto *I =
                                dyn_cast<Instruction>(Cur)) {

                            for (Value *Op :
                                 I->operands()) {

                                if (!isa<Constant>(Op))
                                    Worklist.push_back(Op);
                            }
                        }
                    }

                    if (!VariableAddress)
                        continue;

                    uint64_t Size =
                        M.getDataLayout()
                         .getTypeStoreSize(
                             VariableAddress
                                 ->getAllocatedType())
                         .getFixedValue();

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

                            VariableAddress,

                            ConstantInt::get(
                                SizeTy,
                                Size)
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

                    if (Name == "implicit-taint") {

                        MPM.addPass(
                            ImplicitTaintPass());

                        return true;
                    }

                    return false;
                });
        }
    };
}