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
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

using namespace llvm;

namespace {

struct BranchSite {
  Instruction *Terminator = nullptr;
  uint32_t Id = 0;
};

struct VariableInfo {
  AllocaInst *Alloca = nullptr;
  std::string Name;
  unsigned Line = 0;
  uint64_t Size = 0;
  bool IsPointer = false;
};

static bool isDFSanConditionalCallback(const CallBase *CB) {
  if (!CB)
    return false;

  const Function *Callee = CB->getCalledFunction();
  if (!Callee)
    return false;

  StringRef Name = Callee->getName();
  return Name == "__dfsan_conditional_callback" ||
         Name == "__dfsan_conditional_callback_origin";
}

static CallBase *findConditionalCallback(Instruction *Terminator) {
  if (!Terminator)
    return nullptr;

  BasicBlock *BB = Terminator->getParent();
  for (Instruction &I : *BB) {
    if (&I == Terminator)
      break;

    if (auto *CB = dyn_cast<CallBase>(&I)) {
      if (isDFSanConditionalCallback(CB))
        return CB;
    }
  }

  return nullptr;
}

static BasicBlock *getImmediatePostDominator(PostDominatorTree &PDT,
                                               BasicBlock *BB) {
  if (!BB)
    return nullptr;

  auto *Node = PDT.getNode(BB);
  if (!Node || !Node->getIDom())
    return nullptr;

  return Node->getIDom()->getBlock();
}

static void addUniqueDependency(SmallVectorImpl<uint32_t> &Deps,
                                uint32_t Id) {
  for (uint32_t Existing : Deps) {
    if (Existing == Id)
      return;
  }
  Deps.push_back(Id);
}

static void markControlDependentRegion(
    BasicBlock *BranchBB, BasicBlock *Stop, uint32_t BranchId,
    DenseMap<BasicBlock *, SmallVector<uint32_t, 8>> &Dependencies,
    ArrayRef<BasicBlock *> Successors) {
  SmallVector<BasicBlock *, 64> Worklist;
  SmallPtrSet<BasicBlock *, 32> Seen;

  for (BasicBlock *Succ : Successors) {
    if (Succ && Succ != Stop && Succ != BranchBB)
      Worklist.push_back(Succ);
  }

  while (!Worklist.empty()) {
    BasicBlock *BB = Worklist.pop_back_val();
    if (!BB || BB == Stop || BB == BranchBB || !Seen.insert(BB).second)
      continue;

    addUniqueDependency(Dependencies[BB], BranchId);

    for (BasicBlock *Succ : successors(BB)) {
      if (Succ != Stop && Succ != BranchBB)
        Worklist.push_back(Succ);
    }
  }
}

static void collectBranchSites(
    Function &F, uint32_t &NextBranchId,
    SmallVectorImpl<BranchSite> &Sites) {
  for (BasicBlock &BB : F) {
    Instruction *Term = BB.getTerminator();

    if (auto *BI = dyn_cast<BranchInst>(Term)) {
      if (!BI->isConditional())
        continue;

      Sites.push_back({Term, NextBranchId++});
      continue;
    }

    if (isa<SwitchInst>(Term))
      Sites.push_back({Term, NextBranchId++});
  }
}

static void computeControlDependencies(
    Function &F, PostDominatorTree &PDT, ArrayRef<BranchSite> Sites,
    DenseMap<BasicBlock *, SmallVector<uint32_t, 8>> &Dependencies) {
  for (const BranchSite &Site : Sites) {
    BasicBlock *BranchBB = Site.Terminator->getParent();
    BasicBlock *Merge = getImmediatePostDominator(PDT, BranchBB);

    SmallVector<BasicBlock *, 8> Succs;
    if (auto *BI = dyn_cast<BranchInst>(Site.Terminator)) {
      Succs.push_back(BI->getSuccessor(0));
      Succs.push_back(BI->getSuccessor(1));
    } else if (auto *SI = dyn_cast<SwitchInst>(Site.Terminator)) {
      Succs.push_back(SI->getDefaultDest());
      for (auto Case : SI->cases())
        Succs.push_back(Case.getCaseSuccessor());
    }

    markControlDependentRegion(BranchBB, Merge, Site.Id, Dependencies,
                               Succs);
  }

  for (auto &Entry : Dependencies) {
    auto &Deps = Entry.second;
    std::sort(Deps.begin(), Deps.end());
  }
}

static bool findDebugVariable(Function &F, AllocaInst *AI, std::string &Name,
                              unsigned &Line) {
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      for (DbgRecord &DR : I.getDbgRecordRange()) {
        auto *DVR = dyn_cast<DbgVariableRecord>(&DR);
        if (!DVR || !DVR->isDbgDeclare())
          continue;

        Value *Address = DVR->getAddress();
        if (!Address || Address->stripPointerCasts() != AI)
          continue;

        if (DILocalVariable *Var = DVR->getVariable()) {
          Name = Var->getName().str();
          Line = Var->getLine();
          return !Name.empty();
        }
      }
    }

