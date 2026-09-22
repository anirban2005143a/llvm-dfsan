#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/User.h"

#include <fstream>
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

static void buildVariableInfo(Function &F, VariableInfo &Info) {
  for (Instruction &I : instructions(F)) {
    if (auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {
      if (auto *Var = DDI->getVariable()) {
        Value *Addr = DDI->getAddress();
        if (Addr) {
          Value *Base = Addr->stripPointerCasts();
          if (auto *AI = dyn_cast<AllocaInst>(Base)) {
            Info.AllocaNames[AI] = Var->getName().str();
          }
        }
      }
    }

    if (auto *DVI = dyn_cast<DbgValueInst>(&I)) {
      if (auto *Var = DVI->getVariable()) {
        Value *V = DVI->getValue();
        if (V)
          Info.DebugValueNames[V] = Var->getName().str();
      }
    }
  }
}

static void addUnique(std::vector<std::string> &Vars, const std::string &Name) {
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

  auto DbgIt = Info.DebugValueNames.find(V);
  if (DbgIt != Info.DebugValueNames.end())
    addUnique(Vars, DbgIt->second);

  if (auto *LI = dyn_cast<LoadInst>(V)) {
    Value *Ptr = LI->getPointerOperand()->stripPointerCasts();

    if (auto *AI = dyn_cast<AllocaInst>(Ptr)) {
      auto It = Info.AllocaNames.find(AI);
      if (It != Info.AllocaNames.end())
        addUnique(Vars, It->second);
    }
  }

  if (auto *AI = dyn_cast<AllocaInst>(V)) {
    auto It = Info.AllocaNames.find(AI);
    if (It != Info.AllocaNames.end())
      addUnique(Vars, It->second);

    return;
  }

  if (auto *U = dyn_cast<User>(V)) {
      for (Use &Op : U->operands()) {
          collectVariables(Op.get(), Info, Vars, Visited);
      }
  }
}

static std::string getValueName(Value *V) {
  if (!V)
    return "";

  std::string S;
  raw_string_ostream OS(S);
  V->printAsOperand(OS, false);
  return OS.str();
}

static std::string getBlockName(BasicBlock *BB) {
  if (!BB)
    return "";

  std::string S;
  raw_string_ostream OS(S);
  BB->printAsOperand(OS, false);
  return OS.str();
}

static std::string readSourceLine(const DILocation *Loc) {
  if (!Loc)
    return "";

  std::string File = Loc->getFilename().str();
  std::string Directory = Loc->getDirectory().str();

  if (File.empty())
    return "";

  if (!File.empty() && File[0] != '/' && !Directory.empty())
    File = Directory + "/" + File;

  std::ifstream In(File);
  if (!In)
    return "";

  unsigned TargetLine = Loc->getLine();

  std::string Line;
  unsigned Current = 1;

  while (std::getline(In, Line)) {
    if (Current == TargetLine)
      return Line;

    ++Current;
  }

  return "";
}

static std::string trim(const std::string &S) {
  const char *WS = " \t\r\n";

  size_t Begin = S.find_first_not_of(WS);
  if (Begin == std::string::npos)
    return "";

  size_t End = S.find_last_not_of(WS);

  return S.substr(Begin, End - Begin + 1);
}

static std::string extractConditionText(const std::string &Line) {
  if (Line.empty())
    return "";

  size_t KeywordPos = std::string::npos;
  size_t ParenPos = std::string::npos;

  const char *Keywords[] = {"if", "while", "for", "switch"};

  for (const char *KW : Keywords) {
    size_t P = Line.find(KW);

    if (P == std::string::npos)
      continue;

    size_t LP = Line.find('(', P);

    if (LP != std::string::npos) {
      KeywordPos = P;
      ParenPos = LP;
      break;
    }
  }

  if (ParenPos == std::string::npos)
    return trim(Line);

  int Depth = 0;

  for (size_t I = ParenPos; I < Line.size(); ++I) {
    if (Line[I] == '(')
      ++Depth;

    if (Line[I] == ')') {
      --Depth;

      if (Depth == 0) {
        return trim(Line.substr(ParenPos + 1, I - ParenPos - 1));
      }
    }
  }

  return trim(Line);
}

static std::string joinVariables(const std::vector<std::string> &Vars) {
  std::string Result;

  for (size_t I = 0; I < Vars.size(); ++I) {
    if (I)
      Result += ", ";

    Result += Vars[I];
  }

  return Result;
}

static CallInst *findDFSanConditionalCallback(BranchInst *BR) {
  Instruction *Cur = BR->getPrevNode();

  for (unsigned I = 0; Cur && I < 16; ++I) {
    if (auto *CI = dyn_cast<CallInst>(Cur)) {
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

class ImplicitTaintPass : public PassInfoMixin<ImplicitTaintPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &Ctx = M.getContext();

    unsigned ConditionID = 1;
    bool Changed = false;

    Type *VoidTy = Type::getVoidTy(Ctx);
    Type *I8Ty = Type::getInt8Ty(Ctx);
    Type *I32Ty = Type::getInt32Ty(Ctx);
    PointerType *I8PtrTy = PointerType::get(Ctx, 0);

    std::vector<Type *> CallbackArgs;

    CallbackArgs.push_back(I8Ty);      // dfsan_label
    CallbackArgs.push_back(I32Ty);     // condition id
    CallbackArgs.push_back(I8Ty);      // taken
    CallbackArgs.push_back(I8PtrTy);   // file
    CallbackArgs.push_back(I32Ty);     // line
    CallbackArgs.push_back(I32Ty);     // column
    CallbackArgs.push_back(I8PtrTy);   // source condition
    CallbackArgs.push_back(I8PtrTy);   // llvm condition
    CallbackArgs.push_back(I8PtrTy);   // tainted variables
    CallbackArgs.push_back(I8PtrTy);   // true block
    CallbackArgs.push_back(I8PtrTy);   // false block

    FunctionType *CallbackTy =
        FunctionType::get(VoidTy, CallbackArgs, false);

    FunctionCallee CustomCallback =
        M.getOrInsertFunction("__implicit_conditional_callback", CallbackTy);

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      VariableInfo VarInfo;
      buildVariableInfo(F, VarInfo);

      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          auto *BR = dyn_cast<BranchInst>(&I);

          if (!BR || !BR->isConditional())
            continue;

          CallInst *DFCall = findDFSanConditionalCallback(BR);

          if (!DFCall)
            continue;

          Value *Label = DFCall->getArgOperand(0);

          Value *Condition = BR->getCondition();

          IRBuilder<> Builder(DFCall);

          Value *Taken =
              Builder.CreateZExt(
                  Condition,
                  I8Ty,
                  "implicit.taken");

          std::string LLVMCondition =
              getValueName(Condition);

          std::vector<std::string> Variables;

          std::set<const Value *> Visited;

          collectVariables(
              Condition,
              VarInfo,
              Variables,
              Visited);

          std::string VariableString =
              joinVariables(Variables);

          const DILocation *Loc =
              BR->getDebugLoc().get();

          std::string File;
          unsigned Line = 0;
          unsigned Column = 0;

          if (Loc) {
            File = Loc->getFilename().str();
            Line = Loc->getLine();
            Column = Loc->getColumn();
          }

          std::string SourceLine =
              readSourceLine(Loc);

          std::string ConditionText =
              extractConditionText(SourceLine);

          if (ConditionText.empty())
            ConditionText = SourceLine;

          std::string TrueBlock =
              getBlockName(BR->getSuccessor(0));

          std::string FalseBlock =
              getBlockName(BR->getSuccessor(1));

          Value *ConditionIDValue =
              ConstantInt::get(I32Ty, ConditionID);

          Value *FileValue =
              Builder.CreateGlobalString(
                  File,
                  "implicit.file." + std::to_string(ConditionID));

          Value *ConditionValue =
              Builder.CreateGlobalString(
                  ConditionText,
                  "implicit.condition." +
                      std::to_string(ConditionID));

          Value *LLVMConditionValue =
              Builder.CreateGlobalString(
                  LLVMCondition,
                  "implicit.llvmcond." +
                      std::to_string(ConditionID));

          Value *VariablesValue =
              Builder.CreateGlobalString(
                  VariableString,
                  "implicit.vars." +
                      std::to_string(ConditionID));

          Value *TrueBlockValue =
              Builder.CreateGlobalString(
                  TrueBlock,
                  "implicit.true." +
                      std::to_string(ConditionID));

          Value *FalseBlockValue =
              Builder.CreateGlobalString(
                  FalseBlock,
                  "implicit.false." +
                      std::to_string(ConditionID));

          Builder.CreateCall(
              CustomCallback,
              {
                  Label,
                  ConditionIDValue,
                  Taken,
                  FileValue,
                  ConstantInt::get(I32Ty, Line),
                  ConstantInt::get(I32Ty, Column),
                  ConditionValue,
                  LLVMConditionValue,
                  VariablesValue,
                  TrueBlockValue,
                  FalseBlockValue
              });

          DFCall->eraseFromParent();

          errs() << "[ImplicitTaint] Condition "
                 << ConditionID
                 << " : ";

          if (!File.empty())
            errs() << File;

          errs() << ":" << Line
                 << ":" << Column
                 << "\n";

          ++ConditionID;
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

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION,
      "ImplicitTaint",
      LLVM_VERSION_STRING,
      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name,
               ModulePassManager &MPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "implicit-taint") {
                MPM.addPass(ImplicitTaintPass());
                return true;
              }

              return false;
            });
      }
  };
}