; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [8 x i8] c"LEAK X\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [8 x i8] c"LEAK Y\0A\00", align 1, !dbg !7
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !20 {
entry:
  %0 = alloca i8, align 1
  %retval = alloca i32, align 4
  %secret = alloca i32, align 4
  %1 = alloca i8, align 1
  %i = alloca i32, align 4
  %2 = alloca i8, align 1
  %j = alloca i32, align 4
  %3 = alloca i8, align 1
  %x = alloca i32, align 4
  %4 = alloca i8, align 1
  %y = alloca i32, align 4
  store i8 0, ptr %0, align 1
  store i32 0, ptr %retval, align 4
    #dbg_declare(ptr %secret, !25, !DIExpression(), !26)
  %5 = ptrtoint ptr %secret to i64, !dbg !26
  %6 = xor i64 %5, 87960930222080, !dbg !26
  %7 = inttoptr i64 %6 to ptr, !dbg !26
  store i32 0, ptr %7, align 1, !dbg !26
  store i32 5, ptr %secret, align 4, !dbg !26
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %secret, i64 noundef 4), !dbg !27
    #dbg_declare(ptr %i, !28, !DIExpression(), !30)
  store i8 0, ptr %1, align 1, !dbg !30
  store i32 0, ptr %i, align 4, !dbg !30
  br label %for.cond, !dbg !31

for.cond:                                         ; preds = %for.inc24, %entry
  %8 = load i8, ptr %1, align 1, !dbg !32
  %9 = load i32, ptr %i, align 4, !dbg !32
  %cmp = icmp slt i32 %9, 5, !dbg !34
  call void @__dfsan_conditional_callback(i8 zeroext %8), !dbg !35
  br i1 %cmp, label %for.body, label %for.end26, !dbg !35

for.body:                                         ; preds = %for.cond
    #dbg_declare(ptr %j, !36, !DIExpression(), !39)
  store i8 0, ptr %2, align 1, !dbg !39
  store i32 0, ptr %j, align 4, !dbg !39
  br label %for.cond1, !dbg !40

for.cond1:                                        ; preds = %for.inc, %for.body
  %10 = load i8, ptr %2, align 1, !dbg !41
  %11 = load i32, ptr %j, align 4, !dbg !41
  %cmp2 = icmp slt i32 %11, 5, !dbg !43
  call void @__dfsan_conditional_callback(i8 zeroext %10), !dbg !44
  br i1 %cmp2, label %for.body3, label %for.end, !dbg !44

for.body3:                                        ; preds = %for.cond1
    #dbg_declare(ptr %x, !45, !DIExpression(), !47)
    #dbg_declare(ptr %y, !48, !DIExpression(), !49)
  %12 = load i8, ptr %1, align 1, !dbg !50
  %13 = load i32, ptr %i, align 4, !dbg !50
  %14 = load i8, ptr %2, align 1, !dbg !52
  %15 = load i32, ptr %j, align 4, !dbg !52
  %16 = or i8 %12, %14, !dbg !53
  %add = add nsw i32 %13, %15, !dbg !53
  %cmp4 = icmp sgt i32 %add, 3, !dbg !54
  call void @__dfsan_conditional_callback(i8 zeroext %16), !dbg !54
  br i1 %cmp4, label %if.then, label %if.else, !dbg !54

if.then:                                          ; preds = %for.body3
  %17 = ptrtoint ptr %secret to i64, !dbg !55
  %18 = xor i64 %17, 87960930222080, !dbg !55
  %19 = inttoptr i64 %18 to ptr, !dbg !55
  %20 = load i32, ptr %19, align 1, !dbg !55
  %21 = lshr i32 %20, 16, !dbg !55
  %22 = or i32 %20, %21, !dbg !55
  %23 = lshr i32 %22, 8, !dbg !55
  %24 = or i32 %22, %23, !dbg !55
  %25 = trunc i32 %24 to i8, !dbg !55
  %26 = load i32, ptr %secret, align 4, !dbg !55
  %27 = load i8, ptr %1, align 1, !dbg !57
  %28 = load i32, ptr %i, align 4, !dbg !57
  %29 = or i8 %25, %27, !dbg !58
  %add5 = add nsw i32 %26, %28, !dbg !58
  %30 = load i8, ptr %2, align 1, !dbg !59
  %31 = load i32, ptr %j, align 4, !dbg !59
  %32 = or i8 %29, %30, !dbg !60
  %add6 = add nsw i32 %add5, %31, !dbg !60
  store i8 %32, ptr %3, align 1, !dbg !61
  store i32 %add6, ptr %x, align 4, !dbg !61
  %33 = load i8, ptr %1, align 1, !dbg !62
  %34 = load i32, ptr %i, align 4, !dbg !62
  store i8 %33, ptr %4, align 1, !dbg !63
  store i32 %34, ptr %y, align 4, !dbg !63
  %35 = load i8, ptr %3, align 1, !dbg !64
  %36 = load i32, ptr %x, align 4, !dbg !64
  %cmp7 = icmp sgt i32 %36, 5, !dbg !66
  call void @__dfsan_conditional_callback(i8 zeroext %35), !dbg !66
  br i1 %cmp7, label %if.then8, label %if.end, !dbg !66

