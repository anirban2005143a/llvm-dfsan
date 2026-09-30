; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [169 x i8] c"secret1   label = %u\0Asecret2   label = %u\0Ax         label = %u\0Ay         label = %u\0Az         label = %u\0Aa         label = %u\0Ab         label = %u\0Ac         label = %u\0A\00", align 1, !dbg !0
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !20 {
  %1 = alloca i8, align 1
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
    #dbg_declare(ptr %3, !25, !DIExpression(), !26)
  %11 = ptrtoint ptr %3 to i64, !dbg !26
  %12 = xor i64 %11, 87960930222080, !dbg !26
  %13 = inttoptr i64 %12 to ptr, !dbg !26
  store i32 0, ptr %13, align 1, !dbg !26
  store i32 1, ptr %3, align 4, !dbg !26
    #dbg_declare(ptr %4, !27, !DIExpression(), !28)
  %14 = ptrtoint ptr %4 to i64, !dbg !28
  %15 = xor i64 %14, 87960930222080, !dbg !28
  %16 = inttoptr i64 %15 to ptr, !dbg !28
  store i32 0, ptr %16, align 1, !dbg !28
  store i32 1, ptr %4, align 4, !dbg !28
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !29
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !30
    #dbg_declare(ptr %5, !31, !DIExpression(), !32)
  %17 = ptrtoint ptr %5 to i64, !dbg !32
  %18 = xor i64 %17, 87960930222080, !dbg !32
  %19 = inttoptr i64 %18 to ptr, !dbg !32
  store i32 0, ptr %19, align 1, !dbg !32
  store i32 0, ptr %5, align 4, !dbg !32
    #dbg_declare(ptr %6, !33, !DIExpression(), !34)
  %20 = ptrtoint ptr %6 to i64, !dbg !34
  %21 = xor i64 %20, 87960930222080, !dbg !34
  %22 = inttoptr i64 %21 to ptr, !dbg !34
  store i32 0, ptr %22, align 1, !dbg !34
  store i32 0, ptr %6, align 4, !dbg !34
    #dbg_declare(ptr %7, !35, !DIExpression(), !36)
  %23 = ptrtoint ptr %7 to i64, !dbg !36
  %24 = xor i64 %23, 87960930222080, !dbg !36
  %25 = inttoptr i64 %24 to ptr, !dbg !36
  store i32 0, ptr %25, align 1, !dbg !36
  store i32 0, ptr %7, align 4, !dbg !36
    #dbg_declare(ptr %8, !37, !DIExpression(), !38)
  %26 = ptrtoint ptr %8 to i64, !dbg !38
  %27 = xor i64 %26, 87960930222080, !dbg !38
  %28 = inttoptr i64 %27 to ptr, !dbg !38
  store i32 0, ptr %28, align 1, !dbg !38
  store i32 0, ptr %8, align 4, !dbg !38
    #dbg_declare(ptr %9, !39, !DIExpression(), !40)
  %29 = ptrtoint ptr %9 to i64, !dbg !40
  %30 = xor i64 %29, 87960930222080, !dbg !40
  %31 = inttoptr i64 %30 to ptr, !dbg !40
  store i32 0, ptr %31, align 1, !dbg !40
  store i32 0, ptr %9, align 4, !dbg !40
    #dbg_declare(ptr %10, !41, !DIExpression(), !42)
  %32 = ptrtoint ptr %10 to i64, !dbg !42
  %33 = xor i64 %32, 87960930222080, !dbg !42
  %34 = inttoptr i64 %33 to ptr, !dbg !42
  store i32 0, ptr %34, align 1, !dbg !42
  store i32 0, ptr %10, align 4, !dbg !42
  %35 = ptrtoint ptr %3 to i64, !dbg !43
  %36 = xor i64 %35, 87960930222080, !dbg !43
  %37 = inttoptr i64 %36 to ptr, !dbg !43
  %38 = load i32, ptr %37, align 1, !dbg !43
  %39 = lshr i32 %38, 16, !dbg !43
  %40 = or i32 %38, %39, !dbg !43
  %41 = lshr i32 %40, 8, !dbg !43
  %42 = or i32 %40, %41, !dbg !43
  %43 = trunc i32 %42 to i8, !dbg !43
  %44 = load i32, ptr %3, align 4, !dbg !43
  %45 = ptrtoint ptr %4 to i64, !dbg !45
  %46 = xor i64 %45, 87960930222080, !dbg !45
  %47 = inttoptr i64 %46 to ptr, !dbg !45
  %48 = load i32, ptr %47, align 1, !dbg !45
  %49 = lshr i32 %48, 16, !dbg !45
  %50 = or i32 %48, %49, !dbg !45
  %51 = lshr i32 %50, 8, !dbg !45
  %52 = or i32 %50, %51, !dbg !45
  %53 = trunc i32 %52 to i8, !dbg !45
  %54 = load i32, ptr %4, align 4, !dbg !45
  %55 = or i8 %43, %53, !dbg !46
  %56 = add nsw i32 %44, %54, !dbg !46
  %57 = icmp ne i32 %56, 0, !dbg !46
  call void @__dfsan_conditional_callback(i8 zeroext %55), !dbg !46
  br i1 %57, label %58, label %62, !dbg !46

58:                                               ; preds = %0
  %59 = ptrtoint ptr %5 to i64, !dbg !47
  %60 = xor i64 %59, 87960930222080, !dbg !47
  %61 = inttoptr i64 %60 to ptr, !dbg !47
  store i32 0, ptr %61, align 1, !dbg !47
  store i32 10, ptr %5, align 4, !dbg !47
  br label %83, !dbg !49

62:                                               ; preds = %0
  %63 = ptrtoint ptr %3 to i64, !dbg !50
  %64 = xor i64 %63, 87960930222080, !dbg !50
  %65 = inttoptr i64 %64 to ptr, !dbg !50
  %66 = load i32, ptr %65, align 1, !dbg !50
  %67 = lshr i32 %66, 16, !dbg !50
  %68 = or i32 %66, %67, !dbg !50
  %69 = lshr i32 %68, 8, !dbg !50
  %70 = or i32 %68, %69, !dbg !50
  %71 = trunc i32 %70 to i8, !dbg !50
  %72 = load i32, ptr %3, align 4, !dbg !50
  %73 = icmp ne i32 %72, 0, !dbg !50
  call void @__dfsan_conditional_callback(i8 zeroext %71), !dbg !50
  br i1 %73, label %74, label %78, !dbg !50

74:                                               ; preds = %62
  %75 = ptrtoint ptr %6 to i64, !dbg !52
  %76 = xor i64 %75, 87960930222080, !dbg !52
  %77 = inttoptr i64 %76 to ptr, !dbg !52
  store i32 0, ptr %77, align 1, !dbg !52
  store i32 10, ptr %6, align 4, !dbg !52
  br label %82, !dbg !54

78:                                               ; preds = %62
  %79 = ptrtoint ptr %7 to i64, !dbg !55
  %80 = xor i64 %79, 87960930222080, !dbg !55
  %81 = inttoptr i64 %80 to ptr, !dbg !55
  store i32 0, ptr %81, align 1, !dbg !55
  store i32 10, ptr %7, align 4, !dbg !55
  br label %82

82:                                               ; preds = %78, %74
  br label %83

83:                                               ; preds = %82, %58
  %84 = ptrtoint ptr %5 to i64, !dbg !57
  %85 = xor i64 %84, 87960930222080, !dbg !57
  %86 = inttoptr i64 %85 to ptr, !dbg !57
  %87 = load i32, ptr %86, align 1, !dbg !57
  %88 = lshr i32 %87, 16, !dbg !57
  %89 = or i32 %87, %88, !dbg !57
  %90 = lshr i32 %89, 8, !dbg !57
  %91 = or i32 %89, %90, !dbg !57
  %92 = trunc i32 %91 to i8, !dbg !57
  %93 = load i32, ptr %5, align 4, !dbg !57
  %94 = ptrtoint ptr %8 to i64, !dbg !58
  %95 = xor i64 %94, 87960930222080, !dbg !58
  %96 = inttoptr i64 %95 to ptr, !dbg !58
  %97 = getelementptr i8, ptr %96, i32 0, !dbg !58
  store i8 %92, ptr %97, align 1, !dbg !58
  %98 = getelementptr i8, ptr %96, i32 1, !dbg !58
  store i8 %92, ptr %98, align 1, !dbg !58
  %99 = getelementptr i8, ptr %96, i32 2, !dbg !58
  store i8 %92, ptr %99, align 1, !dbg !58
  %100 = getelementptr i8, ptr %96, i32 3, !dbg !58
  store i8 %92, ptr %100, align 1, !dbg !58
  store i32 %93, ptr %8, align 4, !dbg !58
  %101 = ptrtoint ptr %6 to i64, !dbg !59
  %102 = xor i64 %101, 87960930222080, !dbg !59
  %103 = inttoptr i64 %102 to ptr, !dbg !59
  %104 = load i32, ptr %103, align 1, !dbg !59
  %105 = lshr i32 %104, 16, !dbg !59
  %106 = or i32 %104, %105, !dbg !59
  %107 = lshr i32 %106, 8, !dbg !59
  %108 = or i32 %106, %107, !dbg !59
  %109 = trunc i32 %108 to i8, !dbg !59
  %110 = load i32, ptr %6, align 4, !dbg !59
  %111 = ptrtoint ptr %9 to i64, !dbg !60
  %112 = xor i64 %111, 87960930222080, !dbg !60
  %113 = inttoptr i64 %112 to ptr, !dbg !60
  %114 = getelementptr i8, ptr %113, i32 0, !dbg !60
  store i8 %109, ptr %114, align 1, !dbg !60
  %115 = getelementptr i8, ptr %113, i32 1, !dbg !60
  store i8 %109, ptr %115, align 1, !dbg !60
  %116 = getelementptr i8, ptr %113, i32 2, !dbg !60
  store i8 %109, ptr %116, align 1, !dbg !60
  %117 = getelementptr i8, ptr %113, i32 3, !dbg !60
  store i8 %109, ptr %117, align 1, !dbg !60
  store i32 %110, ptr %9, align 4, !dbg !60
  %118 = ptrtoint ptr %7 to i64, !dbg !61
  %119 = xor i64 %118, 87960930222080, !dbg !61
  %120 = inttoptr i64 %119 to ptr, !dbg !61
  %121 = load i32, ptr %120, align 1, !dbg !61
  %122 = lshr i32 %121, 16, !dbg !61
  %123 = or i32 %121, %122, !dbg !61
  %124 = lshr i32 %123, 8, !dbg !61
  %125 = or i32 %123, %124, !dbg !61
  %126 = trunc i32 %125 to i8, !dbg !61
  %127 = load i32, ptr %7, align 4, !dbg !61
  %128 = ptrtoint ptr %10 to i64, !dbg !62
  %129 = xor i64 %128, 87960930222080, !dbg !62
  %130 = inttoptr i64 %129 to ptr, !dbg !62
  %131 = getelementptr i8, ptr %130, i32 0, !dbg !62
  store i8 %126, ptr %131, align 1, !dbg !62
  %132 = getelementptr i8, ptr %130, i32 1, !dbg !62
  store i8 %126, ptr %132, align 1, !dbg !62
  %133 = getelementptr i8, ptr %130, i32 2, !dbg !62
  store i8 %126, ptr %133, align 1, !dbg !62
  %134 = getelementptr i8, ptr %130, i32 3, !dbg !62
  store i8 %126, ptr %134, align 1, !dbg !62
  store i32 %127, ptr %10, align 4, !dbg !62
  %135 = call zeroext i8 @dfsan_read_label(ptr noundef %3, i64 noundef 4), !dbg !63
  %136 = zext i8 %135 to i32, !dbg !64
  %137 = call zeroext i8 @dfsan_read_label(ptr noundef %4, i64 noundef 4), !dbg !65
  %138 = zext i8 %137 to i32, !dbg !66
  %139 = call zeroext i8 @dfsan_read_label(ptr noundef %5, i64 noundef 4), !dbg !67
  %140 = zext i8 %139 to i32, !dbg !68
  %141 = call zeroext i8 @dfsan_read_label(ptr noundef %6, i64 noundef 4), !dbg !69
  %142 = zext i8 %141 to i32, !dbg !70
  %143 = call zeroext i8 @dfsan_read_label(ptr noundef %7, i64 noundef 4), !dbg !71
  %144 = zext i8 %143 to i32, !dbg !72
  %145 = call zeroext i8 @dfsan_read_label(ptr noundef %8, i64 noundef 4), !dbg !73
  %146 = zext i8 %145 to i32, !dbg !74
  %147 = call zeroext i8 @dfsan_read_label(ptr noundef %9, i64 noundef 4), !dbg !75
  %148 = zext i8 %147 to i32, !dbg !76
  %149 = call zeroext i8 @dfsan_read_label(ptr noundef %10, i64 noundef 4), !dbg !77
  %150 = zext i8 %149 to i32, !dbg !78
  %151 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %136, i32 noundef %138, i32 noundef %140, i32 noundef %142, i32 noundef %144, i32 noundef %146, i32 noundef %148, i32 noundef %150), !dbg !79
  ret i32 0, !dbg !80
}