    for (Instruction &I : BB) {
      auto *DDI = dyn_cast<DbgDeclareInst>(&I);
      if (!DDI)
        continue;

      Value *Address = DDI->getAddress();
      if (!Address || Address->stripPointerCasts() != AI)
        continue;

      if (DILocalVariable *Var = DDI->getVariable()) {
        Name = Var->getName().str();
        Line = Var->getLine();
        return !Name.empty();
      }
    }
  }

  return false;
}

static bool getAllocaSize(const DataLayout &DL, AllocaInst *AI,
                          uint64_t &Size) {
  if (!AI || !AI->getAllocatedType()->isSized())
    return false;

  TypeSize TS = DL.getTypeAllocSize(AI->getAllocatedType());
  if (TS.isScalable())
    return false;

  Size = TS.getFixedValue();
  return Size != 0;
}

static bool isInternalAllocaName(StringRef Name) {
  return Name.starts_with("__dfsan") || Name.starts_with("__implicit_");
}

static void collectVariables(Function &F,
                             SmallVectorImpl<VariableInfo> &Vars) {
  Module *M = F.getParent();
  if (!M)
    return;

  const DataLayout &DL = M->getDataLayout();
  std::unordered_map<std::string, unsigned> NameCounts;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *AI = dyn_cast<AllocaInst>(&I);
      if (!AI)
        continue;

      uint64_t Size = 0;
      if (!getAllocaSize(DL, AI, Size))
        continue;

      std::string Name;
      unsigned Line = 0;
      bool HasDebugName = findDebugVariable(F, AI, Name, Line);

      if (!HasDebugName) {
        Name = AI->getName().str();
        if (Name.empty() || isInternalAllocaName(Name))
          continue;
      }

      if (Name.empty())
        continue;

      std::string KeyName = Name;
      unsigned Count = NameCounts[Name]++;
      if (Count != 0) {
        if (Line != 0)
          KeyName += "@" + std::to_string(Line);
        else
          KeyName += "#" + std::to_string(Count + 1);
      }

      Vars.push_back({AI, KeyName, Line, Size,
                      AI->getAllocatedType()->isPointerTy()});
    }
  }
}

static FunctionCallee getEnterFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  FunctionType *FT = FunctionType::get(VoidTy, {I8PtrTy}, false);
  return M.getOrInsertFunction("__implicit_enter_function", FT);
}

static FunctionCallee getExitFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  FunctionType *FT = FunctionType::get(VoidTy, {I8PtrTy}, false);
  return M.getOrInsertFunction("__implicit_exit_function", FT);
}

static FunctionCallee getRegisterVariableFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(Ctx, 0);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  FunctionType *FT =
      FunctionType::get(VoidTy, {I8PtrTy, I8PtrTy, I8PtrTy, IntPtrTy, I32Ty},
                        false);
  return M.getOrInsertFunction("__implicit_register_variable", FT);
}

static FunctionCallee getClearConditionFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  FunctionType *FT = FunctionType::get(VoidTy, {}, false);
  return M.getOrInsertFunction("__implicit_clear_condition", FT);
}

static FunctionCallee getRecordConditionFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  FunctionType *FT = FunctionType::get(VoidTy, {I32Ty}, false);
  return M.getOrInsertFunction("__implicit_record_condition", FT);
}

static FunctionCallee getRecordOutcomeFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  FunctionType *FT = FunctionType::get(VoidTy, {I32Ty, I32Ty}, false);
  return M.getOrInsertFunction("__implicit_record_branch_outcome", FT);
}

static FunctionCallee getObserveBranchFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  Type *I32PtrTy = PointerType::get(Ctx, 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(Ctx, 0);
  FunctionType *FT =
      FunctionType::get(VoidTy, {I32Ty, I32PtrTy, IntPtrTy}, false);
  return M.getOrInsertFunction("__implicit_observe_branch", FT);
}

