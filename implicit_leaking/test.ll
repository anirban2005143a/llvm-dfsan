; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"Condition 1: true\0A\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [19 x i8] c"Condition 2: true\0A\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [12 x i8] c"Z is clean\0A\00", align 1, !dbg !9
@.str.3 = private unnamed_addr constant [19 x i8] c"Condition 3: true\0A\00", align 1, !dbg !14
@.str.4 = private unnamed_addr constant [19 x i8] c"Condition 4: true\0A\00", align 1, !dbg !16
@.str.5 = private unnamed_addr constant [19 x i8] c"Condition 5: true\0A\00", align 1, !dbg !18
@.str.6 = private unnamed_addr constant [28 x i8] c"Nested condition: all true\0A\00", align 1, !dbg !20
@.str.7 = private unnamed_addr constant [19 x i8] c"Condition 7: true\0A\00", align 1, !dbg !25
@.str.8 = private unnamed_addr constant [19 x i8] c"Condition 8: true\0A\00", align 1, !dbg !27

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !39 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  %13 = alloca i32, align 4
  store i32 0, ptr %1, align 4
    #dbg_declare(ptr %2, !44, !DIExpression(), !45)
  store i32 5, ptr %2, align 4, !dbg !45
    #dbg_declare(ptr %3, !46, !DIExpression(), !47)
  store i32 8, ptr %3, align 4, !dbg !47
    #dbg_declare(ptr %4, !48, !DIExpression(), !49)
  store i32 12, ptr %4, align 4, !dbg !49
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %2, i64 noundef 4), !dbg !50
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %3, i64 noundef 4), !dbg !51
    #dbg_declare(ptr %5, !52, !DIExpression(), !53)
  %14 = load i32, ptr %2, align 4, !dbg !54
  store i32 %14, ptr %5, align 4, !dbg !53
  %15 = load i32, ptr %5, align 4, !dbg !55
  %16 = icmp sgt i32 %15, 10, !dbg !57
  br i1 %16, label %17, label %19, !dbg !57

17:                                               ; preds = %0
  %18 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !58
  br label %19, !dbg !60

19:                                               ; preds = %17, %0
    #dbg_declare(ptr %6, !61, !DIExpression(), !62)
  %20 = load i32, ptr %3, align 4, !dbg !63
  store i32 %20, ptr %6, align 4, !dbg !62
    #dbg_declare(ptr %7, !64, !DIExpression(), !65)
  %21 = load i32, ptr %4, align 4, !dbg !66
  store i32 %21, ptr %7, align 4, !dbg !65
  %22 = load i32, ptr %6, align 4, !dbg !67
  %23 = load i32, ptr %7, align 4, !dbg !69
  %24 = add nsw i32 %22, %23, !dbg !70
  %25 = icmp sgt i32 %24, 20, !dbg !71
  br i1 %25, label %26, label %28, !dbg !71

26:                                               ; preds = %19
  %27 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !72
  br label %28, !dbg !74

28:                                               ; preds = %26, %19
  %29 = load i32, ptr %7, align 4, !dbg !75
  %30 = icmp sgt i32 %29, 20, !dbg !77
  br i1 %30, label %31, label %33, !dbg !77

31:                                               ; preds = %28
  %32 = call i32 (ptr, ...) @printf(ptr noundef @.str.2), !dbg !78
  br label %33, !dbg !80

33:                                               ; preds = %31, %28
    #dbg_declare(ptr %8, !81, !DIExpression(), !82)
  %34 = load i32, ptr %2, align 4, !dbg !83
  %35 = add nsw i32 %34, 5, !dbg !84
  store i32 %35, ptr %8, align 4, !dbg !82
    #dbg_declare(ptr %9, !85, !DIExpression(), !86)
  %36 = load i32, ptr %3, align 4, !dbg !87
  %37 = mul nsw i32 %36, 2, !dbg !88
  store i32 %37, ptr %9, align 4, !dbg !86
  %38 = load i32, ptr %8, align 4, !dbg !89
  %39 = icmp sgt i32 %38, 10, !dbg !91
  br i1 %39, label %40, label %45, !dbg !92

