; ModuleID = 'dfsan.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"Condition 1: true\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [19 x i8] c"Condition 2: true\0A\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [12 x i8] c"Z is clean\0A\00", align 1, !dbg !9
@.str.3 = private unnamed_addr constant [10 x i8] c"Z is zero\00", align 1, !dbg !14
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
@6 = private unnamed_addr constant [2 x i8] c"z\00", align 1
@7 = private unnamed_addr constant [2 x i8] c"z\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !30 {
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
    #dbg_declare(ptr %3, !35, !DIExpression(), !36)
  %11 = ptrtoint ptr %3 to i64, !dbg !36
  %12 = xor i64 %11, 87960930222080, !dbg !36
  %13 = inttoptr i64 %12 to ptr, !dbg !36
  store i32 0, ptr %13, align 1, !dbg !36
  store i32 5, ptr %3, align 4, !dbg !36
    #dbg_declare(ptr %4, !37, !DIExpression(), !38)
  %14 = ptrtoint ptr %4 to i64, !dbg !38
  %15 = xor i64 %14, 87960930222080, !dbg !38
  %16 = inttoptr i64 %15 to ptr, !dbg !38
  store i32 0, ptr %16, align 1, !dbg !38
  store i32 8, ptr %4, align 4, !dbg !38
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !39
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !40
    #dbg_declare(ptr %6, !41, !DIExpression(), !42)
  %17 = ptrtoint ptr %3 to i64, !dbg !43
  %18 = xor i64 %17, 87960930222080, !dbg !43
  %19 = inttoptr i64 %18 to ptr, !dbg !43
  %20 = load i32, ptr %19, align 1, !dbg !43
  %21 = lshr i32 %20, 16, !dbg !43
  %22 = or i32 %20, %21, !dbg !43
  %23 = lshr i32 %22, 8, !dbg !43
  %24 = or i32 %22, %23, !dbg !43
  %25 = trunc i32 %24 to i8, !dbg !43
  %26 = load i32, ptr %3, align 4, !dbg !43
  store i8 %25, ptr %5, align 1, !dbg !42
  store i32 %26, ptr %6, align 4, !dbg !42
    #dbg_declare(ptr %8, !44, !DIExpression(), !45)
  store i8 0, ptr %7, align 1, !dbg !45
  store i32 5, ptr %8, align 4, !dbg !45
    #dbg_declare(ptr %10, !46, !DIExpression(), !47)
  store i8 0, ptr %9, align 1, !dbg !47
  store i32 12, ptr %10, align 4, !dbg !47
  %27 = load i8, ptr %5, align 1, !dbg !48
  %28 = load i32, ptr %6, align 4, !dbg !48
  %29 = load i8, ptr %7, align 1, !dbg !50
  %30 = load i32, ptr %8, align 4, !dbg !50
  %31 = or i8 %27, %29, !dbg !51
  %32 = add nsw i32 %28, %30, !dbg !51
  %33 = load i8, ptr %9, align 1, !dbg !52
  %34 = load i32, ptr %10, align 4, !dbg !52
  %35 = or i8 %31, %33, !dbg !53
  %36 = add nsw i32 %32, %34, !dbg !53
  %37 = icmp sgt i32 %36, 10, !dbg !54
  %x.dfsan.label = load i8, ptr %5, align 1, !dbg !54
  call void @__implicit_branch_callback(i8 %35, i8 %x.dfsan.label, i32 23, i32 19, ptr @1), !dbg !54
  %y.dfsan.label = load i8, ptr %7, align 1, !dbg !54
  call void @__implicit_branch_callback(i8 %35, i8 %y.dfsan.label, i32 23, i32 19, ptr @2), !dbg !54
  %z.dfsan.label = load i8, ptr %9, align 1, !dbg !54
  call void @__implicit_branch_callback(i8 %35, i8 %z.dfsan.label, i32 23, i32 19, ptr @3), !dbg !54
  br i1 %37, label %38, label %40, !dbg !54

38:                                               ; preds = %0
  %39 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !55
  br label %40, !dbg !57

