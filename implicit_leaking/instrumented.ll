; ModuleID = 'dfsan.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"Condition 1: true\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [27 x i8] c"Nested Tainted Condition 1\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [27 x i8] c"Nested Tainted Condition 2\00", align 1, !dbg !12
@.str.3 = private unnamed_addr constant [19 x i8] c"Condition 2: true\0A\00", align 1, !dbg !14
@.str.4 = private unnamed_addr constant [12 x i8] c"Z is clean\0A\00", align 1, !dbg !16
@.str.5 = private unnamed_addr constant [10 x i8] c"Z is zero\00", align 1, !dbg !21
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1
@1 = private unnamed_addr constant [2 x i8] c"x\00", align 1
@2 = private unnamed_addr constant [2 x i8] c"y\00", align 1
@3 = private unnamed_addr constant [2 x i8] c"z\00", align 1
@4 = private unnamed_addr constant [2 x i8] c"y\00", align 1
@5 = private unnamed_addr constant [2 x i8] c"z\00", align 1
@6 = private unnamed_addr constant [2 x i8] c"y\00", align 1
@7 = private unnamed_addr constant [2 x i8] c"y\00", align 1
@8 = private unnamed_addr constant [2 x i8] c"z\00", align 1
@9 = private unnamed_addr constant [2 x i8] c"z\00", align 1
@10 = private unnamed_addr constant [2 x i8] c"z\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !37 {
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
    #dbg_declare(ptr %3, !42, !DIExpression(), !43)
  %11 = ptrtoint ptr %3 to i64, !dbg !43
  %12 = xor i64 %11, 87960930222080, !dbg !43
  %13 = inttoptr i64 %12 to ptr, !dbg !43
  store i32 0, ptr %13, align 1, !dbg !43
  store i32 5, ptr %3, align 4, !dbg !43
    #dbg_declare(ptr %4, !44, !DIExpression(), !45)
  %14 = ptrtoint ptr %4 to i64, !dbg !45
  %15 = xor i64 %14, 87960930222080, !dbg !45
  %16 = inttoptr i64 %15 to ptr, !dbg !45
  store i32 0, ptr %16, align 1, !dbg !45
  store i32 8, ptr %4, align 4, !dbg !45
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !46
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !47
    #dbg_declare(ptr %6, !48, !DIExpression(), !49)
  %17 = ptrtoint ptr %3 to i64, !dbg !50
  %18 = xor i64 %17, 87960930222080, !dbg !50
  %19 = inttoptr i64 %18 to ptr, !dbg !50
  %20 = load i32, ptr %19, align 1, !dbg !50
  %21 = lshr i32 %20, 16, !dbg !50
  %22 = or i32 %20, %21, !dbg !50
  %23 = lshr i32 %22, 8, !dbg !50
  %24 = or i32 %22, %23, !dbg !50
  %25 = trunc i32 %24 to i8, !dbg !50
  %26 = load i32, ptr %3, align 4, !dbg !50
  store i8 %25, ptr %5, align 1, !dbg !49
  store i32 %26, ptr %6, align 4, !dbg !49
    #dbg_declare(ptr %8, !51, !DIExpression(), !52)
  store i8 0, ptr %7, align 1, !dbg !52
  store i32 5, ptr %8, align 4, !dbg !52
    #dbg_declare(ptr %10, !53, !DIExpression(), !54)
  store i8 0, ptr %9, align 1, !dbg !54
  store i32 12, ptr %10, align 4, !dbg !54
  %27 = load i8, ptr %5, align 1, !dbg !55
  %28 = load i32, ptr %6, align 4, !dbg !55
  %29 = load i8, ptr %7, align 1, !dbg !57
  %30 = load i32, ptr %8, align 4, !dbg !57
  %31 = or i8 %27, %29, !dbg !58
  %32 = add nsw i32 %28, %30, !dbg !58
  %33 = load i8, ptr %9, align 1, !dbg !59
  %34 = load i32, ptr %10, align 4, !dbg !59
  %35 = or i8 %31, %33, !dbg !60
  %36 = add nsw i32 %32, %34, !dbg !60
  %37 = icmp sgt i32 %36, 10, !dbg !61
  %x.dfsan.label = load i8, ptr %5, align 1, !dbg !61
  call void @__implicit_branch_callback(i8 %35, i8 %x.dfsan.label, i32 23, i32 19, ptr @1), !dbg !61
  %y.dfsan.label = load i8, ptr %7, align 1, !dbg !61
  call void @__implicit_branch_callback(i8 %35, i8 %y.dfsan.label, i32 23, i32 19, ptr @2), !dbg !61
  %z.dfsan.label = load i8, ptr %9, align 1, !dbg !61
  call void @__implicit_branch_callback(i8 %35, i8 %z.dfsan.label, i32 23, i32 19, ptr @3), !dbg !61
  br i1 %37, label %38, label %56, !dbg !61