declare void @dfsan_set_label(i8 noundef zeroext, ptr noundef, i64 noundef) #1

declare i32 @printf(ptr noundef, ...) #1

declare zeroext i8 @dfsan_read_label(ptr noundef, i64 noundef) #1

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

define linkonce_odr zeroext i8 @"dfsw$dfsan_read_label"(ptr noundef %0, i64 noundef %1) #1 {
  %3 = call i8 @dfsan_read_label(ptr %0, i64 %1)
  store i8 0, ptr @__dfsan_retval_tls, align 2
  ret i8 %3
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!7}
!llvm.module.flags = !{!11, !12, !13, !14, !15, !16, !17, !18}
!llvm.ident = !{!19}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 27, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_taint_propagation", checksumkind: CSK_MD5, checksum: "82e4f6233cc003dc9b6f8706b244124f")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 1352, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 169)
!7 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !8, globals: !10, splitDebugInlining: false, nameTableKind: None)
!8 = !{!9}
!9 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!10 = !{!0}
!11 = !{i32 7, !"Dwarf Version", i32 5}
!12 = !{i32 2, !"Debug Info Version", i32 3}
!13 = !{i32 1, !"wchar_size", i32 4}
!14 = !{i32 8, !"PIC Level", i32 2}
!15 = !{i32 7, !"PIE Level", i32 2}
!16 = !{i32 7, !"uwtable", i32 2}
!17 = !{i32 7, !"frame-pointer", i32 2}
!18 = !{i32 4, !"nosanitize_dataflow", i32 1}
!19 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!20 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !21, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !7, retainedNodes: !24)
!21 = !DISubroutineType(types: !22)
!22 = !{!23}
!23 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!24 = !{}
!25 = !DILocalVariable(name: "secret1", scope: !20, file: !2, line: 6, type: !23)
!26 = !DILocation(line: 6, column: 9, scope: !20)
!27 = !DILocalVariable(name: "secret2", scope: !20, file: !2, line: 7, type: !23)
!28 = !DILocation(line: 7, column: 9, scope: !20)
!29 = !DILocation(line: 9, column: 5, scope: !20)
!30 = !DILocation(line: 10, column: 5, scope: !20)
!31 = !DILocalVariable(name: "x", scope: !20, file: !2, line: 12, type: !23)
!32 = !DILocation(line: 12, column: 9, scope: !20)
!33 = !DILocalVariable(name: "y", scope: !20, file: !2, line: 12, type: !23)
!34 = !DILocation(line: 12, column: 16, scope: !20)
!35 = !DILocalVariable(name: "z", scope: !20, file: !2, line: 12, type: !23)
!36 = !DILocation(line: 12, column: 23, scope: !20)
!37 = !DILocalVariable(name: "a", scope: !20, file: !2, line: 12, type: !23)
!38 = !DILocation(line: 12, column: 30, scope: !20)
!39 = !DILocalVariable(name: "b", scope: !20, file: !2, line: 12, type: !23)
!40 = !DILocation(line: 12, column: 37, scope: !20)
!41 = !DILocalVariable(name: "c", scope: !20, file: !2, line: 12, type: !23)
!42 = !DILocation(line: 12, column: 44, scope: !20)
!43 = !DILocation(line: 14, column: 8, scope: !44)
!44 = distinct !DILexicalBlock(scope: !20, file: !2, line: 14, column: 8)
!45 = !DILocation(line: 14, column: 18, scope: !44)
!46 = !DILocation(line: 14, column: 16, scope: !44)
!47 = !DILocation(line: 15, column: 11, scope: !48)
!48 = distinct !DILexicalBlock(scope: !44, file: !2, line: 14, column: 26)
!49 = !DILocation(line: 16, column: 5, scope: !48)
!50 = !DILocation(line: 16, column: 15, scope: !51)
!51 = distinct !DILexicalBlock(scope: !44, file: !2, line: 16, column: 15)
!52 = !DILocation(line: 17, column: 11, scope: !53)
!53 = distinct !DILexicalBlock(scope: !51, file: !2, line: 16, column: 23)
!54 = !DILocation(line: 18, column: 5, scope: !53)
!55 = !DILocation(line: 19, column: 11, scope: !56)
!56 = distinct !DILexicalBlock(scope: !51, file: !2, line: 18, column: 12)
!57 = !DILocation(line: 22, column: 9, scope: !20)
!58 = !DILocation(line: 22, column: 7, scope: !20)
!59 = !DILocation(line: 23, column: 9, scope: !20)
!60 = !DILocation(line: 23, column: 7, scope: !20)
!61 = !DILocation(line: 24, column: 9, scope: !20)
!62 = !DILocation(line: 24, column: 7, scope: !20)
!63 = !DILocation(line: 44, column: 19, scope: !20)
!64 = !DILocation(line: 44, column: 9, scope: !20)
!65 = !DILocation(line: 47, column: 19, scope: !20)
!66 = !DILocation(line: 47, column: 9, scope: !20)
!67 = !DILocation(line: 50, column: 19, scope: !20)
!68 = !DILocation(line: 50, column: 9, scope: !20)
!69 = !DILocation(line: 53, column: 19, scope: !20)
!70 = !DILocation(line: 53, column: 9, scope: !20)
!71 = !DILocation(line: 56, column: 19, scope: !20)
!72 = !DILocation(line: 56, column: 9, scope: !20)
!73 = !DILocation(line: 59, column: 19, scope: !20)
!74 = !DILocation(line: 59, column: 9, scope: !20)
!75 = !DILocation(line: 62, column: 19, scope: !20)
!76 = !DILocation(line: 62, column: 9, scope: !20)
!77 = !DILocation(line: 65, column: 19, scope: !20)
!78 = !DILocation(line: 65, column: 9, scope: !20)
!79 = !DILocation(line: 26, column: 5, scope: !20)
!80 = !DILocation(line: 70, column: 5, scope: !20)