40:                                               ; preds = %38, %0
  %41 = load i8, ptr %7, align 1, !dbg !58
  %42 = load i32, ptr %8, align 4, !dbg !58
  %43 = load i8, ptr %9, align 1, !dbg !60
  %44 = load i32, ptr %10, align 4, !dbg !60
  %45 = or i8 %41, %43, !dbg !61
  %46 = add nsw i32 %42, %44, !dbg !61
  %47 = icmp sgt i32 %46, 20, !dbg !62
  %y.dfsan.label1 = load i8, ptr %7, align 1, !dbg !62
  call void @__implicit_branch_callback(i8 %45, i8 %y.dfsan.label1, i32 27, i32 15, ptr @4), !dbg !62
  %z.dfsan.label2 = load i8, ptr %9, align 1, !dbg !62
  call void @__implicit_branch_callback(i8 %45, i8 %z.dfsan.label2, i32 27, i32 15, ptr @5), !dbg !62
  br i1 %47, label %48, label %50, !dbg !62

48:                                               ; preds = %40
  %49 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !63
  br label %50, !dbg !65

50:                                               ; preds = %48, %40
  %51 = load i8, ptr %9, align 1, !dbg !66
  %52 = load i32, ptr %10, align 4, !dbg !66
  %53 = icmp sgt i32 %52, 20, !dbg !68
  %z.dfsan.label3 = load i8, ptr %9, align 1, !dbg !68
  call void @__implicit_branch_callback(i8 %51, i8 %z.dfsan.label3, i32 31, i32 11, ptr @6), !dbg !68
  br i1 %53, label %54, label %56, !dbg !68

54:                                               ; preds = %50
  %55 = call i32 (ptr, ...) @printf(ptr noundef @.str.2), !dbg !69
  br label %56, !dbg !71

56:                                               ; preds = %54, %50
  %57 = load i8, ptr %5, align 1, !dbg !72
  %58 = load i32, ptr %6, align 4, !dbg !72
  %59 = load i8, ptr %7, align 1, !dbg !73
  %60 = load i32, ptr %8, align 4, !dbg !73
  %61 = or i8 %57, %59, !dbg !74
  %62 = add nsw i32 %58, %60, !dbg !74
  store i8 %61, ptr %9, align 1, !dbg !75
  store i32 %62, ptr %10, align 4, !dbg !75
  %63 = load i8, ptr %9, align 1, !dbg !76
  %64 = load i32, ptr %10, align 4, !dbg !76
  %65 = icmp eq i32 %64, 0, !dbg !78
  %z.dfsan.label4 = load i8, ptr %9, align 1, !dbg !78
  call void @__implicit_branch_callback(i8 %63, i8 %z.dfsan.label4, i32 37, i32 10, ptr @7), !dbg !78
  br i1 %65, label %66, label %68, !dbg !78

66:                                               ; preds = %56
  %67 = call i32 (ptr, ...) @printf(ptr noundef @.str.3), !dbg !79
  br label %68, !dbg !81

68:                                               ; preds = %66, %56
  ret i32 0, !dbg !82
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

