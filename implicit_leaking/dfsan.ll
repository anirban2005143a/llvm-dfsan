; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !11 {
  %1 = alloca i8, align 1
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8, align 1
  %6 = alloca i32, align 4
  %7 = alloca i8, align 1
  %8 = alloca i32, align 4
  %9 = alloca i8, align 1
  %10 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
    #dbg_declare(ptr %3, !16, !DIExpression(), !17)
  %11 = ptrtoint ptr %3 to i64, !dbg !17
  %12 = xor i64 %11, 87960930222080, !dbg !17
  %13 = inttoptr i64 %12 to ptr, !dbg !17
  store i32 0, ptr %13, align 1, !dbg !17
  store i32 5, ptr %3, align 4, !dbg !17
    #dbg_declare(ptr %4, !18, !DIExpression(), !19)
  %14 = ptrtoint ptr %4 to i64, !dbg !19
  %15 = xor i64 %14, 87960930222080, !dbg !19
  %16 = inttoptr i64 %15 to ptr, !dbg !19
  store i32 0, ptr %16, align 1, !dbg !19
  store i32 8, ptr %4, align 4, !dbg !19
    #dbg_declare(ptr %6, !20, !DIExpression(), !21)
  %17 = ptrtoint ptr %3 to i64, !dbg !22
  %18 = xor i64 %17, 87960930222080, !dbg !22
  %19 = inttoptr i64 %18 to ptr, !dbg !22
  %20 = load i32, ptr %19, align 1, !dbg !22
  %21 = lshr i32 %20, 16, !dbg !22
  %22 = or i32 %20, %21, !dbg !22
  %23 = lshr i32 %22, 8, !dbg !22
  %24 = or i32 %22, %23, !dbg !22
  %25 = trunc i32 %24 to i8, !dbg !22
  %26 = load i32, ptr %3, align 4, !dbg !22
  store i8 %25, ptr %5, align 1, !dbg !21
  store i32 %26, ptr %6, align 4, !dbg !21
    #dbg_declare(ptr %8, !23, !DIExpression(), !24)
  %27 = ptrtoint ptr %4 to i64, !dbg !25
  %28 = xor i64 %27, 87960930222080, !dbg !25
  %29 = inttoptr i64 %28 to ptr, !dbg !25
  %30 = load i32, ptr %29, align 1, !dbg !25
  %31 = lshr i32 %30, 16, !dbg !25
  %32 = or i32 %30, %31, !dbg !25
  %33 = lshr i32 %32, 8, !dbg !25
  %34 = or i32 %32, %33, !dbg !25
  %35 = trunc i32 %34 to i8, !dbg !25
  %36 = load i32, ptr %4, align 4, !dbg !25
  store i8 %35, ptr %7, align 1, !dbg !24
  store i32 %36, ptr %8, align 4, !dbg !24
    #dbg_declare(ptr %10, !26, !DIExpression(), !27)
  store i8 0, ptr %9, align 1, !dbg !27
  store i32 12, ptr %10, align 4, !dbg !27
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !28
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !29
  %37 = load i8, ptr %5, align 1, !dbg !30
  %38 = load i32, ptr %6, align 4, !dbg !30
  %39 = load i8, ptr %7, align 1, !dbg !32
  %40 = load i32, ptr %8, align 4, !dbg !32
  %41 = or i8 %37, %39, !dbg !33
  %42 = add nsw i32 %38, %40, !dbg !33
  %43 = load i8, ptr %9, align 1, !dbg !34
  %44 = load i32, ptr %10, align 4, !dbg !34
  %45 = or i8 %41, %43, !dbg !35
  %46 = add nsw i32 %42, %44, !dbg !35
  %47 = icmp sgt i32 %46, 10, !dbg !36
  call void @__dfsan_conditional_callback(i8 zeroext %45), !dbg !36
  br i1 %47, label %48, label %49, !dbg !36

48:                                               ; preds = %0
  store i8 0, ptr %9, align 1, !dbg !37
  store i32 1, ptr %10, align 4, !dbg !37
  br label %49, !dbg !39

49:                                               ; preds = %48, %0
  %50 = load i8, ptr %7, align 1, !dbg !40
  %51 = load i32, ptr %8, align 4, !dbg !40
  %52 = load i8, ptr %9, align 1, !dbg !42
  %53 = load i32, ptr %10, align 4, !dbg !42
  %54 = or i8 %50, %52, !dbg !43
  %55 = add nsw i32 %51, %53, !dbg !43
  %56 = icmp sgt i32 %55, 20, !dbg !44
  call void @__dfsan_conditional_callback(i8 zeroext %54), !dbg !44
  br i1 %56, label %57, label %58, !dbg !44

57:                                               ; preds = %49
  store i8 0, ptr %9, align 1, !dbg !45
  store i32 2, ptr %10, align 4, !dbg !45
  br label %58, !dbg !47

58:                                               ; preds = %57, %49
  %59 = load i8, ptr %9, align 1, !dbg !48
  %60 = load i32, ptr %10, align 4, !dbg !48
  %61 = icmp sgt i32 %60, 20, !dbg !50
  call void @__dfsan_conditional_callback(i8 zeroext %59), !dbg !50
  br i1 %61, label %62, label %63, !dbg !50

62:                                               ; preds = %58
  store i8 0, ptr %9, align 1, !dbg !51
  store i32 3, ptr %10, align 4, !dbg !51
  br label %63, !dbg !53

63:                                               ; preds = %62, %58
  %64 = load i8, ptr %9, align 1, !dbg !54
  %65 = load i32, ptr %10, align 4, !dbg !54
  ret i32 %65, !dbg !55
}

