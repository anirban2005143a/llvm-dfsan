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
#include "llvm/IR/InlineAsm.h"
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
#include <vector>

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
  std::vector<uint64_t> ArrayIndices;
  uint32_t Flags = 0;
};

// These flag bits are shared with implicit_runtime.c.
static constexpr uint32_t VariableFlagFloating = 1u << 1;
static constexpr uint32_t VariableFlagGlobal = 1u << 3;

static bool isSupportedScalarType(Type *T) {
  return T && (T->isIntegerTy() || T->isFloatingPointTy());
}

// Arrays are tracked element-by-element only when every leaf is a primitive
// integer or floating-point scalar. Pointers and aggregate elements are not
// followed, and only LLVM's statically-sized ArrayType is accepted here.
static bool collectArrayElementPaths(
    Type *T, std::vector<uint64_t> &Path,
    std::vector<std::vector<uint64_t>> &Paths) {
  if (auto *AT = dyn_cast<ArrayType>(T)) {
    for (uint64_t I = 0; I < AT->getNumElements(); ++I) {
      Path.push_back(I);
      bool Supported = collectArrayElementPaths(AT->getElementType(), Path, Paths);
      Path.pop_back();
      if (!Supported)
        return false;
    }
    return true;
  }

  if (!Path.empty() && isSupportedScalarType(T)) {
    Paths.push_back(Path);
    return true;
  }
  return false;
}

static Type *getArrayElementType(Type *T, ArrayRef<uint64_t> Indices) {
  for (uint64_t Index : Indices) {
    auto *AT = dyn_cast<ArrayType>(T);
    if (!AT || Index >= AT->getNumElements())
      return nullptr;
    T = AT->getElementType();
  }
  return T;
}

static std::string indexedVariableName(StringRef Base,
                                       ArrayRef<uint64_t> Indices) {
  std::string Name = Base.str();
  for (uint64_t Index : Indices)
    Name += "[" + std::to_string(Index) + "]";
  return Name;
}

static bool isSingleObjectAlloca(const AllocaInst *AI) {
  if (!AI)
    return false;
  const auto *Count = dyn_cast<ConstantInt>(AI->getArraySize());
  return Count && Count->isOne();
}

static uint32_t scalarFlags(Type *T) {
  return T && T->isFloatingPointTy() ? VariableFlagFloating : 0u;
}

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

static bool isDirectFunctionScope(const DILocalVariable *Var,
                                  const Function &F) {
  if (!Var || !F.getSubprogram())
    return false;

  // Clang may wrap the active source file in DILexicalBlockFile. Strip only
  // those wrappers: a real DILexicalBlock means the variable was declared
  // inside an if/else/loop/other nested block and must not be reported.
  const DIScope *Scope = Var->getScope();
  while (const auto *BlockFile = dyn_cast<DILexicalBlockFile>(Scope))
    Scope = BlockFile->getScope();

  return Scope == F.getSubprogram();
}

static bool findDebugVariable(Function &F, AllocaInst *AI, std::string &Name,
                              unsigned &Line, bool &IsMainFunctionScope,
                              bool &IsParameter) {
  IsMainFunctionScope = false;
  IsParameter = false;
  auto SaveVariable = [&](DILocalVariable *Var) {
    if (!Var)
      return false;
    Name = Var->getName().str();
    Line = Var->getLine();
    IsMainFunctionScope = isDirectFunctionScope(Var, F);
    IsParameter = Var->isParameter();
    return !Name.empty();
  };

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      for (DbgRecord &DR : I.getDbgRecordRange()) {
        auto *DVR = dyn_cast<DbgVariableRecord>(&DR);
        if (!DVR || !DVR->isDbgDeclare())
          continue;

        Value *Address = DVR->getAddress();
        if (!Address || Address->stripPointerCasts() != AI)
          continue;

        if (SaveVariable(DVR->getVariable()))
          return true;
      }
    }

    for (Instruction &I : BB) {
      auto *DDI = dyn_cast<DbgDeclareInst>(&I);
      if (!DDI)
        continue;

      Value *Address = DDI->getAddress();
      if (!Address || Address->stripPointerCasts() != AI)
        continue;

      if (SaveVariable(DDI->getVariable()))
        return true;
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
      if (!AI || !isSingleObjectAlloca(AI))
        continue; // Do not misclassify variable-length arrays as scalars.

      Type *AllocatedType = AI->getAllocatedType();
      std::vector<std::vector<uint64_t>> Paths;
      if (isSupportedScalarType(AllocatedType)) {
        Paths.emplace_back();
      } else if (isa<ArrayType>(AllocatedType)) {
        std::vector<uint64_t> CurrentPath;
        if (!collectArrayElementPaths(AllocatedType, CurrentPath, Paths))
          continue;
      } else {
        // Pointer variables and other aggregates are out of scope.
        continue;
      }

      uint64_t OwnerSize = 0;
      if (!getAllocaSize(DL, AI, OwnerSize))
        continue;

      std::string Name;
      unsigned Line = 0;
      bool IsMainFunctionScope = false;
      bool IsParameter = false;
      bool HasDebugName = findDebugVariable(
          F, AI, Name, Line, IsMainFunctionScope, IsParameter);

      // Exact lexical-scope filtering requires source debug information. The
      // build uses -g; if the source variable cannot be identified reliably,
      // skip it rather than accidentally including a nested-block variable.
      if (!HasDebugName || !IsMainFunctionScope || IsParameter || Name.empty() ||
          isInternalAllocaName(Name))
        continue;

      std::string BaseName = Name;
      unsigned Count = NameCounts[Name]++;
      if (Count != 0) {
        if (Line != 0)
          BaseName += "@" + std::to_string(Line);
        else
          BaseName += "#" + std::to_string(Count + 1);
      }

      for (const std::vector<uint64_t> &Path : Paths) {
        Type *ElementType = Path.empty()
                                ? AllocatedType
                                : getArrayElementType(AllocatedType, Path);
        if (!isSupportedScalarType(ElementType))
          continue;

        TypeSize ElementTS = DL.getTypeStoreSize(ElementType);
        if (ElementTS.isScalable() || ElementTS.getFixedValue() == 0)
          continue;

        std::string KeyName = Path.empty()
                                  ? BaseName
                                  : indexedVariableName(BaseName, Path);
        Vars.push_back({AI, KeyName, Line, ElementTS.getFixedValue(),
                        Path, scalarFlags(ElementType)});
      }
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

static FunctionCallee getMarkInitializedFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(Ctx, 0);
  FunctionType *FT = FunctionType::get(VoidTy, {I8PtrTy, IntPtrTy}, false);
  return M.getOrInsertFunction("__implicit_mark_initialized", FT);
}

