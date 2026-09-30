; ModuleID = 'dfsan.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [105 x i8] c"secret1 = %u\0Asecret2 = %u\0Ax       = %u\0Ay       = %u\0Az       = %u\0Aa       = %u\0Ab       = %u\0Ac       = %u\0A\00", align 1, !dbg !0
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1
@implicit.variable.name = private unnamed_addr constant [2 x i8] c"z\00", align 1
@implicit.variable.name.1 = private unnamed_addr constant [2 x i8] c"z\00", align 1
@implicit.variable.name.2 = private unnamed_addr constant [2 x i8] c"y\00", align 1
@implicit.variable.name.3 = private unnamed_addr constant [2 x i8] c"y\00", align 1
@implicit.variable.name.4 = private unnamed_addr constant [2 x i8] c"x\00", align 1

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
  store i32 0, ptr %4, align 4, !dbg !28
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !29
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !30
    #dbg_declare(ptr %5, !31, !DIExpression(), !32)
  call void @dfsan_add_label(i8 1, ptr %5, i64 4), !dbg !32
  call void @dfsan_add_label(i8 1, ptr %7, i64 4), !dbg !32
  call void @dfsan_add_label(i8 1, ptr %6, i64 4), !dbg !32
  call void @dfsan_add_label(i8 2, ptr %6, i64 4), !dbg !32
  call void @dfsan_add_label(i8 2, ptr %7, i64 4), !dbg !32
  %17 = ptrtoint ptr %5 to i64, !dbg !32
  %18 = xor i64 %17, 87960930222080, !dbg !32
  %19 = inttoptr i64 %18 to ptr, !dbg !32
  store i32 0, ptr %19, align 1, !dbg !32
  store i32 0, ptr %5, align 4, !dbg !32
    #dbg_declare(ptr %6, !33, !DIExpression(), !34)
  call void @dfsan_add_label(i8 1, ptr %5, i64 4), !dbg !34
  %20 = ptrtoint ptr %6 to i64, !dbg !34
  %21 = xor i64 %20, 87960930222080, !dbg !34
  %22 = inttoptr i64 %21 to ptr, !dbg !34
  store i32 0, ptr %22, align 1, !dbg !34
  store i32 0, ptr %6, align 4, !dbg !34
    #dbg_declare(ptr %7, !35, !DIExpression(), !36)
  call void @dfsan_add_label(i8 2, ptr %6, i64 4), !dbg !36
  call void @dfsan_add_label(i8 1, ptr %6, i64 4), !dbg !36
  %23 = ptrtoint ptr %7 to i64, !dbg !36
  %24 = xor i64 %23, 87960930222080, !dbg !36
  %25 = inttoptr i64 %24 to ptr, !dbg !36
  store i32 0, ptr %25, align 1, !dbg !36
  store i32 0, ptr %7, align 4, !dbg !36
    #dbg_declare(ptr %8, !37, !DIExpression(), !38)
  call void @dfsan_add_label(i8 2, ptr %7, i64 4), !dbg !38
  call void @dfsan_add_label(i8 1, ptr %7, i64 4), !dbg !38
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
  %45 = icmp ne i32 %44, 0, !dbg !43
  br i1 %45, label %46, label %50, !dbg !43

46:                                               ; preds = %0
  %47 = ptrtoint ptr %5 to i64, !dbg !45
  %48 = xor i64 %47, 87960930222080, !dbg !45
  %49 = inttoptr i64 %48 to ptr, !dbg !45
  store i32 0, ptr %49, align 1, !dbg !45
  store i32 10, ptr %5, align 4, !dbg !45
  call void @dfsan_add_label(i8 1, ptr %5, i64 4), !dbg !47
  br label %77, !dbg !47

50:                                               ; preds = %0
  %51 = ptrtoint ptr %4 to i64, !dbg !48
  %52 = xor i64 %51, 87960930222080, !dbg !48
  %53 = inttoptr i64 %52 to ptr, !dbg !48
  %54 = load i32, ptr %53, align 1, !dbg !48
  %55 = lshr i32 %54, 16, !dbg !48
  %56 = or i32 %54, %55, !dbg !48
  %57 = lshr i32 %56, 8, !dbg !48
  %58 = or i32 %56, %57, !dbg !48
  %59 = trunc i32 %58 to i8, !dbg !48
  %60 = load i32, ptr %4, align 4, !dbg !48
  %61 = icmp ne i32 %60, 0, !dbg !48
  br i1 %61, label %62, label %69, !dbg !48

