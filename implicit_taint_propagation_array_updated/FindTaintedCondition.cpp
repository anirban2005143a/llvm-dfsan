// End-of-main value-comparison instrumentation for LLVM + DFSan.
//
// This pass deliberately does not instrument branches or control dependencies.
// Implicit taint is inferred by analyze_results.py by comparing final concrete
// values across executions that share the same public inputs.

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugProgramInstruction.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/ErrorHandling.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace {

struct VariableInfo {
  AllocaInst *Alloca = nullptr;
  std::string Name;
  unsigned Line = 0;
  uint64_t Size = 0;
  std::vector<uint64_t> ArrayIndices;
  uint32_t Flags = 0;
  Value *DynamicArrayCount = nullptr;
};

// These flag bits are shared with implicit_runtime.c.
static constexpr uint32_t VariableFlagFloating = 1u << 1;
static constexpr uint32_t VariableFlagGlobal = 1u << 3;

static bool isSupportedScalarType(Type *T) {
  return T && (T->isIntegerTy() || T->isFloatingPointTy());
}

// Enumerate the primitive scalar leaves of a statically-sized array. Each leaf
// is registered separately so DFSan labels can be read/applied per element.
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

static uint32_t scalarFlags(Type *T) {
  return T && T->isFloatingPointTy() ? VariableFlagFloating : 0u;
}

static bool isDirectFunctionScope(const DILocalVariable *Var,
                                  const Function &F) {
  if (!Var || !F.getSubprogram())
    return false;

  // Strip file wrappers, but do not strip a real lexical block: variables
  // inside nested if/else/loop scopes must not be included in the report.
  const DIScope *Scope = Var->getScope();
  while (const auto *BlockFile = dyn_cast<DILexicalBlockFile>(Scope))
    Scope = BlockFile->getScope();

  return Scope == F.getSubprogram();
}

static bool findDebugVariable(Function &F, AllocaInst *AI, std::string &Name,
                              unsigned &Line, bool &IsMainFunctionScope,
                              bool &IsParameter, bool &IsArrayVariable) {
  IsMainFunctionScope = false;
  IsParameter = false;
  IsArrayVariable = false;

  auto SaveVariable = [&](DILocalVariable *Var) {
    if (!Var || Var->isArtificial())
      return false;
    Name = Var->getName().str();
    Line = Var->getLine();
    IsMainFunctionScope = isDirectFunctionScope(Var, F);
    IsParameter = Var->isParameter();
    if (auto *Composite = dyn_cast_or_null<DICompositeType>(Var->getType()))
      IsArrayVariable = Composite->getTag() == dwarf::DW_TAG_array_type;
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

      std::string Name;
      unsigned Line = 0;
      bool IsMainFunctionScope = false;
      bool IsParameter = false;
      bool IsArrayVariable = false;
      bool HasDebugName = findDebugVariable(
          F, AI, Name, Line, IsMainFunctionScope, IsParameter, IsArrayVariable);

      // Keep only source variables declared directly in main. This also
      // excludes compiler-generated allocas without reliable debug metadata.
      if (!HasDebugName || !IsMainFunctionScope || IsParameter || Name.empty())
        continue;

      Type *AllocatedType = AI->getAllocatedType();
      std::vector<std::vector<uint64_t>> Paths;
      Value *DynamicArrayCount = nullptr;

      if (isSupportedScalarType(AllocatedType)) {
        if (IsArrayVariable) {
          // C VLAs lower to an alloca of the primitive element type with a
          // runtime element count. Keep the count as an SSA value so the
          // runtime can register exactly the elements present in this run.
          DynamicArrayCount = AI->getArraySize();
        } else {
          auto *Count = dyn_cast<ConstantInt>(AI->getArraySize());
          if (!Count || !Count->isOne())
            continue;
          Paths.emplace_back();
        }
      } else if (isa<ArrayType>(AllocatedType)) {
        std::vector<uint64_t> CurrentPath;
        if (!collectArrayElementPaths(AllocatedType, CurrentPath, Paths))
          continue;
      } else {
        // Pointer variables, structures, unions and other aggregates are out.
        continue;
      }

      Type *ElementType = AllocatedType;
      if (!DynamicArrayCount && !Paths.empty() && !Paths.front().empty())
        ElementType = getArrayElementType(AllocatedType, Paths.front());
      TypeSize ElementTS = DL.getTypeStoreSize(ElementType);
      if (ElementTS.isScalable() || ElementTS.getFixedValue() == 0)
        continue;

      std::string BaseName = Name;
      unsigned Count = NameCounts[Name]++;
      if (Count != 0) {
        if (Line != 0)
          BaseName += "@" + std::to_string(Line);
        else
          BaseName += "#" + std::to_string(Count + 1);
      }

      if (DynamicArrayCount) {
        Vars.push_back({AI, BaseName, Line, ElementTS.getFixedValue(), {},
                        scalarFlags(ElementType), DynamicArrayCount});
        continue;
      }

      for (const std::vector<uint64_t> &Path : Paths) {
        Type *LeafType = Path.empty()
                             ? AllocatedType
                             : getArrayElementType(AllocatedType, Path);
        if (!isSupportedScalarType(LeafType))
          continue;

        TypeSize LeafTS = DL.getTypeStoreSize(LeafType);
        if (LeafTS.isScalable() || LeafTS.getFixedValue() == 0)
          continue;

        std::string KeyName = Path.empty()
                                  ? BaseName
                                  : indexedVariableName(BaseName, Path);
        Vars.push_back({AI, KeyName, Line, LeafTS.getFixedValue(), Path,
                        scalarFlags(LeafType), nullptr});
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
  FunctionType *FT = FunctionType::get(
      VoidTy, {I8PtrTy, I8PtrTy, I8PtrTy, IntPtrTy, I32Ty}, false);
  return M.getOrInsertFunction("__implicit_register_variable", FT);
}

static FunctionCallee getRegisterArrayFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(Ctx, 0);
  Type *I32Ty = Type::getInt32Ty(Ctx);
  FunctionType *FT = FunctionType::get(
      VoidTy, {I8PtrTy, I8PtrTy, I8PtrTy, IntPtrTy, IntPtrTy, I32Ty}, false);
  return M.getOrInsertFunction("__implicit_register_array", FT);
}

static FunctionCallee getMarkInitializedFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  Type *VoidTy = Type::getVoidTy(Ctx);
  Type *I8PtrTy = PointerType::get(Ctx, 0);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(Ctx, 0);
  FunctionType *FT = FunctionType::get(VoidTy, {I8PtrTy, IntPtrTy}, false);
  return M.getOrInsertFunction("__implicit_mark_initialized", FT);
}

static bool shouldTrackGlobal(const GlobalVariable &GV) {
  if (GV.isDeclaration())
    return false;

  StringRef Name = GV.getName();
  if (Name.empty() || Name.starts_with(".") || Name.starts_with("$") ||
      Name.starts_with("__const") || Name.starts_with("__dfsan") ||
      Name.starts_with("__implicit_") || Name.starts_with("llvm.") ||
      Name.starts_with("__llvm_"))
    return false;

  Type *T = GV.getValueType();
  if (isSupportedScalarType(T))
    return true;
  if (isa<ArrayType>(T)) {
    std::vector<std::vector<uint64_t>> Paths;
    std::vector<uint64_t> CurrentPath;
    return collectArrayElementPaths(T, CurrentPath, Paths);
  }
  return false;
}

static void instrumentFunctionEntryAndVariables(
    Module &M, Function &F, ArrayRef<VariableInfo> Vars) {
  if (F.getName() != "main")
    return;

  // IMPORTANT: collect source globals before CreateGlobalString adds internal
  // globals for instrumentation. This prevents strings such as "main" from
  // being mistaken for user-defined global arrays.
  SmallVector<GlobalVariable *, 64> SourceGlobals;
  for (GlobalVariable &GV : M.globals()) {
    if (shouldTrackGlobal(GV))
      SourceGlobals.push_back(&GV);
  }

  FunctionCallee Enter = getEnterFunction(M);
  FunctionCallee Register = getRegisterVariableFunction(M);
  Type *IntPtrTy = M.getDataLayout().getIntPtrType(M.getContext(), 0);
  Type *I32Ty = Type::getInt32Ty(M.getContext());
  Type *I8PtrTy = PointerType::get(M.getContext(), 0);

  BasicBlock &Entry = F.getEntryBlock();
  Instruction *EntryIP = &*Entry.getFirstInsertionPt();
  IRBuilder<> EntryBuilder(EntryIP);
  Value *ScopePtr = EntryBuilder.CreateGlobalString(F.getName(),
                                                   "__implicit_scope");
  CallInst *EnterCall = EntryBuilder.CreateCall(Enter, ScopePtr);

  Instruction *GlobalInsertPt = EnterCall->getNextNode();
  if (!GlobalInsertPt)
    GlobalInsertPt = Entry.getTerminator();

  const DataLayout &DL = M.getDataLayout();
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
      Value *Scope = GB.CreateGlobalString("global", "__implicit_global_scope");
      std::string ElementName = Path.empty()
                                    ? GV->getName().str()
                                    : indexedVariableName(GV->getName(), Path);
      Value *Name = GB.CreateGlobalString(ElementName, "__implicit_global_name");
      Value *Address = GV;
      if (!Path.empty()) {
        SmallVector<Value *, 8> Indices;
        Indices.push_back(GB.getInt32(0));
        for (uint64_t Index : Path)
          Indices.push_back(GB.getInt32(Index));
        Address = GB.CreateInBoundsGEP(GlobalType, GV, Indices,
                                       "implicit_global_element");
      }
      Address = GB.CreatePointerCast(Address, I8PtrTy);
      Value *Size = ConstantInt::get(IntPtrTy, ElementTS.getFixedValue());
      Value *Flags = ConstantInt::get(
          I32Ty, scalarFlags(ElementType) | VariableFlagGlobal);
      GB.CreateCall(Register, {Scope, Name, Address, Size, Flags});
    }
  }

  FunctionCallee RegisterArray = getRegisterArrayFunction(M);
  for (const VariableInfo &Var : Vars) {
    if (!Var.Alloca)
      continue;

    Instruction *AfterAlloca = Var.Alloca->getNextNode();
    if (!AfterAlloca)
      continue;

    IRBuilder<> B(AfterAlloca);
    Value *Scope = B.CreateGlobalString(F.getName(), "__implicit_scope");
    Value *Name = B.CreateGlobalString(Var.Name, "__implicit_var_name");
    Value *Address = B.CreatePointerCast(Var.Alloca, I8PtrTy);
    Value *Flags = ConstantInt::get(I32Ty, Var.Flags);

    if (Var.DynamicArrayCount) {
      Value *Count = B.CreateZExtOrTrunc(Var.DynamicArrayCount, IntPtrTy,
                                         "implicit_array_count");
      Value *ElementSize = ConstantInt::get(IntPtrTy, Var.Size);
      B.CreateCall(RegisterArray,
                   {Scope, Name, Address, Count, ElementSize, Flags});
      continue;
    }

    if (!Var.ArrayIndices.empty()) {
      SmallVector<Value *, 8> Indices;
      Indices.push_back(B.getInt32(0));
      for (uint64_t Index : Var.ArrayIndices)
        Indices.push_back(B.getInt32(Index));
      Address = B.CreateInBoundsGEP(Var.Alloca->getAllocatedType(),
                                    Var.Alloca, Indices,
                                    "implicit_array_element");
      Address = B.CreatePointerCast(Address, I8PtrTy);
    }
    Value *Size = ConstantInt::get(IntPtrTy, Var.Size);
    B.CreateCall(Register, {Scope, Name, Address, Size, Flags});
  }
}