static void instrumentMainVariableStores(Module &M, Function &F) {
  if (F.getName() != "main")
    return;

  FunctionCallee MarkInitialized = getMarkInitializedFunction(M);
  SmallVector<StoreInst *, 128> Stores;
  SmallVector<MemIntrinsic *, 32> MemoryWrites;
  const DataLayout &DL = M.getDataLayout();

  // Keep the exact destination GEP so arr[i] initializes only that element.
  // A store of an entire aggregate passes its full byte size, allowing the
  // runtime to mark all registered elements covered by that write.
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        Stores.push_back(SI);
      } else if (auto *MI = dyn_cast<MemIntrinsic>(&I)) {
        MemoryWrites.push_back(MI);
      }
    }
  }

  Type *I8PtrTy = PointerType::get(M.getContext(), 0);
  Type *IntPtrTy = DL.getIntPtrType(M.getContext(), 0);
  for (StoreInst *SI : Stores) {
    Instruction *Next = SI->getNextNode();
    if (!Next || !SI->getValueOperand()->getType()->isSized())
      continue;
    TypeSize TS = DL.getTypeStoreSize(SI->getValueOperand()->getType());
    if (TS.isScalable())
      continue;
    IRBuilder<> B(Next);
    Value *Address = B.CreatePointerCast(SI->getPointerOperand(), I8PtrTy);
    Value *Size = ConstantInt::get(IntPtrTy, TS.getFixedValue());
    B.CreateCall(MarkInitialized, {Address, Size});
  }

  for (MemIntrinsic *MI : MemoryWrites) {
    Instruction *Next = MI->getNextNode();
    if (!Next)
      continue;
    IRBuilder<> B(Next);
    Value *Address = B.CreatePointerCast(MI->getRawDest(), I8PtrTy);
    Value *Size = B.CreateZExtOrTrunc(MI->getLength(), IntPtrTy);
    B.CreateCall(MarkInitialized, {Address, Size});
  }
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
  CallInst *EnterCall = EntryBuilder.CreateCall(Enter, ScopePtr);

  // Register primitive scalar globals and each primitive element of fixed-size
  // global arrays. Each array element gets its own name/address/DFSan label.
  Instruction *GlobalInsertPt = EnterCall->getNextNode();
  if (!GlobalInsertPt)
    GlobalInsertPt = Entry.getTerminator();

  const DataLayout &DL = M.getDataLayout();
  SmallVector<GlobalVariable *, 64> SourceGlobals;
  for (GlobalVariable &GV : M.globals()) {
    Type *GlobalType = GV.getValueType();
    if (GV.isDeclaration())
      continue;

    StringRef GlobalName = GV.getName();
    if (GlobalName.empty() || GlobalName.starts_with(".") ||
        GlobalName.starts_with("$") || GlobalName.starts_with("__const") ||
        GlobalName.starts_with("__dfsan") ||
        GlobalName.starts_with("__implicit_") ||
        GlobalName.starts_with("llvm.") ||
        GlobalName.starts_with("__llvm_"))
      continue;

    if (isSupportedScalarType(GlobalType)) {
      SourceGlobals.push_back(&GV);
    } else if (isa<ArrayType>(GlobalType)) {
      std::vector<std::vector<uint64_t>> Paths;
      std::vector<uint64_t> CurrentPath;
      if (collectArrayElementPaths(GlobalType, CurrentPath, Paths))
        SourceGlobals.push_back(&GV);
    }
  }

  // Do not mutate the module's global-variable list while iterating over it:
  // CreateGlobalString below adds globals of its own.
  for (GlobalVariable *GV : SourceGlobals) {
    Type *GlobalType = GV->getValueType();
    TypeSize OwnerTS = DL.getTypeAllocSize(GlobalType);
    if (OwnerTS.isScalable() || OwnerTS.getFixedValue() == 0)
      continue;

    std::vector<std::vector<uint64_t>> Paths;
    if (isSupportedScalarType(GlobalType)) {
      Paths.emplace_back();
    } else {
      std::vector<uint64_t> CurrentPath;
      if (!collectArrayElementPaths(GlobalType, CurrentPath, Paths))
        continue;
    }

    for (const std::vector<uint64_t> &Path : Paths) {
      Type *ElementType = Path.empty()
                              ? GlobalType
                              : getArrayElementType(GlobalType, Path);
      if (!isSupportedScalarType(ElementType))
        continue;

      TypeSize ElementTS = DL.getTypeStoreSize(ElementType);
      if (ElementTS.isScalable() || ElementTS.getFixedValue() == 0)
        continue;

      IRBuilder<> GB(GlobalInsertPt);
      Value *Scope = GB.CreateGlobalString("global", "implicit_global_scope");
      std::string ElementName = Path.empty()
                                    ? GV->getName().str()
                                    : indexedVariableName(GV->getName(), Path);
      Value *Name = GB.CreateGlobalString(ElementName, "implicit_global_name");
      Value *Address = GV;
      if (!Path.empty()) {
        SmallVector<Value *, 8> Indices;
        Indices.push_back(GB.getInt32(0));
        for (uint64_t Index : Path)
          Indices.push_back(GB.getInt32(Index));
        Address = GB.CreateInBoundsGEP(GlobalType, GV, Indices,
                                       "implicit_global_element");
      }
      Address = GB.CreatePointerCast(Address, PointerType::get(M.getContext(), 0));
      Value *Size = ConstantInt::get(IntPtrTy, ElementTS.getFixedValue());
      Value *Flags = ConstantInt::get(
          I32Ty, scalarFlags(ElementType) | VariableFlagGlobal);
      GB.CreateCall(Register, {Scope, Name, Address, Size, Flags});
    }
  }

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
    if (!Var.ArrayIndices.empty()) {
      SmallVector<Value *, 8> Indices;
      Indices.push_back(B.getInt32(0));
      for (uint64_t Index : Var.ArrayIndices)
        Indices.push_back(B.getInt32(Index));
      Address = B.CreateInBoundsGEP(Var.Alloca->getAllocatedType(),
                                    Var.Alloca, Indices,
                                    "implicit_array_element");
    }
    Address = B.CreatePointerCast(Address, PointerType::get(M.getContext(), 0));
    Value *Size = ConstantInt::get(IntPtrTy, Var.Size);
    Value *Flags = ConstantInt::get(I32Ty, Var.Flags);
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