62:                                               ; preds = %50
  %63 = ptrtoint ptr %5 to i64, !dbg !50
  %64 = xor i64 %63, 87960930222080, !dbg !50
  %65 = inttoptr i64 %64 to ptr, !dbg !50
  store i32 0, ptr %65, align 1, !dbg !50
  store i32 10, ptr %5, align 4, !dbg !50
  call void @dfsan_add_label(i8 1, ptr %5, i64 4), !dbg !52
  %66 = ptrtoint ptr %6 to i64, !dbg !52
  %67 = xor i64 %66, 87960930222080, !dbg !52
  %68 = inttoptr i64 %67 to ptr, !dbg !52
  store i32 0, ptr %68, align 1, !dbg !52
  store i32 40, ptr %6, align 4, !dbg !52
  call void @dfsan_add_label(i8 2, ptr %6, i64 4), !dbg !53
  call void @dfsan_add_label(i8 1, ptr %6, i64 4), !dbg !53
  br label %76, !dbg !53

69:                                               ; preds = %50
  %70 = ptrtoint ptr %5 to i64, !dbg !54
  %71 = xor i64 %70, 87960930222080, !dbg !54
  %72 = inttoptr i64 %71 to ptr, !dbg !54
  store i32 0, ptr %72, align 1, !dbg !54
  store i32 10, ptr %5, align 4, !dbg !54
  call void @dfsan_add_label(i8 1, ptr %5, i64 4), !dbg !56
  %73 = ptrtoint ptr %7 to i64, !dbg !56
  %74 = xor i64 %73, 87960930222080, !dbg !56
  %75 = inttoptr i64 %74 to ptr, !dbg !56
  store i32 0, ptr %75, align 1, !dbg !56
  store i32 50, ptr %7, align 4, !dbg !56
  call void @dfsan_add_label(i8 2, ptr %7, i64 4)
  call void @dfsan_add_label(i8 1, ptr %7, i64 4)
  br label %76

76:                                               ; preds = %69, %62
  br label %77

