; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"Condition 1: true\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [19 x i8] c"Condition 2: true\0A\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [12 x i8] c"Z is clean\0A\00", align 1, !dbg !9

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !24 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 0, ptr %1, align 4
    #dbg_declare(ptr %2, !29, !DIExpression(), !30)
  store i32 5, ptr %2, align 4, !dbg !30
    #dbg_declare(ptr %3, !31, !DIExpression(), !32)
  store i32 8, ptr %3, align 4, !dbg !32
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %2, i64 noundef 4), !dbg !33
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %3, i64 noundef 4), !dbg !34
    #dbg_declare(ptr %4, !35, !DIExpression(), !36)
  %7 = load i32, ptr %2, align 4, !dbg !37
  store i32 %7, ptr %4, align 4, !dbg !36
    #dbg_declare(ptr %5, !38, !DIExpression(), !39)
  %8 = load i32, ptr %3, align 4, !dbg !40
  store i32 %8, ptr %5, align 4, !dbg !39
    #dbg_declare(ptr %6, !41, !DIExpression(), !42)
  store i32 12, ptr %6, align 4, !dbg !42
  %9 = load i32, ptr %4, align 4, !dbg !43
  %10 = load i32, ptr %5, align 4, !dbg !45
  %11 = add nsw i32 %9, %10, !dbg !46
  %12 = load i32, ptr %6, align 4, !dbg !47
  %13 = add nsw i32 %11, %12, !dbg !48
  %14 = icmp sgt i32 %13, 10, !dbg !49
  br i1 %14, label %15, label %17, !dbg !49

15:                                               ; preds = %0
  %16 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !50
  br label %17, !dbg !52

17:                                               ; preds = %15, %0
  %18 = load i32, ptr %5, align 4, !dbg !53
  %19 = load i32, ptr %6, align 4, !dbg !55
  %20 = add nsw i32 %18, %19, !dbg !56
  %21 = icmp sgt i32 %20, 20, !dbg !57
  br i1 %21, label %22, label %24, !dbg !57

22:                                               ; preds = %17
  %23 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !58
  br label %24, !dbg !60

24:                                               ; preds = %22, %17
  %25 = load i32, ptr %6, align 4, !dbg !61
  %26 = icmp sgt i32 %25, 20, !dbg !63
  br i1 %26, label %27, label %29, !dbg !63

27:                                               ; preds = %24
  %28 = call i32 (ptr, ...) @printf(ptr noundef @.str.2), !dbg !64
  br label %29, !dbg !66

29:                                               ; preds = %27, %24
  ret i32 0, !dbg !67
}

declare void @dfsan_set_label(i8 noundef zeroext, ptr noundef, i64 noundef) #1

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!14}
!llvm.module.flags = !{!16, !17, !18, !19, !20, !21, !22}
!llvm.ident = !{!23}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 17, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "3990ba2dd0e0d4365faf5c70c4102ecf")
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
!23 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!24 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !25, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !14, retainedNodes: !28)
!25 = !DISubroutineType(types: !26)
!26 = !{!27}
!27 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!28 = !{}
!29 = !DILocalVariable(name: "secret1", scope: !24, file: !2, line: 6, type: !27)
!30 = !DILocation(line: 6, column: 9, scope: !24)
!31 = !DILocalVariable(name: "secret2", scope: !24, file: !2, line: 7, type: !27)
!32 = !DILocation(line: 7, column: 9, scope: !24)
!33 = !DILocation(line: 9, column: 5, scope: !24)
!34 = !DILocation(line: 10, column: 5, scope: !24)
!35 = !DILocalVariable(name: "x", scope: !24, file: !2, line: 12, type: !27)
!36 = !DILocation(line: 12, column: 9, scope: !24)
!37 = !DILocation(line: 12, column: 13, scope: !24)
!38 = !DILocalVariable(name: "y", scope: !24, file: !2, line: 13, type: !27)
!39 = !DILocation(line: 13, column: 9, scope: !24)
!40 = !DILocation(line: 13, column: 13, scope: !24)
!41 = !DILocalVariable(name: "z", scope: !24, file: !2, line: 14, type: !27)
!42 = !DILocation(line: 14, column: 9, scope: !24)
!43 = !DILocation(line: 16, column: 9, scope: !44)
!44 = distinct !DILexicalBlock(scope: !24, file: !2, line: 16, column: 9)
!45 = !DILocation(line: 16, column: 13, scope: !44)
!46 = !DILocation(line: 16, column: 11, scope: !44)
!47 = !DILocation(line: 16, column: 17, scope: !44)
!48 = !DILocation(line: 16, column: 15, scope: !44)
!49 = !DILocation(line: 16, column: 19, scope: !44)
!50 = !DILocation(line: 17, column: 9, scope: !51)
!51 = distinct !DILexicalBlock(scope: !44, file: !2, line: 16, column: 25)
!52 = !DILocation(line: 18, column: 5, scope: !51)
!53 = !DILocation(line: 20, column: 9, scope: !54)
!54 = distinct !DILexicalBlock(scope: !24, file: !2, line: 20, column: 9)
!55 = !DILocation(line: 20, column: 13, scope: !54)
!56 = !DILocation(line: 20, column: 11, scope: !54)
!57 = !DILocation(line: 20, column: 15, scope: !54)
!58 = !DILocation(line: 21, column: 9, scope: !59)
!59 = distinct !DILexicalBlock(scope: !54, file: !2, line: 20, column: 21)
!60 = !DILocation(line: 22, column: 5, scope: !59)
!61 = !DILocation(line: 24, column: 9, scope: !62)
!62 = distinct !DILexicalBlock(scope: !24, file: !2, line: 24, column: 9)
!63 = !DILocation(line: 24, column: 11, scope: !62)
!64 = !DILocation(line: 25, column: 9, scope: !65)
!65 = distinct !DILexicalBlock(scope: !62, file: !2, line: 24, column: 17)
!66 = !DILocation(line: 26, column: 5, scope: !65)
!67 = !DILocation(line: 28, column: 5, scope: !24)
