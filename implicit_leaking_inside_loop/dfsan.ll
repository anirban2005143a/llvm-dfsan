; ModuleID = 'test.c'
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

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !23 {
entry:
  %0 = alloca i8, align 1
  %retval = alloca i32, align 4
  %secret = alloca i32, align 4
  %1 = alloca i8, align 1
  %i = alloca i32, align 4
  %2 = alloca i8, align 1
  %x = alloca i32, align 4
  %3 = alloca i8, align 1
  %y = alloca i32, align 4
  store i8 0, ptr %0, align 1
  store i32 0, ptr %retval, align 4
    #dbg_declare(ptr %secret, !28, !DIExpression(), !29)
  %4 = ptrtoint ptr %secret to i64, !dbg !29
  %5 = xor i64 %4, 87960930222080, !dbg !29
  %6 = inttoptr i64 %5 to ptr, !dbg !29
  store i32 0, ptr %6, align 1, !dbg !29
  store i32 5, ptr %secret, align 4, !dbg !29
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %secret, i64 noundef 4), !dbg !30
    #dbg_declare(ptr %i, !31, !DIExpression(), !33)
  store i8 0, ptr %1, align 1, !dbg !33
  store i32 0, ptr %i, align 4, !dbg !33
  br label %for.cond, !dbg !34

for.cond:                                         ; preds = %for.inc, %entry
  %7 = load i8, ptr %1, align 1, !dbg !35
  %8 = load i32, ptr %i, align 4, !dbg !35
  %cmp = icmp slt i32 %8, 3, !dbg !37
  call void @__dfsan_conditional_callback(i8 zeroext %7), !dbg !38
  br i1 %cmp, label %for.body, label %for.end, !dbg !38

for.body:                                         ; preds = %for.cond
    #dbg_declare(ptr %x, !39, !DIExpression(), !41)
  %9 = ptrtoint ptr %secret to i64, !dbg !42
  %10 = xor i64 %9, 87960930222080, !dbg !42
  %11 = inttoptr i64 %10 to ptr, !dbg !42
  %12 = load i32, ptr %11, align 1, !dbg !42
  %13 = lshr i32 %12, 16, !dbg !42
  %14 = or i32 %12, %13, !dbg !42
  %15 = lshr i32 %14, 8, !dbg !42
  %16 = or i32 %14, %15, !dbg !42
  %17 = trunc i32 %16 to i8, !dbg !42
  %18 = load i32, ptr %secret, align 4, !dbg !42
  store i8 %17, ptr %2, align 1, !dbg !41
  store i32 %18, ptr %x, align 4, !dbg !41
    #dbg_declare(ptr %y, !43, !DIExpression(), !44)
  %19 = ptrtoint ptr %secret to i64, !dbg !45
  %20 = xor i64 %19, 87960930222080, !dbg !45
  %21 = inttoptr i64 %20 to ptr, !dbg !45
  %22 = load i32, ptr %21, align 1, !dbg !45
  %23 = lshr i32 %22, 16, !dbg !45
  %24 = or i32 %22, %23, !dbg !45
  %25 = lshr i32 %24, 8, !dbg !45
  %26 = or i32 %24, %25, !dbg !45
  %27 = trunc i32 %26 to i8, !dbg !45
  %28 = load i32, ptr %secret, align 4, !dbg !45
  %add = add nsw i32 %28, 2, !dbg !46
  store i8 %27, ptr %3, align 1, !dbg !44
  store i32 %add, ptr %y, align 4, !dbg !44
  %29 = load i8, ptr %1, align 1, !dbg !47
  %30 = load i32, ptr %i, align 4, !dbg !47
  %cmp1 = icmp sgt i32 %30, 1, !dbg !49
  call void @__dfsan_conditional_callback(i8 zeroext %29), !dbg !49
  br i1 %cmp1, label %if.then, label %if.else, !dbg !49

if.then:                                          ; preds = %for.body
  %31 = load i8, ptr %2, align 1, !dbg !50
  %32 = load i32, ptr %x, align 4, !dbg !50
  %cmp2 = icmp sgt i32 %32, 2, !dbg !53
  call void @__dfsan_conditional_callback(i8 zeroext %31), !dbg !53
  br i1 %cmp2, label %if.then3, label %if.end, !dbg !53

