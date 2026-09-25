; ModuleID = 'dfsan.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [16 x i8] c"leaking x and y\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [10 x i8] c"leaking y\00", align 1, !dbg !7
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1
@__implicit_loop_names = private unnamed_addr constant [2 x i8] c"i\00", align 1
@__implicit_variable_names = private unnamed_addr constant [2 x i8] c"i\00", align 1
@__implicit_loop_names.1 = private unnamed_addr constant [2 x i8] c"i\00", align 1
@__implicit_variable_names.2 = private unnamed_addr constant [2 x i8] c"i\00", align 1
@__implicit_loop_names.3 = private unnamed_addr constant [2 x i8] c"i\00", align 1
@__implicit_variable_names.4 = private unnamed_addr constant [2 x i8] c"x\00", align 1
@__implicit_loop_names.5 = private unnamed_addr constant [2 x i8] c"i\00", align 1
@__implicit_variable_names.6 = private unnamed_addr constant [2 x i8] c"y\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !23 {
entry:
  %__implicit_loop_values = alloca [1 x i64], align 8
  %secret = alloca i32, align 4
    #dbg_declare(ptr %secret, !28, !DIExpression(), !29)
  %0 = ptrtoint ptr %secret to i64, !dbg !29
  %1 = xor i64 %0, 87960930222080, !dbg !29
  %2 = inttoptr i64 %1 to ptr, !dbg !29
  store i32 0, ptr %2, align 1, !dbg !29
  store i32 5, ptr %secret, align 4, !dbg !29
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %secret, i64 noundef 4), !dbg !30
    #dbg_value(i32 0, !31, !DIExpression(), !33)
  br label %for.cond, !dbg !34

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ], !dbg !35
    #dbg_value(i32 %i.0, !31, !DIExpression(), !33)
  %cmp = icmp slt i32 %i.0, 3, !dbg !36
  %3 = sext i32 %i.0 to i64, !dbg !38
  %4 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !38
  store i64 %3, ptr %4, align 8, !dbg !38
  %5 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !38
  call void @__implicit_branch_callback(i8 0, i32 12, i32 3, i32 1, ptr @__implicit_loop_names, ptr %5, ptr @__implicit_variable_names), !dbg !38
  call void @__dfsan_conditional_callback(i8 zeroext 0), !dbg !38
  br i1 %cmp, label %for.body, label %for.end, !dbg !38

for.body:                                         ; preds = %for.cond
  %6 = ptrtoint ptr %secret to i64, !dbg !39
  %7 = xor i64 %6, 87960930222080, !dbg !39
  %8 = inttoptr i64 %7 to ptr, !dbg !39
  %9 = load i32, ptr %8, align 1, !dbg !39
  %10 = lshr i32 %9, 16, !dbg !39
  %11 = or i32 %9, %10, !dbg !39
  %12 = lshr i32 %11, 8, !dbg !39
  %13 = or i32 %11, %12, !dbg !39
  %14 = trunc i32 %13 to i8, !dbg !39
  %15 = load i32, ptr %secret, align 4, !dbg !39
    #dbg_value(i32 %15, !41, !DIExpression(), !42)
  %16 = ptrtoint ptr %secret to i64, !dbg !43
  %17 = xor i64 %16, 87960930222080, !dbg !43
  %18 = inttoptr i64 %17 to ptr, !dbg !43
  %19 = load i32, ptr %18, align 1, !dbg !43
  %20 = lshr i32 %19, 16, !dbg !43
  %21 = or i32 %19, %20, !dbg !43
  %22 = lshr i32 %21, 8, !dbg !43
  %23 = or i32 %21, %22, !dbg !43
  %24 = trunc i32 %23 to i8, !dbg !43
  %25 = load i32, ptr %secret, align 4, !dbg !43
  %add = add nsw i32 %25, 2, !dbg !44
    #dbg_value(i32 %add, !45, !DIExpression(), !42)
  %cmp1 = icmp sgt i32 %i.0, 1, !dbg !46
  %26 = sext i32 %i.0 to i64, !dbg !46
  %27 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !46
  store i64 %26, ptr %27, align 8, !dbg !46
  %28 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !46
  call void @__implicit_branch_callback(i8 0, i32 15, i32 12, i32 1, ptr @__implicit_loop_names.1, ptr %28, ptr @__implicit_variable_names.2), !dbg !46
  call void @__dfsan_conditional_callback(i8 zeroext 0), !dbg !46
  br i1 %cmp1, label %if.then, label %if.else, !dbg !46

if.then:                                          ; preds = %for.body
  %cmp2 = icmp sgt i32 %15, 2, !dbg !48
  %29 = sext i32 %i.0 to i64, !dbg !48
  %30 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !48
  store i64 %29, ptr %30, align 8, !dbg !48
  %31 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !48
  call void @__implicit_branch_callback(i8 %14, i32 16, i32 14, i32 1, ptr @__implicit_loop_names.3, ptr %31, ptr @__implicit_variable_names.4), !dbg !48
  call void @__dfsan_conditional_callback(i8 zeroext %14), !dbg !48
  br i1 %cmp2, label %if.then3, label %if.end, !dbg !48

if.then3:                                         ; preds = %if.then
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !51
  br label %if.end, !dbg !51

if.end:                                           ; preds = %if.then3, %if.then
  br label %if.end7, !dbg !52