40:                                               ; preds = %33
  %41 = load i32, ptr %9, align 4, !dbg !93
  %42 = icmp sgt i32 %41, 10, !dbg !94
  br i1 %42, label %43, label %45, !dbg !92

43:                                               ; preds = %40
  %44 = call i32 (ptr, ...) @printf(ptr noundef @.str.3), !dbg !95
  br label %45, !dbg !97

45:                                               ; preds = %43, %40, %33
    #dbg_declare(ptr %10, !98, !DIExpression(), !99)
  %46 = load i32, ptr %2, align 4, !dbg !100
  %47 = load i32, ptr %3, align 4, !dbg !101
  %48 = add nsw i32 %46, %47, !dbg !102
  store i32 %48, ptr %10, align 4, !dbg !99
  %49 = load i32, ptr %10, align 4, !dbg !103
  %50 = mul nsw i32 %49, 2, !dbg !105
  %51 = icmp sgt i32 %50, 25, !dbg !106
  br i1 %51, label %52, label %54, !dbg !106

52:                                               ; preds = %45
  %53 = call i32 (ptr, ...) @printf(ptr noundef @.str.4), !dbg !107
  br label %54, !dbg !109

54:                                               ; preds = %52, %45
    #dbg_declare(ptr %11, !110, !DIExpression(), !111)
  %55 = load i32, ptr %2, align 4, !dbg !112
  store i32 %55, ptr %11, align 4, !dbg !111
    #dbg_declare(ptr %12, !113, !DIExpression(), !114)
  %56 = load i32, ptr %3, align 4, !dbg !115
  store i32 %56, ptr %12, align 4, !dbg !114
    #dbg_declare(ptr %13, !116, !DIExpression(), !117)
  %57 = load i32, ptr %4, align 4, !dbg !118
  store i32 %57, ptr %13, align 4, !dbg !117
  %58 = load i32, ptr %11, align 4, !dbg !119
  %59 = load i32, ptr %12, align 4, !dbg !121
  %60 = add nsw i32 %58, %59, !dbg !122
  %61 = load i32, ptr %13, align 4, !dbg !123
  %62 = add nsw i32 %60, %61, !dbg !124
  %63 = icmp sgt i32 %62, 30, !dbg !125
  br i1 %63, label %64, label %66, !dbg !125

64:                                               ; preds = %54
  %65 = call i32 (ptr, ...) @printf(ptr noundef @.str.5), !dbg !126
  br label %66, !dbg !128

66:                                               ; preds = %64, %54
  %67 = load i32, ptr %2, align 4, !dbg !129
  %68 = icmp sgt i32 %67, 2, !dbg !131
  br i1 %68, label %69, label %79, !dbg !131

69:                                               ; preds = %66
  %70 = load i32, ptr %3, align 4, !dbg !132
  %71 = icmp sgt i32 %70, 5, !dbg !135
  br i1 %71, label %72, label %78, !dbg !135

72:                                               ; preds = %69
  %73 = load i32, ptr %4, align 4, !dbg !136
  %74 = icmp sgt i32 %73, 10, !dbg !139
  br i1 %74, label %75, label %77, !dbg !139

75:                                               ; preds = %72
  %76 = call i32 (ptr, ...) @printf(ptr noundef @.str.6), !dbg !140
  br label %77, !dbg !142

77:                                               ; preds = %75, %72
  br label %78, !dbg !143

78:                                               ; preds = %77, %69
  br label %79, !dbg !144

79:                                               ; preds = %78, %66
  %80 = load i32, ptr %2, align 4, !dbg !145
  %81 = icmp sgt i32 %80, 100, !dbg !147
  br i1 %81, label %85, label %82, !dbg !148

82:                                               ; preds = %79
  %83 = load i32, ptr %3, align 4, !dbg !149
  %84 = icmp sgt i32 %83, 100, !dbg !150
  br i1 %84, label %85, label %87, !dbg !148

85:                                               ; preds = %82, %79
  %86 = call i32 (ptr, ...) @printf(ptr noundef @.str.7), !dbg !151
  br label %87, !dbg !153