if.then8:                                         ; preds = %if.then
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !67
  br label %if.end, !dbg !69

if.end:                                           ; preds = %if.then8, %if.then
  %37 = load i8, ptr %4, align 1, !dbg !70
  %38 = load i32, ptr %y, align 4, !dbg !70
  %cmp9 = icmp sgt i32 %38, 4, !dbg !72
  call void @__dfsan_conditional_callback(i8 zeroext %37), !dbg !72
  br i1 %cmp9, label %if.then10, label %if.end12, !dbg !72

if.then10:                                        ; preds = %if.end
  %call11 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !73
  br label %if.end12, !dbg !75

if.end12:                                         ; preds = %if.then10, %if.end
  br label %if.end23, !dbg !76

if.else:                                          ; preds = %for.body3
  %39 = load i8, ptr %1, align 1, !dbg !77
  %40 = load i32, ptr %i, align 4, !dbg !77
  %41 = load i8, ptr %2, align 1, !dbg !79
  %42 = load i32, ptr %j, align 4, !dbg !79
  %43 = or i8 %39, %41, !dbg !80
  %add13 = add nsw i32 %40, %42, !dbg !80
  store i8 %43, ptr %3, align 1, !dbg !81
  store i32 %add13, ptr %x, align 4, !dbg !81
  %44 = ptrtoint ptr %secret to i64, !dbg !82
  %45 = xor i64 %44, 87960930222080, !dbg !82
  %46 = inttoptr i64 %45 to ptr, !dbg !82
  %47 = load i32, ptr %46, align 1, !dbg !82
  %48 = lshr i32 %47, 16, !dbg !82
  %49 = or i32 %47, %48, !dbg !82
  %50 = lshr i32 %49, 8, !dbg !82
  %51 = or i32 %49, %50, !dbg !82
  %52 = trunc i32 %51 to i8, !dbg !82
  %53 = load i32, ptr %secret, align 4, !dbg !82
  %54 = load i8, ptr %1, align 1, !dbg !83
  %55 = load i32, ptr %i, align 4, !dbg !83
  %56 = or i8 %52, %54, !dbg !84
  %add14 = add nsw i32 %53, %55, !dbg !84
  store i8 %56, ptr %4, align 1, !dbg !85
  store i32 %add14, ptr %y, align 4, !dbg !85
  %57 = load i8, ptr %3, align 1, !dbg !86
  %58 = load i32, ptr %x, align 4, !dbg !86
  %cmp15 = icmp sgt i32 %58, 5, !dbg !88
  call void @__dfsan_conditional_callback(i8 zeroext %57), !dbg !88
  br i1 %cmp15, label %if.then16, label %if.end18, !dbg !88

if.then16:                                        ; preds = %if.else
  %call17 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !89
  br label %if.end18, !dbg !91

if.end18:                                         ; preds = %if.then16, %if.else
  %59 = load i8, ptr %4, align 1, !dbg !92
  %60 = load i32, ptr %y, align 4, !dbg !92
  %cmp19 = icmp sgt i32 %60, 4, !dbg !94
  call void @__dfsan_conditional_callback(i8 zeroext %59), !dbg !94
  br i1 %cmp19, label %if.then20, label %if.end22, !dbg !94

if.then20:                                        ; preds = %if.end18
  %call21 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !95
  br label %if.end22, !dbg !97

if.end22:                                         ; preds = %if.then20, %if.end18
  br label %if.end23

if.end23:                                         ; preds = %if.end22, %if.end12
  br label %for.inc, !dbg !98

for.inc:                                          ; preds = %if.end23
  %61 = load i8, ptr %2, align 1, !dbg !99
  %62 = load i32, ptr %j, align 4, !dbg !99
  %inc = add nsw i32 %62, 1, !dbg !99
  store i8 %61, ptr %2, align 1, !dbg !99
  store i32 %inc, ptr %j, align 4, !dbg !99
  br label %for.cond1, !dbg !100, !llvm.loop !101

