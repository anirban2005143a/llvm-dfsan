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
  %11 = alloca i8, align 1
  %12 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
    #dbg_declare(ptr %3, !16, !DIExpression(), !17)
  %13 = ptrtoint ptr %3 to i64, !dbg !17
  %14 = xor i64 %13, 87960930222080, !dbg !17
  %15 = inttoptr i64 %14 to ptr, !dbg !17
  store i32 0, ptr %15, align 1, !dbg !17
  store i32 5, ptr %3, align 4, !dbg !17
    #dbg_declare(ptr %4, !18, !DIExpression(), !19)
  %16 = ptrtoint ptr %4 to i64, !dbg !19
  %17 = xor i64 %16, 87960930222080, !dbg !19
  %18 = inttoptr i64 %17 to ptr, !dbg !19
  store i32 0, ptr %18, align 1, !dbg !19
  store i32 8, ptr %4, align 4, !dbg !19
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !20
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !21
    #dbg_declare(ptr %6, !22, !DIExpression(), !23)
  %19 = ptrtoint ptr %3 to i64, !dbg !24
  %20 = xor i64 %19, 87960930222080, !dbg !24
  %21 = inttoptr i64 %20 to ptr, !dbg !24
  %22 = load i32, ptr %21, align 1, !dbg !24
  %23 = lshr i32 %22, 16, !dbg !24
  %24 = or i32 %22, %23, !dbg !24
  %25 = lshr i32 %24, 8, !dbg !24
  %26 = or i32 %24, %25, !dbg !24
  %27 = trunc i32 %26 to i8, !dbg !24
  %28 = load i32, ptr %3, align 4, !dbg !24
  store i8 %27, ptr %5, align 1, !dbg !23
  store i32 %28, ptr %6, align 4, !dbg !23
    #dbg_declare(ptr %8, !25, !DIExpression(), !26)
  %29 = ptrtoint ptr %4 to i64, !dbg !27
  %30 = xor i64 %29, 87960930222080, !dbg !27
  %31 = inttoptr i64 %30 to ptr, !dbg !27
  %32 = load i32, ptr %31, align 1, !dbg !27
  %33 = lshr i32 %32, 16, !dbg !27
  %34 = or i32 %32, %33, !dbg !27
  %35 = lshr i32 %34, 8, !dbg !27
  %36 = or i32 %34, %35, !dbg !27
  %37 = trunc i32 %36 to i8, !dbg !27
  %38 = load i32, ptr %4, align 4, !dbg !27
  store i8 %37, ptr %7, align 1, !dbg !26
  store i32 %38, ptr %8, align 4, !dbg !26
    #dbg_declare(ptr %10, !28, !DIExpression(), !29)
  store i8 0, ptr %9, align 1, !dbg !29
  store i32 12, ptr %10, align 4, !dbg !29
    #dbg_declare(ptr %12, !30, !DIExpression(), !32)
  store i8 0, ptr %11, align 1, !dbg !32
  store volatile i32 0, ptr %12, align 4, !dbg !32
  %39 = load i8, ptr %5, align 1, !dbg !33
  %40 = load i32, ptr %6, align 4, !dbg !33
  %41 = load i8, ptr %7, align 1, !dbg !35
  %42 = load i32, ptr %8, align 4, !dbg !35
  %43 = or i8 %39, %41, !dbg !36
  %44 = add nsw i32 %40, %42, !dbg !36
  %45 = load i8, ptr %9, align 1, !dbg !37
  %46 = load i32, ptr %10, align 4, !dbg !37
  %47 = or i8 %43, %45, !dbg !38
  %48 = add nsw i32 %44, %46, !dbg !38
  %49 = icmp sgt i32 %48, 10, !dbg !39
  call void @__dfsan_conditional_callback(i8 zeroext %47), !dbg !39
  br i1 %49, label %50, label %51, !dbg !39

50:                                               ; preds = %0
  store i8 0, ptr %11, align 1, !dbg !40
  store volatile i32 1, ptr %12, align 4, !dbg !40
  br label %51, !dbg !42

51:                                               ; preds = %50, %0
  %52 = load i8, ptr %7, align 1, !dbg !43
  %53 = load i32, ptr %8, align 4, !dbg !43
  %54 = load i8, ptr %9, align 1, !dbg !45
  %55 = load i32, ptr %10, align 4, !dbg !45
  %56 = or i8 %52, %54, !dbg !46
  %57 = add nsw i32 %53, %55, !dbg !46
  %58 = icmp sgt i32 %57, 20, !dbg !47
  call void @__dfsan_conditional_callback(i8 zeroext %56), !dbg !47
  br i1 %58, label %59, label %60, !dbg !47