static void instrumentMainVariableStores(Module &M, Function &F) {
  if (F.getName() != "main")
    return;

  FunctionCallee MarkInitialized = getMarkInitializedFunction(M);
  SmallVector<StoreInst *, 128> Stores;
  SmallVector<MemIntrinsic *, 32> MemoryWrites;
  const DataLayout &DL = M.getDataLayout();

  // Track initialization independently per scalar/array element. A single
  // aggregate write marks all registered elements fully covered by that write.
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *SI = dyn_cast<StoreInst>(&I))
        Stores.push_back(SI);
      else if (auto *MI = dyn_cast<MemIntrinsic>(&I))
        MemoryWrites.push_back(MI);
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

// Find the stack-save token that is active when a tracked VLA is allocated.
// Clang normally emits stacksave immediately before the dynamic alloca, but
// use dominance rather than relying only on textual adjacency.
static IntrinsicInst *findNearestDominatingStackSave(AllocaInst *AI,
                                                     DominatorTree &DT) {
  IntrinsicInst *Best = nullptr;
  bool Ambiguous = false;

  Function *F = AI->getFunction();
  for (BasicBlock &BB : *F) {
    for (Instruction &I : BB) {
      auto *II = dyn_cast<IntrinsicInst>(&I);
      if (!II || II->getIntrinsicID() != Intrinsic::stacksave ||
          !DT.dominates(II, AI))
        continue;

      if (!Best || DT.dominates(Best, II)) {
        Best = II;
      } else if (!DT.dominates(II, Best)) {
        // Do not guess if two incomparable saves can reach this alloca.
        Ambiguous = true;
      }
    }
  }

  return Ambiguous ? nullptr : Best;
}