38:                                               ; preds = %0
  %39 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !62
  %40 = load i8, ptr %7, align 1, !dbg !64
  %41 = load i32, ptr %8, align 4, !dbg !64
  %42 = load i8, ptr %9, align 1, !dbg !66
  %43 = load i32, ptr %10, align 4, !dbg !66
  %44 = or i8 %40, %42, !dbg !67
  %45 = add nsw i32 %41, %43, !dbg !67
  %46 = icmp sgt i32 %45, 10, !dbg !68
  %y.dfsan.label1 = load i8, ptr %7, align 1, !dbg !68
  call void @__implicit_branch_callback(i8 %44, i8 %y.dfsan.label1, i32 26, i32 16, ptr @4), !dbg !68
  %z.dfsan.label2 = load i8, ptr %9, align 1, !dbg !68
  call void @__implicit_branch_callback(i8 %44, i8 %z.dfsan.label2, i32 26, i32 16, ptr @5), !dbg !68
  br i1 %46, label %47, label %55, !dbg !68

47:                                               ; preds = %38
  %48 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !69
  %49 = load i8, ptr %7, align 1, !dbg !71
  %50 = load i32, ptr %8, align 4, !dbg !71
  %51 = icmp sgt i32 %50, 10, !dbg !73
  %y.dfsan.label3 = load i8, ptr %7, align 1, !dbg !73
  call void @__implicit_branch_callback(i8 %49, i8 %y.dfsan.label3, i32 29, i32 18, ptr @6), !dbg !73
  br i1 %51, label %52, label %54, !dbg !73

52:                                               ; preds = %47
  %53 = call i32 (ptr, ...) @printf(ptr noundef @.str.2), !dbg !74
  br label %54, !dbg !76

54:                                               ; preds = %52, %47
  br label %55, !dbg !77

55:                                               ; preds = %54, %38
  br label %56, !dbg !78

56:                                               ; preds = %55, %0
  %57 = load i8, ptr %7, align 1, !dbg !79
  %58 = load i32, ptr %8, align 4, !dbg !79
  %59 = load i8, ptr %9, align 1, !dbg !81
  %60 = load i32, ptr %10, align 4, !dbg !81
  %61 = or i8 %57, %59, !dbg !82
  %62 = add nsw i32 %58, %60, !dbg !82
  %63 = icmp sgt i32 %62, 20, !dbg !83
  %y.dfsan.label4 = load i8, ptr %7, align 1, !dbg !83
  call void @__implicit_branch_callback(i8 %61, i8 %y.dfsan.label4, i32 35, i32 15, ptr @7), !dbg !83
  %z.dfsan.label5 = load i8, ptr %9, align 1, !dbg !83
  call void @__implicit_branch_callback(i8 %61, i8 %z.dfsan.label5, i32 35, i32 15, ptr @8), !dbg !83
  br i1 %63, label %64, label %66, !dbg !83

64:                                               ; preds = %56
  %65 = call i32 (ptr, ...) @printf(ptr noundef @.str.3), !dbg !84
  br label %66, !dbg !86

66:                                               ; preds = %64, %56
  %67 = load i8, ptr %9, align 1, !dbg !87
  %68 = load i32, ptr %10, align 4, !dbg !87
  %69 = icmp sgt i32 %68, 20, !dbg !89
  %z.dfsan.label6 = load i8, ptr %9, align 1, !dbg !89
  call void @__implicit_branch_callback(i8 %67, i8 %z.dfsan.label6, i32 39, i32 11, ptr @9), !dbg !89
  br i1 %69, label %70, label %72, !dbg !89

70:                                               ; preds = %66
  %71 = call i32 (ptr, ...) @printf(ptr noundef @.str.4), !dbg !90
  br label %72, !dbg !92