!llvm.dbg.cu = !{!19}
!llvm.module.flags = !{!21, !22, !23, !24, !25, !26, !27, !28}
!llvm.ident = !{!29}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 24, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "364ba4b84c8fadf09c25254551e0c6fc")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 19)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 28, type: !3, isLocal: true, isDefinition: true)
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(scope: null, file: !2, line: 32, type: !11, isLocal: true, isDefinition: true)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 96, elements: !12)
!12 = !{!13}
!13 = !DISubrange(count: 12)
!14 = !DIGlobalVariableExpression(var: !15, expr: !DIExpression())
!15 = distinct !DIGlobalVariable(scope: null, file: !2, line: 38, type: !16, isLocal: true, isDefinition: true)
!16 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 80, elements: !17)
!17 = !{!18}
!18 = !DISubrange(count: 10)
!19 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !20, splitDebugInlining: false, nameTableKind: None)
!20 = !{!0, !7, !9, !14}
!21 = !{i32 7, !"Dwarf Version", i32 5}
!22 = !{i32 2, !"Debug Info Version", i32 3}
!23 = !{i32 1, !"wchar_size", i32 4}
!24 = !{i32 8, !"PIC Level", i32 2}
!25 = !{i32 7, !"PIE Level", i32 2}
!26 = !{i32 7, !"uwtable", i32 2}
!27 = !{i32 7, !"frame-pointer", i32 2}
!28 = !{i32 4, !"nosanitize_dataflow", i32 1}
!29 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!30 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !31, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !19, retainedNodes: !34)
!31 = !DISubroutineType(types: !32)
!32 = !{!33}
!33 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!34 = !{}
!35 = !DILocalVariable(name: "secret1", scope: !30, file: !2, line: 6, type: !33)
!36 = !DILocation(line: 6, column: 9, scope: !30)
!37 = !DILocalVariable(name: "secret2", scope: !30, file: !2, line: 7, type: !33)
!38 = !DILocation(line: 7, column: 9, scope: !30)
!39 = !DILocation(line: 9, column: 5, scope: !30)
!40 = !DILocation(line: 14, column: 5, scope: !30)
!41 = !DILocalVariable(name: "x", scope: !30, file: !2, line: 19, type: !33)
!42 = !DILocation(line: 19, column: 9, scope: !30)
!43 = !DILocation(line: 19, column: 13, scope: !30)
!44 = !DILocalVariable(name: "y", scope: !30, file: !2, line: 20, type: !33)
!45 = !DILocation(line: 20, column: 9, scope: !30)
!46 = !DILocalVariable(name: "z", scope: !30, file: !2, line: 21, type: !33)
!47 = !DILocation(line: 21, column: 9, scope: !30)
!48 = !DILocation(line: 23, column: 9, scope: !49)
!49 = distinct !DILexicalBlock(scope: !30, file: !2, line: 23, column: 9)
!50 = !DILocation(line: 23, column: 13, scope: !49)
!51 = !DILocation(line: 23, column: 11, scope: !49)
!52 = !DILocation(line: 23, column: 17, scope: !49)
!53 = !DILocation(line: 23, column: 15, scope: !49)
!54 = !DILocation(line: 23, column: 19, scope: !49)
!55 = !DILocation(line: 24, column: 9, scope: !56)
!56 = distinct !DILexicalBlock(scope: !49, file: !2, line: 23, column: 25)
!57 = !DILocation(line: 25, column: 5, scope: !56)
!58 = !DILocation(line: 27, column: 9, scope: !59)
!59 = distinct !DILexicalBlock(scope: !30, file: !2, line: 27, column: 9)
!60 = !DILocation(line: 27, column: 13, scope: !59)
!61 = !DILocation(line: 27, column: 11, scope: !59)
!62 = !DILocation(line: 27, column: 15, scope: !59)
!63 = !DILocation(line: 28, column: 9, scope: !64)
!64 = distinct !DILexicalBlock(scope: !59, file: !2, line: 27, column: 21)
!65 = !DILocation(line: 29, column: 5, scope: !64)
!66 = !DILocation(line: 31, column: 9, scope: !67)
!67 = distinct !DILexicalBlock(scope: !30, file: !2, line: 31, column: 9)
!68 = !DILocation(line: 31, column: 11, scope: !67)
!69 = !DILocation(line: 32, column: 9, scope: !70)
!70 = distinct !DILexicalBlock(scope: !67, file: !2, line: 31, column: 17)
!71 = !DILocation(line: 33, column: 5, scope: !70)
!72 = !DILocation(line: 35, column: 9, scope: !30)
!73 = !DILocation(line: 35, column: 13, scope: !30)
!74 = !DILocation(line: 35, column: 11, scope: !30)
!75 = !DILocation(line: 35, column: 7, scope: !30)
!76 = !DILocation(line: 37, column: 8, scope: !77)
!77 = distinct !DILexicalBlock(scope: !30, file: !2, line: 37, column: 8)
!78 = !DILocation(line: 37, column: 10, scope: !77)
!79 = !DILocation(line: 38, column: 9, scope: !80)
!80 = distinct !DILexicalBlock(scope: !77, file: !2, line: 37, column: 15)
!81 = !DILocation(line: 39, column: 5, scope: !80)
!82 = !DILocation(line: 41, column: 5, scope: !30)