for.end:                                          ; preds = %for.cond1
  br label %for.inc24, !dbg !104

for.inc24:                                        ; preds = %for.end
  %63 = load i8, ptr %1, align 1, !dbg !105
  %64 = load i32, ptr %i, align 4, !dbg !105
  %inc25 = add nsw i32 %64, 1, !dbg !105
  store i8 %63, ptr %1, align 1, !dbg !105
  store i32 %inc25, ptr %i, align 4, !dbg !105
  br label %for.cond, !dbg !106, !llvm.loop !107

for.end26:                                        ; preds = %for.cond
  ret i32 0, !dbg !109
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
entry:
  %0 = call i32 @main()
  store i8 0, ptr @__dfsan_retval_tls, align 2
  ret i32 %0
}

define linkonce_odr void @"dfsw$dfsan_set_label"(i8 noundef zeroext %0, ptr noundef %1, i64 noundef %2) #1 {
entry:
  call void @dfsan_set_label(i8 %0, ptr %1, i64 %2)
  ret void
}

define linkonce_odr i32 @"dfsw$printf"(ptr noundef %0, ...) #1 {
entry:
  call void @__dfsan_vararg_wrapper(ptr @0)
  unreachable
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!9}
!llvm.module.flags = !{!11, !12, !13, !14, !15, !16, !17, !18}
!llvm.ident = !{!19}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 23, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking_inside_loop", checksumkind: CSK_MD5, checksum: "62d3b1e660c5bc81c2906055b35a3892")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 64, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 8)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 27, type: !3, isLocal: true, isDefinition: true)
!9 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !10, splitDebugInlining: false, nameTableKind: None)
!10 = !{!0, !7}
!11 = !{i32 7, !"Dwarf Version", i32 5}
!12 = !{i32 2, !"Debug Info Version", i32 3}
!13 = !{i32 1, !"wchar_size", i32 4}
!14 = !{i32 8, !"PIC Level", i32 2}
!15 = !{i32 7, !"PIE Level", i32 2}
!16 = !{i32 7, !"uwtable", i32 2}
!17 = !{i32 7, !"frame-pointer", i32 2}
!18 = !{i32 4, !"nosanitize_dataflow", i32 1}
!19 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!20 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 6, type: !21, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !9, retainedNodes: !24)
!21 = !DISubroutineType(types: !22)
!22 = !{!23}
!23 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!24 = !{}
!25 = !DILocalVariable(name: "secret", scope: !20, file: !2, line: 8, type: !23)
!26 = !DILocation(line: 8, column: 7, scope: !20)
!27 = !DILocation(line: 10, column: 3, scope: !20)
!28 = !DILocalVariable(name: "i", scope: !29, file: !2, line: 12, type: !23)
!29 = distinct !DILexicalBlock(scope: !20, file: !2, line: 12, column: 3)
!30 = !DILocation(line: 12, column: 12, scope: !29)
!31 = !DILocation(line: 12, column: 8, scope: !29)
!32 = !DILocation(line: 12, column: 19, scope: !33)
!33 = distinct !DILexicalBlock(scope: !29, file: !2, line: 12, column: 3)
!34 = !DILocation(line: 12, column: 21, scope: !33)
!35 = !DILocation(line: 12, column: 3, scope: !29)
!36 = !DILocalVariable(name: "j", scope: !37, file: !2, line: 14, type: !23)
!37 = distinct !DILexicalBlock(scope: !38, file: !2, line: 14, column: 5)
!38 = distinct !DILexicalBlock(scope: !33, file: !2, line: 12, column: 31)
!39 = !DILocation(line: 14, column: 14, scope: !37)
!40 = !DILocation(line: 14, column: 10, scope: !37)
!41 = !DILocation(line: 14, column: 21, scope: !42)
!42 = distinct !DILexicalBlock(scope: !37, file: !2, line: 14, column: 5)
!43 = !DILocation(line: 14, column: 23, scope: !42)
!44 = !DILocation(line: 14, column: 5, scope: !37)
!45 = !DILocalVariable(name: "x", scope: !46, file: !2, line: 16, type: !23)
!46 = distinct !DILexicalBlock(scope: !42, file: !2, line: 14, column: 33)
!47 = !DILocation(line: 16, column: 11, scope: !46)
!48 = !DILocalVariable(name: "y", scope: !46, file: !2, line: 16, type: !23)
!49 = !DILocation(line: 16, column: 13, scope: !46)
!50 = !DILocation(line: 18, column: 11, scope: !51)
!51 = distinct !DILexicalBlock(scope: !46, file: !2, line: 18, column: 11)
!52 = !DILocation(line: 18, column: 15, scope: !51)
!53 = !DILocation(line: 18, column: 13, scope: !51)
!54 = !DILocation(line: 18, column: 17, scope: !51)
!55 = !DILocation(line: 19, column: 13, scope: !56)
!56 = distinct !DILexicalBlock(scope: !51, file: !2, line: 18, column: 22)
!57 = !DILocation(line: 19, column: 22, scope: !56)
!58 = !DILocation(line: 19, column: 20, scope: !56)
!59 = !DILocation(line: 19, column: 26, scope: !56)
!60 = !DILocation(line: 19, column: 24, scope: !56)
!61 = !DILocation(line: 19, column: 11, scope: !56)
!62 = !DILocation(line: 20, column: 13, scope: !56)
!63 = !DILocation(line: 20, column: 11, scope: !56)
!64 = !DILocation(line: 22, column: 13, scope: !65)
!65 = distinct !DILexicalBlock(scope: !56, file: !2, line: 22, column: 13)
!66 = !DILocation(line: 22, column: 15, scope: !65)
!67 = !DILocation(line: 23, column: 11, scope: !68)
!68 = distinct !DILexicalBlock(scope: !65, file: !2, line: 22, column: 20)
!69 = !DILocation(line: 24, column: 9, scope: !68)
!70 = !DILocation(line: 26, column: 13, scope: !71)
!71 = distinct !DILexicalBlock(scope: !56, file: !2, line: 26, column: 13)
!72 = !DILocation(line: 26, column: 15, scope: !71)
!73 = !DILocation(line: 27, column: 11, scope: !74)
!74 = distinct !DILexicalBlock(scope: !71, file: !2, line: 26, column: 20)
!75 = !DILocation(line: 28, column: 9, scope: !74)
!76 = !DILocation(line: 29, column: 7, scope: !56)
!77 = !DILocation(line: 30, column: 13, scope: !78)
!78 = distinct !DILexicalBlock(scope: !51, file: !2, line: 29, column: 14)
!79 = !DILocation(line: 30, column: 15, scope: !78)
!80 = !DILocation(line: 30, column: 14, scope: !78)
!81 = !DILocation(line: 30, column: 11, scope: !78)
!82 = !DILocation(line: 31, column: 13, scope: !78)
!83 = !DILocation(line: 31, column: 22, scope: !78)
!84 = !DILocation(line: 31, column: 20, scope: !78)
!85 = !DILocation(line: 31, column: 11, scope: !78)
!86 = !DILocation(line: 33, column: 13, scope: !87)
!87 = distinct !DILexicalBlock(scope: !78, file: !2, line: 33, column: 13)
!88 = !DILocation(line: 33, column: 15, scope: !87)
!89 = !DILocation(line: 34, column: 11, scope: !90)
!90 = distinct !DILexicalBlock(scope: !87, file: !2, line: 33, column: 20)
!91 = !DILocation(line: 35, column: 9, scope: !90)
!92 = !DILocation(line: 37, column: 13, scope: !93)
!93 = distinct !DILexicalBlock(scope: !78, file: !2, line: 37, column: 13)
!94 = !DILocation(line: 37, column: 15, scope: !93)
!95 = !DILocation(line: 38, column: 11, scope: !96)
!96 = distinct !DILexicalBlock(scope: !93, file: !2, line: 37, column: 20)
!97 = !DILocation(line: 39, column: 9, scope: !96)
!98 = !DILocation(line: 41, column: 5, scope: !46)
!99 = !DILocation(line: 14, column: 28, scope: !42)
!100 = !DILocation(line: 14, column: 5, scope: !42)
!101 = distinct !{!101, !44, !102, !103}
!102 = !DILocation(line: 41, column: 5, scope: !37)
!103 = !{!"llvm.loop.mustprogress"}
!104 = !DILocation(line: 42, column: 3, scope: !38)
!105 = !DILocation(line: 12, column: 26, scope: !33)
!106 = !DILocation(line: 12, column: 3, scope: !33)
!107 = distinct !{!107, !35, !108, !103}
!108 = !DILocation(line: 42, column: 3, scope: !29)
!109 = !DILocation(line: 44, column: 3, scope: !20)