72:                                               ; preds = %70, %66
  %73 = load i8, ptr %5, align 1, !dbg !93
  %74 = load i32, ptr %6, align 4, !dbg !93
  %75 = load i8, ptr %7, align 1, !dbg !94
  %76 = load i32, ptr %8, align 4, !dbg !94
  %77 = or i8 %73, %75, !dbg !95
  %78 = add nsw i32 %74, %76, !dbg !95
  store i8 %77, ptr %9, align 1, !dbg !96
  store i32 %78, ptr %10, align 4, !dbg !96
  %79 = load i8, ptr %9, align 1, !dbg !97
  %80 = load i32, ptr %10, align 4, !dbg !97
  %81 = icmp eq i32 %80, 0, !dbg !99
  %z.dfsan.label7 = load i8, ptr %9, align 1, !dbg !99
  call void @__implicit_branch_callback(i8 %79, i8 %z.dfsan.label7, i32 45, i32 10, ptr @10), !dbg !99
  br i1 %81, label %82, label %84, !dbg !99

82:                                               ; preds = %72
  %83 = call i32 (ptr, ...) @printf(ptr noundef @.str.5), !dbg !100
  br label %84, !dbg !102

84:                                               ; preds = %82, %72
  ret i32 0, !dbg !103
}

declare void @dfsan_set_label(i8 noundef zeroext, ptr noundef, i64 noundef) #1

declare i32 @printf(ptr noundef, ...) #1

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

define linkonce_odr i32 @"dfsw$printf"(ptr noundef %0, ...) #1 {
  call void @__dfsan_vararg_wrapper(ptr @0)
  unreachable
}