// A frontend commonly spills the opaque stacksave result to an entry-block
// alloca and reloads it at stackrestore. Resolve that simple memory round-trip,
// as well as direct, cast, select, and PHI forms, back to its stacksave call.
static void collectStackSaveOrigins(
    Value *V, Function &F,
    SmallPtrSetImpl<const IntrinsicInst *> &Origins,
    SmallPtrSetImpl<const Value *> &Visited) {
  if (!V)
    return;

  V = V->stripPointerCasts();
  if (!Visited.insert(V).second)
    return;

  if (auto *II = dyn_cast<IntrinsicInst>(V)) {
    if (II->getIntrinsicID() == Intrinsic::stacksave)
      Origins.insert(II);
    return;
  }

  if (auto *PN = dyn_cast<PHINode>(V)) {
    for (Value *Incoming : PN->incoming_values())
      collectStackSaveOrigins(Incoming, F, Origins, Visited);
    return;
  }

  if (auto *SI = dyn_cast<SelectInst>(V)) {
    collectStackSaveOrigins(SI->getTrueValue(), F, Origins, Visited);
    collectStackSaveOrigins(SI->getFalseValue(), F, Origins, Visited);
    return;
  }

  if (auto *FI = dyn_cast<FreezeInst>(V)) {
    collectStackSaveOrigins(FI->getOperand(0), F, Origins, Visited);
    return;
  }

  if (auto *CI = dyn_cast<CastInst>(V)) {
    collectStackSaveOrigins(CI->getOperand(0), F, Origins, Visited);
    return;
  }

  auto *LI = dyn_cast<LoadInst>(V);
  if (!LI)
    return;

  const Value *Slot = getUnderlyingObject(LI->getPointerOperand());
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *Store = dyn_cast<StoreInst>(&I);
      if (!Store ||
          getUnderlyingObject(Store->getPointerOperand()) != Slot)
        continue;
      collectStackSaveOrigins(Store->getValueOperand(), F, Origins, Visited);
    }
  }
}