77:                                               ; preds = %76, %46
  %78 = ptrtoint ptr %5 to i64, !dbg !57
  %79 = xor i64 %78, 87960930222080, !dbg !57
  %80 = inttoptr i64 %79 to ptr, !dbg !57
  %81 = load i32, ptr %80, align 1, !dbg !57
  %82 = lshr i32 %81, 16, !dbg !57
  %83 = or i32 %81, %82, !dbg !57
  %84 = lshr i32 %83, 8, !dbg !57
  %85 = or i32 %83, %84, !dbg !57
  %86 = trunc i32 %85 to i8, !dbg !57
  %87 = load i32, ptr %5, align 4, !dbg !57
  %88 = ptrtoint ptr %8 to i64, !dbg !58
  %89 = xor i64 %88, 87960930222080, !dbg !58
  %90 = inttoptr i64 %89 to ptr, !dbg !58
  %91 = getelementptr i8, ptr %90, i32 0, !dbg !58
  store i8 %86, ptr %91, align 1, !dbg !58
  %92 = getelementptr i8, ptr %90, i32 1, !dbg !58
  store i8 %86, ptr %92, align 1, !dbg !58
  %93 = getelementptr i8, ptr %90, i32 2, !dbg !58
  store i8 %86, ptr %93, align 1, !dbg !58
  %94 = getelementptr i8, ptr %90, i32 3, !dbg !58
  store i8 %86, ptr %94, align 1, !dbg !58
  store i32 %87, ptr %8, align 4, !dbg !58
  %95 = ptrtoint ptr %6 to i64, !dbg !59
  %96 = xor i64 %95, 87960930222080, !dbg !59
  %97 = inttoptr i64 %96 to ptr, !dbg !59
  %98 = load i32, ptr %97, align 1, !dbg !59
  %99 = lshr i32 %98, 16, !dbg !59
  %100 = or i32 %98, %99, !dbg !59
  %101 = lshr i32 %100, 8, !dbg !59
  %102 = or i32 %100, %101, !dbg !59
  %103 = trunc i32 %102 to i8, !dbg !59
  %104 = load i32, ptr %6, align 4, !dbg !59
  %105 = ptrtoint ptr %9 to i64, !dbg !60
  %106 = xor i64 %105, 87960930222080, !dbg !60
  %107 = inttoptr i64 %106 to ptr, !dbg !60
  %108 = getelementptr i8, ptr %107, i32 0, !dbg !60
  store i8 %103, ptr %108, align 1, !dbg !60
  %109 = getelementptr i8, ptr %107, i32 1, !dbg !60
  store i8 %103, ptr %109, align 1, !dbg !60
  %110 = getelementptr i8, ptr %107, i32 2, !dbg !60
  store i8 %103, ptr %110, align 1, !dbg !60
  %111 = getelementptr i8, ptr %107, i32 3, !dbg !60
  store i8 %103, ptr %111, align 1, !dbg !60
  store i32 %104, ptr %9, align 4, !dbg !60
  %112 = ptrtoint ptr %7 to i64, !dbg !61
  %113 = xor i64 %112, 87960930222080, !dbg !61
  %114 = inttoptr i64 %113 to ptr, !dbg !61
  %115 = load i32, ptr %114, align 1, !dbg !61
  %116 = lshr i32 %115, 16, !dbg !61
  %117 = or i32 %115, %116, !dbg !61
  %118 = lshr i32 %117, 8, !dbg !61
  %119 = or i32 %117, %118, !dbg !61
  %120 = trunc i32 %119 to i8, !dbg !61
  %121 = load i32, ptr %7, align 4, !dbg !61
  %122 = ptrtoint ptr %10 to i64, !dbg !62
  %123 = xor i64 %122, 87960930222080, !dbg !62
  %124 = inttoptr i64 %123 to ptr, !dbg !62
  %125 = getelementptr i8, ptr %124, i32 0, !dbg !62
  store i8 %120, ptr %125, align 1, !dbg !62
  %126 = getelementptr i8, ptr %124, i32 1, !dbg !62
  store i8 %120, ptr %126, align 1, !dbg !62
  %127 = getelementptr i8, ptr %124, i32 2, !dbg !62
  store i8 %120, ptr %127, align 1, !dbg !62
  %128 = getelementptr i8, ptr %124, i32 3, !dbg !62
  store i8 %120, ptr %128, align 1, !dbg !62
  store i32 %121, ptr %10, align 4, !dbg !62
  %129 = call zeroext i8 @dfsan_read_label(ptr noundef %3, i64 noundef 4), !dbg !63
  %130 = zext i8 %129 to i32, !dbg !64
  %131 = call zeroext i8 @dfsan_read_label(ptr noundef %4, i64 noundef 4), !dbg !65
  %132 = zext i8 %131 to i32, !dbg !66
  %133 = call zeroext i8 @dfsan_read_label(ptr noundef %5, i64 noundef 4), !dbg !67
  %134 = zext i8 %133 to i32, !dbg !68
  %135 = call zeroext i8 @dfsan_read_label(ptr noundef %6, i64 noundef 4), !dbg !69
  %136 = zext i8 %135 to i32, !dbg !70
  %137 = call zeroext i8 @dfsan_read_label(ptr noundef %7, i64 noundef 4), !dbg !71
  %138 = zext i8 %137 to i32, !dbg !72
  %139 = call zeroext i8 @dfsan_read_label(ptr noundef %8, i64 noundef 4), !dbg !73
  %140 = zext i8 %139 to i32, !dbg !74
  %141 = call zeroext i8 @dfsan_read_label(ptr noundef %9, i64 noundef 4), !dbg !75
  %142 = zext i8 %141 to i32, !dbg !76
  %143 = call zeroext i8 @dfsan_read_label(ptr noundef %10, i64 noundef 4), !dbg !77
  %144 = zext i8 %143 to i32, !dbg !78
  %145 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %130, i32 noundef %132, i32 noundef %134, i32 noundef %136, i32 noundef %138, i32 noundef %140, i32 noundef %142, i32 noundef %144), !dbg !79
  %implicit.final.label = call i8 @dfsan_read_label(ptr %7, i64 4), !dbg !80
  call void @__implicit_branch_callback(i8 1, i8 %implicit.final.label, i32 27, i32 9, ptr @implicit.variable.name), !dbg !80
  %implicit.final.label1 = call i8 @dfsan_read_label(ptr %7, i64 4), !dbg !80
  call void @__implicit_branch_callback(i8 2, i8 %implicit.final.label1, i32 29, i32 16, ptr @implicit.variable.name.1), !dbg !80
  %implicit.final.label2 = call i8 @dfsan_read_label(ptr %6, i64 4), !dbg !80
  call void @__implicit_branch_callback(i8 1, i8 %implicit.final.label2, i32 27, i32 9, ptr @implicit.variable.name.2), !dbg !80
  %implicit.final.label3 = call i8 @dfsan_read_label(ptr %6, i64 4), !dbg !80
  call void @__implicit_branch_callback(i8 2, i8 %implicit.final.label3, i32 29, i32 16, ptr @implicit.variable.name.3), !dbg !80
  %implicit.final.label4 = call i8 @dfsan_read_label(ptr %5, i64 4), !dbg !80
  call void @__implicit_branch_callback(i8 1, i8 %implicit.final.label4, i32 27, i32 9, ptr @implicit.variable.name.4), !dbg !80
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

declare i8 @dfsan_union(i8, i8)

declare void @dfsan_add_label(i8, ptr, i64)