static void instrumentConditional(Module &M, Instruction *Terminator,
                                  uint32_t BranchId) {
  FunctionCallee Clear = getClearConditionFunction(M);
  FunctionCallee Record = getRecordConditionFunction(M);
  FunctionCallee Outcome = getRecordOutcomeFunction(M);

  CallBase *DFSanCallback = findConditionalCallback(Terminator);

  if (DFSanCallback) {
    IRBuilder<> B(DFSanCallback);
    B.CreateCall(Clear);
  } else {
    IRBuilder<> B(Terminator);
    B.CreateCall(Clear);
  }

  IRBuilder<> B(Terminator);
  LLVMContext &Ctx = M.getContext();
  Value *BranchIdValue = ConstantInt::get(Type::getInt32Ty(Ctx), BranchId);
  B.CreateCall(Record, BranchIdValue);

  Value *OutcomeValue = nullptr;

  if (auto *BI = dyn_cast<BranchInst>(Terminator)) {
    Value *Cond = BI->getCondition();
    OutcomeValue = B.CreateZExt(Cond, Type::getInt32Ty(Ctx),
                                "implicit_branch_outcome");
  } else if (auto *SI = dyn_cast<SwitchInst>(Terminator)) {
    Value *SwitchValue = SI->getCondition();

    DenseMap<BasicBlock *, unsigned> SuccessorIds;
    unsigned NextSuccessorId = 0;
    SuccessorIds[SI->getDefaultDest()] = NextSuccessorId++;

    for (auto Case : SI->cases()) {
      BasicBlock *Succ = Case.getCaseSuccessor();
      if (!SuccessorIds.count(Succ))
        SuccessorIds[Succ] = NextSuccessorId++;
    }

    OutcomeValue = B.getInt32(0);

    for (auto Case : SI->cases()) {
      BasicBlock *Succ = Case.getCaseSuccessor();
      unsigned SuccessorId = SuccessorIds.lookup(Succ);
      Value *Match = B.CreateICmpEQ(SwitchValue, Case.getCaseValue(),
                                    "implicit_switch_match");
      OutcomeValue = B.CreateSelect(
          Match, B.getInt32(SuccessorId), OutcomeValue,
          "implicit_switch_outcome");
    }
  }

  if (OutcomeValue)
    B.CreateCall(Outcome, {BranchIdValue, OutcomeValue});
}

static GlobalVariable *createDependencyGlobal(
    Module &M, Function &F, uint32_t BranchId,
    ArrayRef<uint32_t> Dependencies) {
  LLVMContext &Ctx = M.getContext();
  Type *I32Ty = Type::getInt32Ty(Ctx);
  ArrayType *ArrayTy = ArrayType::get(I32Ty, Dependencies.size());

  SmallVector<Constant *, 16> Values;
  Values.reserve(Dependencies.size());
  for (uint32_t Id : Dependencies)
    Values.push_back(ConstantInt::get(I32Ty, Id));

  Constant *Initializer = ConstantArray::get(ArrayTy, Values);
  std::string GlobalName =
      "__implicit_ctx_" + F.getName().str() + "_" +
      std::to_string(BranchId);

  return new GlobalVariable(M, ArrayTy, true, GlobalValue::PrivateLinkage,
                            Initializer, GlobalName);
}