declare void @dfsan_set_label(i8 noundef zeroext, ptr noundef, i64 noundef) #1

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

; Function Attrs: noinline nounwind uwtable
define linkonce_odr dso_local i32 @"dfsw$main"() #0 {
  %1 = call i32 @main()
  store i8 0, ptr @__dfsan_retval_tls, align 2
  ret i32 %1
}

define linkonce_odr void @"dfsw$dfsan_set_label"(i8 noundef zeroext %0, ptr noundef %1, i64 noundef %2) #1 {
  call void @dfsan_set_label(i8 %0, ptr %1, i64 %2)
  ret void
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8, !9}
!llvm.ident = !{!10}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "472ceaca292b0233630553a0d62746d7")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 8, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{i32 4, !"nosanitize_dataflow", i32 1}
!10 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!11 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !12, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !15)
!12 = !DISubroutineType(types: !13)
!13 = !{!14}
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !{}
!16 = !DILocalVariable(name: "secret1", scope: !11, file: !1, line: 5, type: !14)
!17 = !DILocation(line: 5, column: 9, scope: !11)
!18 = !DILocalVariable(name: "secret2", scope: !11, file: !1, line: 6, type: !14)
!19 = !DILocation(line: 6, column: 9, scope: !11)
!20 = !DILocalVariable(name: "x", scope: !11, file: !1, line: 8, type: !14)
!21 = !DILocation(line: 8, column: 9, scope: !11)
!22 = !DILocation(line: 8, column: 13, scope: !11)
!23 = !DILocalVariable(name: "y", scope: !11, file: !1, line: 9, type: !14)
!24 = !DILocation(line: 9, column: 9, scope: !11)
!25 = !DILocation(line: 9, column: 13, scope: !11)
!26 = !DILocalVariable(name: "clean", scope: !11, file: !1, line: 11, type: !14)
!27 = !DILocation(line: 11, column: 9, scope: !11)
!28 = !DILocation(line: 13, column: 5, scope: !11)
!29 = !DILocation(line: 18, column: 5, scope: !11)
!30 = !DILocation(line: 23, column: 9, scope: !31)
!31 = distinct !DILexicalBlock(scope: !11, file: !1, line: 23, column: 9)
!32 = !DILocation(line: 23, column: 13, scope: !31)
!33 = !DILocation(line: 23, column: 11, scope: !31)
!34 = !DILocation(line: 23, column: 17, scope: !31)
!35 = !DILocation(line: 23, column: 15, scope: !31)
!36 = !DILocation(line: 23, column: 23, scope: !31)
!37 = !DILocation(line: 24, column: 15, scope: !38)
!38 = distinct !DILexicalBlock(scope: !31, file: !1, line: 23, column: 29)
!39 = !DILocation(line: 25, column: 5, scope: !38)
!40 = !DILocation(line: 27, column: 9, scope: !41)
!41 = distinct !DILexicalBlock(scope: !11, file: !1, line: 27, column: 9)
!42 = !DILocation(line: 27, column: 13, scope: !41)
!43 = !DILocation(line: 27, column: 11, scope: !41)
!44 = !DILocation(line: 27, column: 19, scope: !41)
!45 = !DILocation(line: 28, column: 15, scope: !46)
!46 = distinct !DILexicalBlock(scope: !41, file: !1, line: 27, column: 25)
!47 = !DILocation(line: 29, column: 5, scope: !46)
!48 = !DILocation(line: 31, column: 9, scope: !49)
!49 = distinct !DILexicalBlock(scope: !11, file: !1, line: 31, column: 9)
!50 = !DILocation(line: 31, column: 15, scope: !49)
!51 = !DILocation(line: 32, column: 15, scope: !52)
!52 = distinct !DILexicalBlock(scope: !49, file: !1, line: 31, column: 21)
!53 = !DILocation(line: 33, column: 5, scope: !52)
!54 = !DILocation(line: 35, column: 12, scope: !11)
!55 = !DILocation(line: 35, column: 5, scope: !11)