declare void @__implicit_branch_callback(i8, i8, i32, i32, ptr)

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!7}
!llvm.module.flags = !{!11, !12, !13, !14, !15, !16, !17, !18}
!llvm.ident = !{!19}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 42, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_taint_propagation", checksumkind: CSK_MD5, checksum: "a71f02dea5e1149aa6ad09dae57a7f65")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 840, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 105)
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
!20 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 5, type: !21, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !7, retainedNodes: !24)
!21 = !DISubroutineType(types: !22)
!22 = !{!23}
!23 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!24 = !{}
!25 = !DILocalVariable(name: "secret1", scope: !20, file: !2, line: 7, type: !23)
!26 = !DILocation(line: 7, column: 9, scope: !20)
!27 = !DILocalVariable(name: "secret2", scope: !20, file: !2, line: 8, type: !23)
!28 = !DILocation(line: 8, column: 9, scope: !20)
!29 = !DILocation(line: 10, column: 5, scope: !20)
!30 = !DILocation(line: 15, column: 5, scope: !20)
!31 = !DILocalVariable(name: "x", scope: !20, file: !2, line: 20, type: !23)
!32 = !DILocation(line: 20, column: 9, scope: !20)
!33 = !DILocalVariable(name: "y", scope: !20, file: !2, line: 21, type: !23)
!34 = !DILocation(line: 21, column: 9, scope: !20)
!35 = !DILocalVariable(name: "z", scope: !20, file: !2, line: 22, type: !23)
!36 = !DILocation(line: 22, column: 9, scope: !20)
!37 = !DILocalVariable(name: "a", scope: !20, file: !2, line: 23, type: !23)
!38 = !DILocation(line: 23, column: 9, scope: !20)
!39 = !DILocalVariable(name: "b", scope: !20, file: !2, line: 24, type: !23)
!40 = !DILocation(line: 24, column: 9, scope: !20)
!41 = !DILocalVariable(name: "c", scope: !20, file: !2, line: 25, type: !23)
!42 = !DILocation(line: 25, column: 9, scope: !20)
!43 = !DILocation(line: 27, column: 9, scope: !44)
!44 = distinct !DILexicalBlock(scope: !20, file: !2, line: 27, column: 9)
!45 = !DILocation(line: 28, column: 11, scope: !46)
!46 = distinct !DILexicalBlock(scope: !44, file: !2, line: 27, column: 18)
!47 = !DILocation(line: 29, column: 5, scope: !46)
!48 = !DILocation(line: 29, column: 16, scope: !49)
!49 = distinct !DILexicalBlock(scope: !44, file: !2, line: 29, column: 16)
!50 = !DILocation(line: 30, column: 11, scope: !51)
!51 = distinct !DILexicalBlock(scope: !49, file: !2, line: 29, column: 25)
!52 = !DILocation(line: 31, column: 11, scope: !51)
!53 = !DILocation(line: 32, column: 5, scope: !51)
!54 = !DILocation(line: 33, column: 11, scope: !55)
!55 = distinct !DILexicalBlock(scope: !49, file: !2, line: 32, column: 12)
!56 = !DILocation(line: 34, column: 11, scope: !55)
!57 = !DILocation(line: 37, column: 9, scope: !20)
!58 = !DILocation(line: 37, column: 7, scope: !20)
!59 = !DILocation(line: 38, column: 9, scope: !20)
!60 = !DILocation(line: 38, column: 7, scope: !20)
!61 = !DILocation(line: 39, column: 9, scope: !20)
!62 = !DILocation(line: 39, column: 7, scope: !20)
!63 = !DILocation(line: 51, column: 19, scope: !20)
!64 = !DILocation(line: 51, column: 9, scope: !20)
!65 = !DILocation(line: 55, column: 19, scope: !20)
!66 = !DILocation(line: 55, column: 9, scope: !20)
!67 = !DILocation(line: 59, column: 19, scope: !20)
!68 = !DILocation(line: 59, column: 9, scope: !20)
!69 = !DILocation(line: 63, column: 19, scope: !20)
!70 = !DILocation(line: 63, column: 9, scope: !20)
!71 = !DILocation(line: 67, column: 19, scope: !20)
!72 = !DILocation(line: 67, column: 9, scope: !20)
!73 = !DILocation(line: 71, column: 19, scope: !20)
!74 = !DILocation(line: 71, column: 9, scope: !20)
!75 = !DILocation(line: 75, column: 19, scope: !20)
!76 = !DILocation(line: 75, column: 9, scope: !20)
!77 = !DILocation(line: 79, column: 19, scope: !20)
!78 = !DILocation(line: 79, column: 9, scope: !20)
!79 = !DILocation(line: 41, column: 5, scope: !20)
!80 = !DILocation(line: 84, column: 5, scope: !20)