if.else:                                          ; preds = %for.body
  %tobool = icmp ne i32 %add, 0, !dbg !53
  %32 = sext i32 %i.0 to i64, !dbg !53
  %33 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !53
  store i64 %32, ptr %33, align 8, !dbg !53
  %34 = getelementptr inbounds [1 x i64], ptr %__implicit_loop_values, i32 0, i32 0, !dbg !53
  call void @__implicit_branch_callback(i8 %24, i32 18, i32 12, i32 1, ptr @__implicit_loop_names.5, ptr %34, ptr @__implicit_variable_names.6), !dbg !53
  call void @__dfsan_conditional_callback(i8 zeroext %24), !dbg !53
  br i1 %tobool, label %if.then4, label %if.end6, !dbg !53

if.then4:                                         ; preds = %if.else
  %call5 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !56
  br label %if.end6, !dbg !56

if.end6:                                          ; preds = %if.then4, %if.else
  br label %if.end7

if.end7:                                          ; preds = %if.end6, %if.end
  br label %for.inc, !dbg !57

for.inc:                                          ; preds = %if.end7
  %inc = add nsw i32 %i.0, 1, !dbg !58
    #dbg_value(i32 %inc, !31, !DIExpression(), !33)
  br label %for.cond, !dbg !59, !llvm.loop !60

for.end:                                          ; preds = %for.cond
  ret i32 0, !dbg !63
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

declare void @__implicit_branch_callback(i8, i32, i32, i32, ptr, ptr, ptr)

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!12}
!llvm.module.flags = !{!14, !15, !16, !17, !18, !19, !20, !21}
!llvm.ident = !{!22}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 16, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking_inside_loop", checksumkind: CSK_MD5, checksum: "4e037d528df961bdbf2a2a90994bbfcd")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 128, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 16)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 18, type: !9, isLocal: true, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 80, elements: !10)
!10 = !{!11}
!11 = !DISubrange(count: 10)
!12 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !13, splitDebugInlining: false, nameTableKind: None)
!13 = !{!0, !7}
!14 = !{i32 7, !"Dwarf Version", i32 5}
!15 = !{i32 2, !"Debug Info Version", i32 3}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{i32 8, !"PIC Level", i32 2}
!18 = !{i32 7, !"PIE Level", i32 2}
!19 = !{i32 7, !"uwtable", i32 2}
!20 = !{i32 7, !"frame-pointer", i32 2}
!21 = !{i32 4, !"nosanitize_dataflow", i32 1}
!22 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!23 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 6, type: !24, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !12, retainedNodes: !27)
!24 = !DISubroutineType(types: !25)
!25 = !{!26}
!26 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!27 = !{}
!28 = !DILocalVariable(name: "secret", scope: !23, file: !2, line: 8, type: !26)
!29 = !DILocation(line: 8, column: 7, scope: !23)
!30 = !DILocation(line: 10, column: 3, scope: !23)
!31 = !DILocalVariable(name: "i", scope: !32, file: !2, line: 12, type: !26)
!32 = distinct !DILexicalBlock(scope: !23, file: !2, line: 12, column: 3)
!33 = !DILocation(line: 0, scope: !32)
!34 = !DILocation(line: 12, column: 8, scope: !32)
!35 = !DILocation(line: 12, scope: !32)
!36 = !DILocation(line: 12, column: 21, scope: !37)
!37 = distinct !DILexicalBlock(scope: !32, file: !2, line: 12, column: 3)
!38 = !DILocation(line: 12, column: 3, scope: !32)
!39 = !DILocation(line: 13, column: 15, scope: !40)
!40 = distinct !DILexicalBlock(scope: !37, file: !2, line: 12, column: 31)
!41 = !DILocalVariable(name: "x", scope: !40, file: !2, line: 13, type: !26)
!42 = !DILocation(line: 0, scope: !40)
!43 = !DILocation(line: 13, column: 27, scope: !40)
!44 = !DILocation(line: 13, column: 33, scope: !40)
!45 = !DILocalVariable(name: "y", scope: !40, file: !2, line: 13, type: !26)
!46 = !DILocation(line: 15, column: 12, scope: !47)
!47 = distinct !DILexicalBlock(scope: !40, file: !2, line: 15, column: 10)
!48 = !DILocation(line: 16, column: 14, scope: !49)
!49 = distinct !DILexicalBlock(scope: !50, file: !2, line: 16, column: 12)
!50 = distinct !DILexicalBlock(scope: !47, file: !2, line: 15, column: 16)
!51 = !DILocation(line: 16, column: 19, scope: !49)
!52 = !DILocation(line: 17, column: 7, scope: !50)
!53 = !DILocation(line: 18, column: 12, scope: !54)
!54 = distinct !DILexicalBlock(scope: !55, file: !2, line: 18, column: 12)
!55 = distinct !DILexicalBlock(scope: !47, file: !2, line: 17, column: 12)
!56 = !DILocation(line: 18, column: 15, scope: !54)
!57 = !DILocation(line: 21, column: 3, scope: !40)
!58 = !DILocation(line: 12, column: 26, scope: !37)
!59 = !DILocation(line: 12, column: 3, scope: !37)
!60 = distinct !{!60, !38, !61, !62}
!61 = !DILocation(line: 21, column: 3, scope: !32)
!62 = !{!"llvm.loop.mustprogress"}
!63 = !DILocation(line: 23, column: 3, scope: !23)