static void insertMainExitHook(Module &M, Function &F, Instruction *IP,
                               FunctionCallee Exit) {
  IRBuilder<> B(IP);
  Value *Scope = B.CreateGlobalString(F.getName(), "__implicit_scope");
  B.CreateCall(Exit, Scope);
}

static void instrumentFunctionExits(Module &M, Function &F,
                                    ArrayRef<VariableInfo> Vars) {
  if (F.getName() != "main")
    return;

  FunctionCallee Exit = getExitFunction(M);

  // A VLA's storage is commonly released by llvm.stackrestore *before* the
  // function's ret. Snapshot at the restore that releases a tracked main-scope
  // VLA, while every tracked address and its DFSan shadow are still valid.
  DominatorTree DT(F);
  SmallPtrSet<const IntrinsicInst *, 8> TrackedArraySaves;
  bool HasTrackedVLA = false;
  bool HasUnassociatedVLA = false;
  for (const VariableInfo &Var : Vars) {
    if (!Var.DynamicArrayCount || !Var.Alloca)
      continue;

    HasTrackedVLA = true;
    IntrinsicInst *Save = findNearestDominatingStackSave(Var.Alloca, DT);
    if (Save)
      TrackedArraySaves.insert(Save);
    else
      HasUnassociatedVLA = true;
  }

  SmallVector<IntrinsicInst *, 8> RestoreHooks;
  bool HasStackRestore = false;
  bool HasUnresolvedStackRestore = false;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *Restore = dyn_cast<IntrinsicInst>(&I);
      if (!Restore || Restore->getIntrinsicID() != Intrinsic::stackrestore)
        continue;

      HasStackRestore = true;
      SmallPtrSet<const IntrinsicInst *, 8> Origins;
      SmallPtrSet<const Value *, 32> Visited;
      collectStackSaveOrigins(Restore->getArgOperand(0), F, Origins, Visited);

      if (Origins.empty()) {
        HasUnresolvedStackRestore = true;
        continue;
      }

      bool ReleasesTrackedVLA = false;
      for (const IntrinsicInst *Origin : Origins) {
        if (TrackedArraySaves.count(Origin) != 0) {
          ReleasesTrackedVLA = true;
        }
      }
      if (ReleasesTrackedVLA)
        RestoreHooks.push_back(Restore);
    }
  }

  // Failing closed is safer than silently producing a report from a pointer
  // that may already have been invalidated by an unrecognized stackrestore.
  if (HasStackRestore && HasTrackedVLA &&
      (HasUnassociatedVLA || HasUnresolvedStackRestore)) {
    report_fatal_error(
        "implicit-taint: cannot safely associate a tracked VLA with "
        "llvm.stackrestore; refusing to snapshot potentially expired storage");
  }

  for (IntrinsicInst *Restore : RestoreHooks)
    insertMainExitHook(M, F, Restore, Exit);

  // Keep a return hook for paths that return before a VLA is allocated, and
  // for frontends that leave the dynamic allocation live until function exit.
  // If a restore hook already ran, the runtime has popped this frame, so this
  // second call is harmless.
  for (BasicBlock &BB : F) {
    auto *RI = dyn_cast<ReturnInst>(BB.getTerminator());
    if (RI)
      insertMainExitHook(M, F, RI, Exit);
  }
}