59:                                               ; preds = %51
  store i8 0, ptr %11, align 1, !dbg !48
  store volatile i32 2, ptr %12, align 4, !dbg !48
  br label %60, !dbg !50

60:                                               ; preds = %59, %51
  %61 = load i8, ptr %9, align 1, !dbg !51
  %62 = load i32, ptr %10, align 4, !dbg !51
  %63 = icmp sgt i32 %62, 20, !dbg !53
  call void @__dfsan_conditional_callback(i8 zeroext %61), !dbg !53
  br i1 %63, label %64, label %65, !dbg !53

64:                                               ; preds = %60
  store i8 0, ptr %11, align 1, !dbg !54
  store volatile i32 3, ptr %12, align 4, !dbg !54
  br label %65, !dbg !56

65:                                               ; preds = %64, %60
  %66 = load i8, ptr %11, align 1, !dbg !57
  %67 = load volatile i32, ptr %12, align 4, !dbg !57
  ret i32 %67, !dbg !58
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
!1 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "398d7ca1467630a75132bdace960efb8")
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
!20 = !DILocation(line: 8, column: 5, scope: !11)
!21 = !DILocation(line: 13, column: 5, scope: !11)
!22 = !DILocalVariable(name: "x", scope: !11, file: !1, line: 18, type: !14)
!23 = !DILocation(line: 18, column: 9, scope: !11)
!24 = !DILocation(line: 18, column: 13, scope: !11)
!25 = !DILocalVariable(name: "y", scope: !11, file: !1, line: 19, type: !14)
!26 = !DILocation(line: 19, column: 9, scope: !11)
!27 = !DILocation(line: 19, column: 13, scope: !11)
!28 = !DILocalVariable(name: "z", scope: !11, file: !1, line: 20, type: !14)
!29 = !DILocation(line: 20, column: 9, scope: !11)
!30 = !DILocalVariable(name: "sink", scope: !11, file: !1, line: 22, type: !31)
!31 = !DIDerivedType(tag: DW_TAG_volatile_type, baseType: !14)
!32 = !DILocation(line: 22, column: 18, scope: !11)
!33 = !DILocation(line: 24, column: 9, scope: !34)
!34 = distinct !DILexicalBlock(scope: !11, file: !1, line: 24, column: 9)
!35 = !DILocation(line: 24, column: 13, scope: !34)
!36 = !DILocation(line: 24, column: 11, scope: !34)
!37 = !DILocation(line: 24, column: 17, scope: !34)
!38 = !DILocation(line: 24, column: 15, scope: !34)
!39 = !DILocation(line: 24, column: 19, scope: !34)
!40 = !DILocation(line: 25, column: 14, scope: !41)
!41 = distinct !DILexicalBlock(scope: !34, file: !1, line: 24, column: 25)
!42 = !DILocation(line: 26, column: 5, scope: !41)
!43 = !DILocation(line: 28, column: 9, scope: !44)
!44 = distinct !DILexicalBlock(scope: !11, file: !1, line: 28, column: 9)
!45 = !DILocation(line: 28, column: 13, scope: !44)
!46 = !DILocation(line: 28, column: 11, scope: !44)
!47 = !DILocation(line: 28, column: 15, scope: !44)
!48 = !DILocation(line: 29, column: 14, scope: !49)
!49 = distinct !DILexicalBlock(scope: !44, file: !1, line: 28, column: 21)
!50 = !DILocation(line: 30, column: 5, scope: !49)
!51 = !DILocation(line: 32, column: 9, scope: !52)
!52 = distinct !DILexicalBlock(scope: !11, file: !1, line: 32, column: 9)
!53 = !DILocation(line: 32, column: 11, scope: !52)
!54 = !DILocation(line: 33, column: 14, scope: !55)
!55 = distinct !DILexicalBlock(scope: !52, file: !1, line: 32, column: 17)
!56 = !DILocation(line: 34, column: 5, scope: !55)
!57 = !DILocation(line: 36, column: 12, scope: !11)
!58 = !DILocation(line: 36, column: 5, scope: !11)