static void instrumentBranchObservations(
    Module &M, Function &F, ArrayRef<BranchSite> Sites,
    PostDominatorTree &PDT,
    DenseMap<BasicBlock *, SmallVector<uint32_t, 8>> &Dependencies) {
  if (F.getName() != "main")
    return;

  FunctionCallee Observe = getObserveBranchFunction(M);
  Type *I32PtrTy = PointerType::get(M.getContext(), 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(M.getContext(), 0);

  for (const BranchSite &Site : Sites) {
    BasicBlock *BranchBB = Site.Terminator->getParent();
    BasicBlock *Merge = getImmediatePostDominator(PDT, BranchBB);
    if (!Merge || Merge == BranchBB)
      continue;

    SmallVector<uint32_t, 8> Context;
    auto DepIt = Dependencies.find(BranchBB);
    if (DepIt != Dependencies.end()) {
      for (uint32_t Id : DepIt->second)
        addUniqueDependency(Context, Id);
    }
    addUniqueDependency(Context, Site.Id);
    std::sort(Context.begin(), Context.end());

    GlobalVariable *GV =
        createDependencyGlobal(M, F, Site.Id, ArrayRef<uint32_t>(Context));

    Instruction *IP = &*Merge->getFirstInsertionPt();
    IRBuilder<> B(IP);

    Value *BranchIdValue =
        ConstantInt::get(Type::getInt32Ty(M.getContext()), Site.Id);
    Value *Zero = B.getInt32(0);
    Value *ContextPtr =
        B.CreateInBoundsGEP(GV->getValueType(), GV,
                            ArrayRef<Value *>({Zero, Zero}),
                            "implicit_context_ptr");
    Value *Count = ConstantInt::get(IntPtrTy, Context.size());

    if (Context.empty())
      ContextPtr = ConstantPointerNull::get(cast<PointerType>(I32PtrTy));

    B.CreateCall(Observe, {BranchIdValue, ContextPtr, Count});
  }
}

static void instrumentFunctionEntryAndVariables(
    Module &M, Function &F, ArrayRef<VariableInfo> Vars) {
  if (F.getName() != "main")
    return;

  FunctionCallee Enter = getEnterFunction(M);
  FunctionCallee Register = getRegisterVariableFunction(M);
  Type *I8PtrTy = PointerType::get(M.getContext(), 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(M.getContext(), 0);
  Type *I32Ty = Type::getInt32Ty(M.getContext());

  BasicBlock &Entry = F.getEntryBlock();
  Instruction *EntryIP = &*Entry.getFirstInsertionPt();
  IRBuilder<> EntryBuilder(EntryIP);
  Value *ScopePtr =
      EntryBuilder.CreateGlobalString(F.getName(), "implicit_scope");
  EntryBuilder.CreateCall(Enter, ScopePtr);

  for (const VariableInfo &Var : Vars) {
    if (!Var.Alloca)
      continue;

    Instruction *AfterAlloca = Var.Alloca->getNextNode();
    if (!AfterAlloca)
      continue;

    IRBuilder<> B(AfterAlloca);
    Value *Scope = B.CreateGlobalString(F.getName(), "implicit_scope");
    Value *Name = B.CreateGlobalString(Var.Name, "implicit_var_name");
    Value *Address = Var.Alloca;
    Value *Size = ConstantInt::get(IntPtrTy, Var.Size);
    Value *Flags = ConstantInt::get(I32Ty, Var.IsPointer ? 1 : 0);
    B.CreateCall(Register, {Scope, Name, Address, Size, Flags});
  }

  (void)I8PtrTy;
}

static void instrumentFunctionExits(Module &M, Function &F) {
  if (F.getName() != "main")
    return;

  FunctionCallee Exit = getExitFunction(M);

  for (BasicBlock &BB : F) {
    auto *RI = dyn_cast<ReturnInst>(BB.getTerminator());
    if (!RI)
      continue;

    IRBuilder<> B(RI);
    Value *Scope = B.CreateGlobalString(F.getName(), "implicit_scope");
    B.CreateCall(Exit, Scope);
  }
}

struct ImplicitTaintPropagation
    : public PassInfoMixin<ImplicitTaintPropagation> {
  uint32_t NextBranchId = 1;

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    if (F.isDeclaration() || F.empty())
      return PreservedAnalyses::all();

    Module *M = F.getParent();
    if (!M)
      return PreservedAnalyses::all();

    PostDominatorTree &PDT =
        FAM.getResult<PostDominatorTreeAnalysis>(F);

    SmallVector<BranchSite, 32> Branches;
    collectBranchSites(F, NextBranchId, Branches);

    DenseMap<BasicBlock *, SmallVector<uint32_t, 8>> Dependencies;
    computeControlDependencies(F, PDT, Branches, Dependencies);

    SmallVector<VariableInfo, 64> Variables;
    if (F.getName() == "main")
      collectVariables(F, Variables);

    instrumentFunctionEntryAndVariables(*M, F, Variables);
    instrumentBranchObservations(*M, F, Branches, PDT, Dependencies);

    for (const BranchSite &Site : Branches)
      instrumentConditional(*M, Site.Terminator, Site.Id);

    instrumentFunctionExits(*M, F);

    return PreservedAnalyses::none();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ImplicitTaintPropagation",
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "implicit-taint")
                    return false;

                  MPM.addPass(createModuleToFunctionPassAdaptor(
                      ImplicitTaintPropagation()));
                  return true;
                });
          }};
}