87:                                               ; preds = %85, %82
  %88 = load i32, ptr %2, align 4, !dbg !154
  %89 = load i32, ptr %3, align 4, !dbg !156
  %90 = icmp sgt i32 %88, %89, !dbg !157
  br i1 %90, label %91, label %93, !dbg !157

91:                                               ; preds = %87
  %92 = call i32 (ptr, ...) @printf(ptr noundef @.str.8), !dbg !158
  br label %93, !dbg !160

93:                                               ; preds = %91, %87
  ret i32 0, !dbg !161
}

declare void @dfsan_set_label(i8 noundef zeroext, ptr noundef, i64 noundef) #1

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!29}
!llvm.module.flags = !{!31, !32, !33, !34, !35, !36, !37}
!llvm.ident = !{!38}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 17, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_leaking", checksumkind: CSK_MD5, checksum: "da59049ef53dd48883ce7c0b7bf57ce9")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 152, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 19)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 24, type: !3, isLocal: true, isDefinition: true)
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(scope: null, file: !2, line: 28, type: !11, isLocal: true, isDefinition: true)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 96, elements: !12)
!12 = !{!13}
!13 = !DISubrange(count: 12)
!14 = !DIGlobalVariableExpression(var: !15, expr: !DIExpression())
!15 = distinct !DIGlobalVariable(scope: null, file: !2, line: 35, type: !3, isLocal: true, isDefinition: true)
!16 = !DIGlobalVariableExpression(var: !17, expr: !DIExpression())
!17 = distinct !DIGlobalVariable(scope: null, file: !2, line: 41, type: !3, isLocal: true, isDefinition: true)
!18 = !DIGlobalVariableExpression(var: !19, expr: !DIExpression())
!19 = distinct !DIGlobalVariable(scope: null, file: !2, line: 49, type: !3, isLocal: true, isDefinition: true)
!20 = !DIGlobalVariableExpression(var: !21, expr: !DIExpression())
!21 = distinct !DIGlobalVariable(scope: null, file: !2, line: 55, type: !22, isLocal: true, isDefinition: true)
!22 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 224, elements: !23)
!23 = !{!24}
!24 = !DISubrange(count: 28)
!25 = !DIGlobalVariableExpression(var: !26, expr: !DIExpression())
!26 = distinct !DIGlobalVariable(scope: null, file: !2, line: 61, type: !3, isLocal: true, isDefinition: true)
!27 = !DIGlobalVariableExpression(var: !28, expr: !DIExpression())
!28 = distinct !DIGlobalVariable(scope: null, file: !2, line: 65, type: !3, isLocal: true, isDefinition: true)
!29 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !30, splitDebugInlining: false, nameTableKind: None)
!30 = !{!0, !7, !9, !14, !16, !18, !20, !25, !27}
!31 = !{i32 7, !"Dwarf Version", i32 5}
!32 = !{i32 2, !"Debug Info Version", i32 3}
!33 = !{i32 1, !"wchar_size", i32 4}
!34 = !{i32 8, !"PIC Level", i32 2}
!35 = !{i32 7, !"PIE Level", i32 2}
!36 = !{i32 7, !"uwtable", i32 2}
!37 = !{i32 7, !"frame-pointer", i32 2}
!38 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!39 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !40, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !43)
!40 = !DISubroutineType(types: !41)
!41 = !{!42}
!42 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!43 = !{}
!44 = !DILocalVariable(name: "secret1", scope: !39, file: !2, line: 6, type: !42)
!45 = !DILocation(line: 6, column: 9, scope: !39)
!46 = !DILocalVariable(name: "secret2", scope: !39, file: !2, line: 7, type: !42)
!47 = !DILocation(line: 7, column: 9, scope: !39)
!48 = !DILocalVariable(name: "secret3", scope: !39, file: !2, line: 8, type: !42)
!49 = !DILocation(line: 8, column: 9, scope: !39)
!50 = !DILocation(line: 10, column: 5, scope: !39)
!51 = !DILocation(line: 11, column: 5, scope: !39)
!52 = !DILocalVariable(name: "x", scope: !39, file: !2, line: 14, type: !42)
!53 = !DILocation(line: 14, column: 9, scope: !39)
!54 = !DILocation(line: 14, column: 13, scope: !39)
!55 = !DILocation(line: 16, column: 9, scope: !56)
!56 = distinct !DILexicalBlock(scope: !39, file: !2, line: 16, column: 9)
!57 = !DILocation(line: 16, column: 11, scope: !56)
!58 = !DILocation(line: 17, column: 9, scope: !59)
!59 = distinct !DILexicalBlock(scope: !56, file: !2, line: 16, column: 17)
!60 = !DILocation(line: 18, column: 5, scope: !59)
!61 = !DILocalVariable(name: "y", scope: !39, file: !2, line: 20, type: !42)
!62 = !DILocation(line: 20, column: 9, scope: !39)
!63 = !DILocation(line: 20, column: 13, scope: !39)
!64 = !DILocalVariable(name: "z", scope: !39, file: !2, line: 21, type: !42)
!65 = !DILocation(line: 21, column: 9, scope: !39)
!66 = !DILocation(line: 21, column: 13, scope: !39)
!67 = !DILocation(line: 23, column: 9, scope: !68)
!68 = distinct !DILexicalBlock(scope: !39, file: !2, line: 23, column: 9)
!69 = !DILocation(line: 23, column: 13, scope: !68)
!70 = !DILocation(line: 23, column: 11, scope: !68)
!71 = !DILocation(line: 23, column: 15, scope: !68)
!72 = !DILocation(line: 24, column: 9, scope: !73)
!73 = distinct !DILexicalBlock(scope: !68, file: !2, line: 23, column: 21)
!74 = !DILocation(line: 25, column: 5, scope: !73)
!75 = !DILocation(line: 27, column: 9, scope: !76)
!76 = distinct !DILexicalBlock(scope: !39, file: !2, line: 27, column: 9)
!77 = !DILocation(line: 27, column: 11, scope: !76)
!78 = !DILocation(line: 28, column: 9, scope: !79)
!79 = distinct !DILexicalBlock(scope: !76, file: !2, line: 27, column: 17)
!80 = !DILocation(line: 29, column: 5, scope: !79)
!81 = !DILocalVariable(name: "a", scope: !39, file: !2, line: 31, type: !42)
!82 = !DILocation(line: 31, column: 9, scope: !39)
!83 = !DILocation(line: 31, column: 13, scope: !39)
!84 = !DILocation(line: 31, column: 21, scope: !39)
!85 = !DILocalVariable(name: "b", scope: !39, file: !2, line: 32, type: !42)
!86 = !DILocation(line: 32, column: 9, scope: !39)
!87 = !DILocation(line: 32, column: 13, scope: !39)
!88 = !DILocation(line: 32, column: 21, scope: !39)
!89 = !DILocation(line: 34, column: 9, scope: !90)
!90 = distinct !DILexicalBlock(scope: !39, file: !2, line: 34, column: 9)
!91 = !DILocation(line: 34, column: 11, scope: !90)
!92 = !DILocation(line: 34, column: 16, scope: !90)
!93 = !DILocation(line: 34, column: 19, scope: !90)
!94 = !DILocation(line: 34, column: 21, scope: !90)
!95 = !DILocation(line: 35, column: 9, scope: !96)
!96 = distinct !DILexicalBlock(scope: !90, file: !2, line: 34, column: 27)
!97 = !DILocation(line: 36, column: 5, scope: !96)
!98 = !DILocalVariable(name: "c", scope: !39, file: !2, line: 38, type: !42)
!99 = !DILocation(line: 38, column: 9, scope: !39)
!100 = !DILocation(line: 38, column: 13, scope: !39)
!101 = !DILocation(line: 38, column: 23, scope: !39)
!102 = !DILocation(line: 38, column: 21, scope: !39)
!103 = !DILocation(line: 40, column: 10, scope: !104)
!104 = distinct !DILexicalBlock(scope: !39, file: !2, line: 40, column: 9)
!105 = !DILocation(line: 40, column: 12, scope: !104)
!106 = !DILocation(line: 40, column: 17, scope: !104)
!107 = !DILocation(line: 41, column: 9, scope: !108)
!108 = distinct !DILexicalBlock(scope: !104, file: !2, line: 40, column: 23)
!109 = !DILocation(line: 42, column: 5, scope: !108)
!110 = !DILocalVariable(name: "p", scope: !39, file: !2, line: 44, type: !42)
!111 = !DILocation(line: 44, column: 9, scope: !39)
!112 = !DILocation(line: 44, column: 13, scope: !39)
!113 = !DILocalVariable(name: "q", scope: !39, file: !2, line: 45, type: !42)
!114 = !DILocation(line: 45, column: 9, scope: !39)
!115 = !DILocation(line: 45, column: 13, scope: !39)
!116 = !DILocalVariable(name: "r", scope: !39, file: !2, line: 46, type: !42)
!117 = !DILocation(line: 46, column: 9, scope: !39)
!118 = !DILocation(line: 46, column: 13, scope: !39)
!119 = !DILocation(line: 48, column: 9, scope: !120)
!120 = distinct !DILexicalBlock(scope: !39, file: !2, line: 48, column: 9)
!121 = !DILocation(line: 48, column: 13, scope: !120)
!122 = !DILocation(line: 48, column: 11, scope: !120)
!123 = !DILocation(line: 48, column: 17, scope: !120)
!124 = !DILocation(line: 48, column: 15, scope: !120)
!125 = !DILocation(line: 48, column: 19, scope: !120)
!126 = !DILocation(line: 49, column: 9, scope: !127)
!127 = distinct !DILexicalBlock(scope: !120, file: !2, line: 48, column: 25)
!128 = !DILocation(line: 50, column: 5, scope: !127)
!129 = !DILocation(line: 52, column: 9, scope: !130)
!130 = distinct !DILexicalBlock(scope: !39, file: !2, line: 52, column: 9)
!131 = !DILocation(line: 52, column: 17, scope: !130)
!132 = !DILocation(line: 53, column: 13, scope: !133)
!133 = distinct !DILexicalBlock(scope: !134, file: !2, line: 53, column: 13)
!134 = distinct !DILexicalBlock(scope: !130, file: !2, line: 52, column: 22)
!135 = !DILocation(line: 53, column: 21, scope: !133)
!136 = !DILocation(line: 54, column: 17, scope: !137)
!137 = distinct !DILexicalBlock(scope: !138, file: !2, line: 54, column: 17)
!138 = distinct !DILexicalBlock(scope: !133, file: !2, line: 53, column: 26)
!139 = !DILocation(line: 54, column: 25, scope: !137)
!140 = !DILocation(line: 55, column: 17, scope: !141)
!141 = distinct !DILexicalBlock(scope: !137, file: !2, line: 54, column: 31)
!142 = !DILocation(line: 56, column: 13, scope: !141)
!143 = !DILocation(line: 57, column: 9, scope: !138)
!144 = !DILocation(line: 58, column: 5, scope: !134)
!145 = !DILocation(line: 60, column: 9, scope: !146)
!146 = distinct !DILexicalBlock(scope: !39, file: !2, line: 60, column: 9)
!147 = !DILocation(line: 60, column: 17, scope: !146)
!148 = !DILocation(line: 60, column: 23, scope: !146)
!149 = !DILocation(line: 60, column: 26, scope: !146)
!150 = !DILocation(line: 60, column: 34, scope: !146)
!151 = !DILocation(line: 61, column: 9, scope: !152)
!152 = distinct !DILexicalBlock(scope: !146, file: !2, line: 60, column: 41)
!153 = !DILocation(line: 62, column: 5, scope: !152)
!154 = !DILocation(line: 64, column: 9, scope: !155)
!155 = distinct !DILexicalBlock(scope: !39, file: !2, line: 64, column: 9)
!156 = !DILocation(line: 64, column: 19, scope: !155)
!157 = !DILocation(line: 64, column: 17, scope: !155)
!158 = !DILocation(line: 65, column: 9, scope: !159)
!159 = distinct !DILexicalBlock(scope: !155, file: !2, line: 64, column: 28)
!160 = !DILocation(line: 66, column: 5, scope: !159)
!161 = !DILocation(line: 68, column: 5, scope: !39)
