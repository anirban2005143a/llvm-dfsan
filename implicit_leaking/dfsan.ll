; ModuleID = 'instrumented.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main.dfsan() #0 !dbg !11 {
  %1 = alloca i8, align 1
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
    #dbg_declare(ptr %3, !16, !DIExpression(), !17)
  %4 = ptrtoint ptr %3 to i64, !dbg !17
  %5 = xor i64 %4, 87960930222080, !dbg !17
  %6 = inttoptr i64 %5 to ptr, !dbg !17
  store i32 0, ptr %6, align 1, !dbg !17
  store i32 5, ptr %3, align 4, !dbg !17
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !18
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !18
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !18
  call void @dfsan_set_label.dfsan(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !18
  %7 = ptrtoint ptr %3 to i64, !dbg !19
  %8 = xor i64 %7, 87960930222080, !dbg !19
  %9 = inttoptr i64 %8 to ptr, !dbg !19
  %10 = load i32, ptr %9, align 1, !dbg !19
  %11 = lshr i32 %10, 16, !dbg !19
  %12 = or i32 %10, %11, !dbg !19
  %13 = lshr i32 %12, 8, !dbg !19
  %14 = or i32 %12, %13, !dbg !19
  %15 = trunc i32 %14 to i8, !dbg !19
  %16 = load i32, ptr %3, align 4, !dbg !19
  %17 = icmp sgt i32 %16, 2, !dbg !21
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !21
  call void @__implicit_condition_id.dfsan(i32 1), !dbg !21
  call void @__dfsan_conditional_callback(i8 zeroext %15), !dbg !21
  br i1 %17, label %18, label %19, !dbg !21

18:                                               ; preds = %0
  store i8 0, ptr %1, align 1, !dbg !22
  store i32 1, ptr %2, align 4, !dbg !22
  br label %20, !dbg !22

19:                                               ; preds = %0
  store i8 0, ptr %1, align 1, !dbg !24
  store i32 0, ptr %2, align 4, !dbg !24
  br label %20, !dbg !24

20:                                               ; preds = %19, %18
  %21 = load i8, ptr %1, align 1, !dbg !25
  %22 = load i32, ptr %2, align 4, !dbg !25
  store i8 %21, ptr @__dfsan_retval_tls, align 2, !dbg !25
  ret i32 %22, !dbg !25
}

declare void @dfsan_set_label.dfsan(i8 noundef zeroext, ptr noundef, i64 noundef) #1

declare void @__implicit_condition_id.dfsan(i32)

declare void @__dfsan_load_callback(i8 zeroext, ptr)

declare void @__dfsan_store_callback(i8 zeroext, ptr)

declare void @__dfsan_mem_transfer_callback(ptr, i64)

declare void @__dfsan_cmp_callback(i8 zeroext)

declare void @__dfsan_conditional_callback(i8 zeroext)

declare void @__dfsan_conditional_callback_origin(i8 zeroext, i32)

declare void @__dfsan_reaches_function_callback(i8 zeroext, ptr, i32, ptr)

declare void @__dfsan_reaches_function_callback_origin(i8 zeroext, i32, ptr, i32, ptr)

; Function Attrs: nounwind memory(read)
declare zeroext i8 @__dfsan_union_load(ptr, i64) #2

; Function Attrs: nounwind memory(read)
declare zeroext i64 @__dfsan_load_label_and_origin(ptr, i64) #2

declare void @__dfsan_unimplemented(ptr)

declare void @__dfsan_wrapper_extern_weak_null(ptr, ptr)

declare void @__dfsan_set_label(i8 zeroext, i32 zeroext, ptr, i64)

declare void @__dfsan_nonzero_label()

declare void @__dfsan_vararg_wrapper(ptr)

declare zeroext i32 @__dfsan_chain_origin(i32 zeroext)

declare zeroext i32 @__dfsan_chain_origin_if_tainted(i8 zeroext, i32 zeroext)

declare void @__dfsan_mem_origin_transfer(ptr, ptr, i64)

declare void @__dfsan_mem_shadow_origin_transfer(ptr, ptr, i64)

declare void @__dfsan_mem_shadow_origin_conditional_exchange(i8, ptr, ptr, ptr, i64)

declare void @__dfsan_maybe_store_origin(i8 zeroext, ptr, i64, i32 zeroext)

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8, !9}
!llvm.ident = !{!10}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "ad6d62b7d32e55ed0496d08fd18a8458")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 8, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{i32 4, !"nosanitize_dataflow", i32 1}
!10 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!11 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !12, scopeLine: 3, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !15)
!12 = !DISubroutineType(types: !13)
!13 = !{!14}
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !{}
!16 = !DILocalVariable(name: "x", scope: !11, file: !1, line: 4, type: !14)
!17 = !DILocation(line: 4, column: 9, scope: !11)
!18 = !DILocation(line: 6, column: 5, scope: !11)
!19 = !DILocation(line: 8, column: 9, scope: !20)
!20 = distinct !DILexicalBlock(scope: !11, file: !1, line: 8, column: 9)
!21 = !DILocation(line: 8, column: 11, scope: !20)
!22 = !DILocation(line: 9, column: 9, scope: !23)
!23 = distinct !DILexicalBlock(scope: !20, file: !1, line: 8, column: 16)
!24 = !DILocation(line: 12, column: 5, scope: !11)
!25 = !DILocation(line: 13, column: 1, scope: !11)
