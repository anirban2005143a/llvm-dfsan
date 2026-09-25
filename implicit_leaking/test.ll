; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"Condition 1: true\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [19 x i8] c"Condition 2: true\0A\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [12 x i8] c"Z is clean\0A\00", align 1, !dbg !9
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !25 {
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
    #dbg_declare(ptr %3, !30, !DIExpression(), !31)
  %11 = ptrtoint ptr %3 to i64, !dbg !31
  %12 = xor i64 %11, 87960930222080, !dbg !31
  %13 = inttoptr i64 %12 to ptr, !dbg !31
  store i32 0, ptr %13, align 1, !dbg !31
  store i32 5, ptr %3, align 4, !dbg !31
    #dbg_declare(ptr %4, !32, !DIExpression(), !33)
  %14 = ptrtoint ptr %4 to i64, !dbg !33
  %15 = xor i64 %14, 87960930222080, !dbg !33
  %16 = inttoptr i64 %15 to ptr, !dbg !33
  store i32 0, ptr %16, align 1, !dbg !33
  store i32 8, ptr %4, align 4, !dbg !33
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !34
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !35
    #dbg_declare(ptr %6, !36, !DIExpression(), !37)
  %17 = ptrtoint ptr %3 to i64, !dbg !38
  %18 = xor i64 %17, 87960930222080, !dbg !38
  %19 = inttoptr i64 %18 to ptr, !dbg !38
  %20 = load i32, ptr %19, align 1, !dbg !38
  %21 = lshr i32 %20, 16, !dbg !38
  %22 = or i32 %20, %21, !dbg !38
  %23 = lshr i32 %22, 8, !dbg !38
  %24 = or i32 %22, %23, !dbg !38
  %25 = trunc i32 %24 to i8, !dbg !38
  %26 = load i32, ptr %3, align 4, !dbg !38
  store i8 %25, ptr %5, align 1, !dbg !37
  store i32 %26, ptr %6, align 4, !dbg !37
    #dbg_declare(ptr %8, !39, !DIExpression(), !40)
  %27 = ptrtoint ptr %4 to i64, !dbg !41
  %28 = xor i64 %27, 87960930222080, !dbg !41
  %29 = inttoptr i64 %28 to ptr, !dbg !41
  %30 = load i32, ptr %29, align 1, !dbg !41
  %31 = lshr i32 %30, 16, !dbg !41
  %32 = or i32 %30, %31, !dbg !41
  %33 = lshr i32 %32, 8, !dbg !41
  %34 = or i32 %32, %33, !dbg !41
  %35 = trunc i32 %34 to i8, !dbg !41
  %36 = load i32, ptr %4, align 4, !dbg !41
  store i8 %35, ptr %7, align 1, !dbg !40
  store i32 %36, ptr %8, align 4, !dbg !40
    #dbg_declare(ptr %10, !42, !DIExpression(), !43)
  store i8 0, ptr %9, align 1, !dbg !43
  store i32 12, ptr %10, align 4, !dbg !43
  %37 = load i8, ptr %5, align 1, !dbg !44
  %38 = load i32, ptr %6, align 4, !dbg !44
  %39 = load i8, ptr %7, align 1, !dbg !46
  %40 = load i32, ptr %8, align 4, !dbg !46
  %41 = or i8 %37, %39, !dbg !47
  %42 = add nsw i32 %38, %40, !dbg !47
  %43 = load i8, ptr %9, align 1, !dbg !48
  %44 = load i32, ptr %10, align 4, !dbg !48
  %45 = or i8 %41, %43, !dbg !49
  %46 = add nsw i32 %42, %44, !dbg !49
  %47 = icmp sgt i32 %46, 10, !dbg !50
  call void @__dfsan_conditional_callback(i8 zeroext %45), !dbg !50
  br i1 %47, label %48, label %50, !dbg !50

48:                                               ; preds = %0
  %49 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !51
  br label %50, !dbg !53

50:                                               ; preds = %48, %0
  %51 = load i8, ptr %7, align 1, !dbg !54
  %52 = load i32, ptr %8, align 4, !dbg !54
  %53 = load i8, ptr %9, align 1, !dbg !56
  %54 = load i32, ptr %10, align 4, !dbg !56
  %55 = or i8 %51, %53, !dbg !57
  %56 = add nsw i32 %52, %54, !dbg !57
  %57 = icmp sgt i32 %56, 20, !dbg !58
  call void @__dfsan_conditional_callback(i8 zeroext %55), !dbg !58
  br i1 %57, label %58, label %60, !dbg !58

58:                                               ; preds = %50
  %59 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !59
  br label %60, !dbg !61

60:                                               ; preds = %58, %50
  %61 = load i8, ptr %9, align 1, !dbg !62
  %62 = load i32, ptr %10, align 4, !dbg !62
  %63 = icmp sgt i32 %62, 20, !dbg !64
  call void @__dfsan_conditional_callback(i8 zeroext %61), !dbg !64
  br i1 %63, label %64, label %66, !dbg !64

64:                                               ; preds = %60
  %65 = call i32 (ptr, ...) @printf(ptr noundef @.str.2), !dbg !65
  br label %66, !dbg !67

66:                                               ; preds = %64, %60
  ret i32 0, !dbg !68
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

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!14}
!llvm.module.flags = !{!16, !17, !18, !19, !20, !21, !22, !23}
!llvm.ident = !{!24}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 17, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "160a06f1948da0e9109660661e7634bd")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 19)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 21, type: !3, isLocal: true, isDefinition: true)
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(scope: null, file: !2, line: 25, type: !11, isLocal: true, isDefinition: true)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 96, elements: !12)
!12 = !{!13}
!13 = !DISubrange(count: 12)
!14 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !15, splitDebugInlining: false, nameTableKind: None)
!15 = !{!0, !7, !9}
!16 = !{i32 7, !"Dwarf Version", i32 5}
!17 = !{i32 2, !"Debug Info Version", i32 3}
!18 = !{i32 1, !"wchar_size", i32 4}
!19 = !{i32 8, !"PIC Level", i32 2}
!20 = !{i32 7, !"PIE Level", i32 2}
!21 = !{i32 7, !"uwtable", i32 2}
!22 = !{i32 7, !"frame-pointer", i32 2}
!23 = !{i32 4, !"nosanitize_dataflow", i32 1}
!24 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!25 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !26, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !14, retainedNodes: !29)
!26 = !DISubroutineType(types: !27)
!27 = !{!28}
!28 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!29 = !{}
!30 = !DILocalVariable(name: "secret1", scope: !25, file: !2, line: 6, type: !28)
!31 = !DILocation(line: 6, column: 9, scope: !25)
!32 = !DILocalVariable(name: "secret2", scope: !25, file: !2, line: 7, type: !28)
!33 = !DILocation(line: 7, column: 9, scope: !25)
!34 = !DILocation(line: 9, column: 5, scope: !25)
!35 = !DILocation(line: 10, column: 5, scope: !25)
!36 = !DILocalVariable(name: "x", scope: !25, file: !2, line: 12, type: !28)
!37 = !DILocation(line: 12, column: 9, scope: !25)
!38 = !DILocation(line: 12, column: 13, scope: !25)
!39 = !DILocalVariable(name: "y", scope: !25, file: !2, line: 13, type: !28)
!40 = !DILocation(line: 13, column: 9, scope: !25)
!41 = !DILocation(line: 13, column: 13, scope: !25)
!42 = !DILocalVariable(name: "z", scope: !25, file: !2, line: 14, type: !28)
!43 = !DILocation(line: 14, column: 9, scope: !25)
!44 = !DILocation(line: 16, column: 9, scope: !45)
!45 = distinct !DILexicalBlock(scope: !25, file: !2, line: 16, column: 9)
!46 = !DILocation(line: 16, column: 11, scope: !45)
!47 = !DILocation(line: 16, column: 10, scope: !45)
!48 = !DILocation(line: 16, column: 13, scope: !45)
!49 = !DILocation(line: 16, column: 12, scope: !45)
!50 = !DILocation(line: 16, column: 15, scope: !45)
!51 = !DILocation(line: 17, column: 9, scope: !52)
!52 = distinct !DILexicalBlock(scope: !45, file: !2, line: 16, column: 21)
!53 = !DILocation(line: 18, column: 5, scope: !52)
!54 = !DILocation(line: 20, column: 9, scope: !55)
!55 = distinct !DILexicalBlock(scope: !25, file: !2, line: 20, column: 9)
!56 = !DILocation(line: 20, column: 13, scope: !55)
!57 = !DILocation(line: 20, column: 11, scope: !55)
!58 = !DILocation(line: 20, column: 15, scope: !55)
!59 = !DILocation(line: 21, column: 9, scope: !60)
!60 = distinct !DILexicalBlock(scope: !55, file: !2, line: 20, column: 21)
!61 = !DILocation(line: 22, column: 5, scope: !60)
!62 = !DILocation(line: 24, column: 9, scope: !63)
!63 = distinct !DILexicalBlock(scope: !25, file: !2, line: 24, column: 9)
!64 = !DILocation(line: 24, column: 11, scope: !63)
!65 = !DILocation(line: 25, column: 9, scope: !66)
!66 = distinct !DILexicalBlock(scope: !63, file: !2, line: 24, column: 17)
!67 = !DILocation(line: 26, column: 5, scope: !66)
!68 = !DILocation(line: 29, column: 5, scope: !25)