// Must run before DFSan instrumentation. DFSan normally keeps the labels of
// non-escaping local allocas in compiler-generated shadow allocas. Since the
// implicit analysis adds labels dynamically at runtime, those labels would not
// be visible to later loads/stores. An opaque side-effecting asm use makes each
// main-local address escape, forcing DFSan to use memory-backed shadow for it.
struct EscapeMainAllocasForDFSan : public PassInfoMixin<EscapeMainAllocasForDFSan> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    Function *F = M.getFunction("main");
    if (!F || F->isDeclaration())
      return PreservedAnalyses::all();

    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    SmallVector<AllocaInst *, 64> Allocas;
    for (BasicBlock &BB : *F) {
      for (Instruction &I : BB) {
        if (auto *AI = dyn_cast<AllocaInst>(&I))
          Allocas.push_back(AI);
      }
    }

    if (Allocas.empty())
      return PreservedAnalyses::all();

    for (AllocaInst *AI : Allocas) {
      Instruction *InsertPt = AI->getNextNode();
      if (!InsertPt)
        continue;

      Type *PtrTy = AI->getType();
      FunctionType *AsmTy = FunctionType::get(VoidTy, {PtrTy}, false);
      InlineAsm *Escape = InlineAsm::get(AsmTy, "", "r,~{memory}", true);
      IRBuilder<> B(InsertPt);
      B.CreateCall(Escape, {AI});
    }

    return PreservedAnalyses::none();
  }
};

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
    instrumentMainVariableStores(*M, F);
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
            // Clang loads this plugin while building its default optimization
            // pipeline. PipelineStart runs before DFSan's sanitizer pass is
            // inserted, which is essential for the alloca-shadow workaround.
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                  MPM.addPass(EscapeMainAllocasForDFSan());
                });

            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "implicit-escape") {
                    MPM.addPass(EscapeMainAllocasForDFSan());
                    return true;
                  }

                  if (Name == "implicit-taint") {
                    MPM.addPass(createModuleToFunctionPassAdaptor(
                        ImplicitTaintPropagation()));
                    return true;
                  }

                  return false;
                });
          }};
}