declare void @__implicit_branch_callback(i8, i8, i32, i32, ptr)

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!26}
!llvm.module.flags = !{!28, !29, !30, !31, !32, !33, !34, !35}
!llvm.ident = !{!36}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 24, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "865e0586a60b5b7faafb43c0581821ef")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 19)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 27, type: !9, isLocal: true, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 216, elements: !10)
!10 = !{!11}
!11 = !DISubrange(count: 27)
!12 = !DIGlobalVariableExpression(var: !13, expr: !DIExpression())
!13 = distinct !DIGlobalVariable(scope: null, file: !2, line: 30, type: !9, isLocal: true, isDefinition: true)
!14 = !DIGlobalVariableExpression(var: !15, expr: !DIExpression())
!15 = distinct !DIGlobalVariable(scope: null, file: !2, line: 36, type: !3, isLocal: true, isDefinition: true)
!16 = !DIGlobalVariableExpression(var: !17, expr: !DIExpression())
!17 = distinct !DIGlobalVariable(scope: null, file: !2, line: 40, type: !18, isLocal: true, isDefinition: true)
!18 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 96, elements: !19)
!19 = !{!20}
!20 = !DISubrange(count: 12)
!21 = !DIGlobalVariableExpression(var: !22, expr: !DIExpression())
!22 = distinct !DIGlobalVariable(scope: null, file: !2, line: 46, type: !23, isLocal: true, isDefinition: true)
!23 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 80, elements: !24)
!24 = !{!25}
!25 = !DISubrange(count: 10)
!26 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !27, splitDebugInlining: false, nameTableKind: None)
!27 = !{!0, !7, !12, !14, !16, !21}
!28 = !{i32 7, !"Dwarf Version", i32 5}
!29 = !{i32 2, !"Debug Info Version", i32 3}
!30 = !{i32 1, !"wchar_size", i32 4}
!31 = !{i32 8, !"PIC Level", i32 2}
!32 = !{i32 7, !"PIE Level", i32 2}
!33 = !{i32 7, !"uwtable", i32 2}
!34 = !{i32 7, !"frame-pointer", i32 2}
!35 = !{i32 4, !"nosanitize_dataflow", i32 1}
!36 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!37 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !38, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !26, retainedNodes: !41)
!38 = !DISubroutineType(types: !39)
!39 = !{!40}
!40 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!41 = !{}
!42 = !DILocalVariable(name: "secret1", scope: !37, file: !2, line: 6, type: !40)
!43 = !DILocation(line: 6, column: 9, scope: !37)
!44 = !DILocalVariable(name: "secret2", scope: !37, file: !2, line: 7, type: !40)
!45 = !DILocation(line: 7, column: 9, scope: !37)
!46 = !DILocation(line: 9, column: 5, scope: !37)
!47 = !DILocation(line: 14, column: 5, scope: !37)
!48 = !DILocalVariable(name: "x", scope: !37, file: !2, line: 19, type: !40)
!49 = !DILocation(line: 19, column: 9, scope: !37)
!50 = !DILocation(line: 19, column: 13, scope: !37)
!51 = !DILocalVariable(name: "y", scope: !37, file: !2, line: 20, type: !40)
!52 = !DILocation(line: 20, column: 9, scope: !37)
!53 = !DILocalVariable(name: "z", scope: !37, file: !2, line: 21, type: !40)
!54 = !DILocation(line: 21, column: 9, scope: !37)
!55 = !DILocation(line: 23, column: 9, scope: !56)
!56 = distinct !DILexicalBlock(scope: !37, file: !2, line: 23, column: 9)
!57 = !DILocation(line: 23, column: 13, scope: !56)
!58 = !DILocation(line: 23, column: 11, scope: !56)
!59 = !DILocation(line: 23, column: 17, scope: !56)
!60 = !DILocation(line: 23, column: 15, scope: !56)
!61 = !DILocation(line: 23, column: 19, scope: !56)
!62 = !DILocation(line: 24, column: 9, scope: !63)
!63 = distinct !DILexicalBlock(scope: !56, file: !2, line: 23, column: 25)
!64 = !DILocation(line: 26, column: 12, scope: !65)
!65 = distinct !DILexicalBlock(scope: !63, file: !2, line: 26, column: 12)
!66 = !DILocation(line: 26, column: 14, scope: !65)
!67 = !DILocation(line: 26, column: 13, scope: !65)
!68 = !DILocation(line: 26, column: 16, scope: !65)
!69 = !DILocation(line: 27, column: 13, scope: !70)
!70 = distinct !DILexicalBlock(scope: !65, file: !2, line: 26, column: 21)
!71 = !DILocation(line: 29, column: 16, scope: !72)
!72 = distinct !DILexicalBlock(scope: !70, file: !2, line: 29, column: 16)
!73 = !DILocation(line: 29, column: 18, scope: !72)
!74 = !DILocation(line: 30, column: 17, scope: !75)
!75 = distinct !DILexicalBlock(scope: !72, file: !2, line: 29, column: 23)
!76 = !DILocation(line: 31, column: 13, scope: !75)
!77 = !DILocation(line: 32, column: 9, scope: !70)
!78 = !DILocation(line: 33, column: 5, scope: !63)
!79 = !DILocation(line: 35, column: 9, scope: !80)
!80 = distinct !DILexicalBlock(scope: !37, file: !2, line: 35, column: 9)
!81 = !DILocation(line: 35, column: 13, scope: !80)
!82 = !DILocation(line: 35, column: 11, scope: !80)
!83 = !DILocation(line: 35, column: 15, scope: !80)
!84 = !DILocation(line: 36, column: 9, scope: !85)
!85 = distinct !DILexicalBlock(scope: !80, file: !2, line: 35, column: 21)
!86 = !DILocation(line: 37, column: 5, scope: !85)
!87 = !DILocation(line: 39, column: 9, scope: !88)
!88 = distinct !DILexicalBlock(scope: !37, file: !2, line: 39, column: 9)
!89 = !DILocation(line: 39, column: 11, scope: !88)
!90 = !DILocation(line: 40, column: 9, scope: !91)
!91 = distinct !DILexicalBlock(scope: !88, file: !2, line: 39, column: 17)
!92 = !DILocation(line: 41, column: 5, scope: !91)
!93 = !DILocation(line: 43, column: 9, scope: !37)
!94 = !DILocation(line: 43, column: 13, scope: !37)
!95 = !DILocation(line: 43, column: 11, scope: !37)
!96 = !DILocation(line: 43, column: 7, scope: !37)
!97 = !DILocation(line: 45, column: 8, scope: !98)
!98 = distinct !DILexicalBlock(scope: !37, file: !2, line: 45, column: 8)
!99 = !DILocation(line: 45, column: 10, scope: !98)
!100 = !DILocation(line: 46, column: 9, scope: !101)
!101 = distinct !DILexicalBlock(scope: !98, file: !2, line: 45, column: 15)
!102 = !DILocation(line: 47, column: 5, scope: !101)
!103 = !DILocation(line: 49, column: 5, scope: !37)