if.then3:                                         ; preds = %if.then
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !54
  br label %if.end, !dbg !54

if.end:                                           ; preds = %if.then3, %if.then
  br label %if.end7, !dbg !55

if.else:                                          ; preds = %for.body
  %33 = load i8, ptr %3, align 1, !dbg !56
  %34 = load i32, ptr %y, align 4, !dbg !56
  %tobool = icmp ne i32 %34, 0, !dbg !56
  call void @__dfsan_conditional_callback(i8 zeroext %33), !dbg !56
  br i1 %tobool, label %if.then4, label %if.end6, !dbg !56

if.then4:                                         ; preds = %if.else
  %call5 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !59
  br label %if.end6, !dbg !59

if.end6:                                          ; preds = %if.then4, %if.else
  br label %if.end7

if.end7:                                          ; preds = %if.end6, %if.end
  br label %for.inc, !dbg !60

for.inc:                                          ; preds = %if.end7
  %35 = load i8, ptr %1, align 1, !dbg !61
  %36 = load i32, ptr %i, align 4, !dbg !61
  %inc = add nsw i32 %36, 1, !dbg !61
  store i8 %35, ptr %1, align 1, !dbg !61
  store i32 %inc, ptr %i, align 4, !dbg !61
  br label %for.cond, !dbg !62, !llvm.loop !63

for.end:                                          ; preds = %for.cond
  ret i32 0, !dbg !66
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
!33 = !DILocation(line: 12, column: 12, scope: !32)
!34 = !DILocation(line: 12, column: 8, scope: !32)
!35 = !DILocation(line: 12, column: 19, scope: !36)
!36 = distinct !DILexicalBlock(scope: !32, file: !2, line: 12, column: 3)
!37 = !DILocation(line: 12, column: 21, scope: !36)
!38 = !DILocation(line: 12, column: 3, scope: !32)
!39 = !DILocalVariable(name: "x", scope: !40, file: !2, line: 13, type: !26)
!40 = distinct !DILexicalBlock(scope: !36, file: !2, line: 12, column: 31)
!41 = !DILocation(line: 13, column: 11, scope: !40)
!42 = !DILocation(line: 13, column: 15, scope: !40)
!43 = !DILocalVariable(name: "y", scope: !40, file: !2, line: 13, type: !26)
!44 = !DILocation(line: 13, column: 23, scope: !40)
!45 = !DILocation(line: 13, column: 27, scope: !40)
!46 = !DILocation(line: 13, column: 33, scope: !40)
!47 = !DILocation(line: 15, column: 10, scope: !48)
!48 = distinct !DILexicalBlock(scope: !40, file: !2, line: 15, column: 10)
!49 = !DILocation(line: 15, column: 12, scope: !48)
!50 = !DILocation(line: 16, column: 12, scope: !51)
!51 = distinct !DILexicalBlock(scope: !52, file: !2, line: 16, column: 12)
!52 = distinct !DILexicalBlock(scope: !48, file: !2, line: 15, column: 16)
!53 = !DILocation(line: 16, column: 14, scope: !51)
!54 = !DILocation(line: 16, column: 19, scope: !51)
!55 = !DILocation(line: 17, column: 7, scope: !52)
!56 = !DILocation(line: 18, column: 12, scope: !57)
!57 = distinct !DILexicalBlock(scope: !58, file: !2, line: 18, column: 12)
!58 = distinct !DILexicalBlock(scope: !48, file: !2, line: 17, column: 12)
!59 = !DILocation(line: 18, column: 15, scope: !57)
!60 = !DILocation(line: 21, column: 3, scope: !40)
!61 = !DILocation(line: 12, column: 26, scope: !36)
!62 = !DILocation(line: 12, column: 3, scope: !36)
!63 = distinct !{!63, !38, !64, !65}
!64 = !DILocation(line: 21, column: 3, scope: !32)
!65 = !{!"llvm.loop.mustprogress"}
!66 = !DILocation(line: 23, column: 3, scope: !23)