// Run before DFSan instrumentation. The dynamic second pass adds inferred
// labels to tracked main-local memory, so make only those eligible allocas
// escape and force DFSan to use addressable shadow memory for them.
struct EscapeMainAllocasForDFSan
    : public PassInfoMixin<EscapeMainAllocasForDFSan> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    Function *F = M.getFunction("main");
    if (!F || F->isDeclaration())
      return PreservedAnalyses::all();

    SmallVector<VariableInfo, 64> Vars;
    collectVariables(*F, Vars);
    SmallPtrSet<AllocaInst *, 32> Seen;
    SmallVector<AllocaInst *, 64> EligibleAllocas;
    for (const VariableInfo &Var : Vars) {
      if (Var.Alloca && Seen.insert(Var.Alloca).second)
        EligibleAllocas.push_back(Var.Alloca);
    }
    if (EligibleAllocas.empty())
      return PreservedAnalyses::all();

    LLVMContext &Ctx = M.getContext();
    Type *VoidTy = Type::getVoidTy(Ctx);
    for (AllocaInst *AI : EligibleAllocas) {
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
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    if (F.isDeclaration() || F.empty() || F.getName() != "main")
      return PreservedAnalyses::all();

    Module *M = F.getParent();
    if (!M)
      return PreservedAnalyses::all();

    SmallVector<VariableInfo, 64> Variables;
    collectVariables(F, Variables);
    instrumentFunctionEntryAndVariables(*M, F, Variables);
    instrumentMainVariableStores(*M, F);
    instrumentFunctionExits(*M, F, Variables);
    return PreservedAnalyses::none();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ImplicitTaintPropagation",
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // This must run before DFSan instrumentation so eligible main-local
            // variables receive addressable shadow memory for inferred labels.
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                  MPM.addPass(EscapeMainAllocasForDFSan());
                });

            // `implicit-taint` is a project-defined pipeline spelling, not a
            // built-in LLVM pass. run.sh invokes it with opt -passes=...
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
