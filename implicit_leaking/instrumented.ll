; ModuleID = 'test.ll'
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
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1
@implicit.file.1 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.1 = private unnamed_addr constant [7 x i8] c"x > 10\00", align 1
@implicit.llvmcond.1 = private unnamed_addr constant [4 x i8] c"%43\00", align 1
@implicit.vars.1 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.1 = private unnamed_addr constant [4 x i8] c"%44\00", align 1
@implicit.false.1 = private unnamed_addr constant [4 x i8] c"%46\00", align 1
@implicit.file.2 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.2 = private unnamed_addr constant [11 x i8] c"y + z > 20\00", align 1
@implicit.llvmcond.2 = private unnamed_addr constant [4 x i8] c"%65\00", align 1
@implicit.vars.2 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.2 = private unnamed_addr constant [4 x i8] c"%66\00", align 1
@implicit.false.2 = private unnamed_addr constant [4 x i8] c"%68\00", align 1
@implicit.file.3 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.3 = private unnamed_addr constant [7 x i8] c"z > 20\00", align 1
@implicit.llvmcond.3 = private unnamed_addr constant [4 x i8] c"%71\00", align 1
@implicit.vars.3 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.3 = private unnamed_addr constant [4 x i8] c"%72\00", align 1
@implicit.false.3 = private unnamed_addr constant [4 x i8] c"%74\00", align 1
@implicit.file.4 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.4 = private unnamed_addr constant [17 x i8] c"a > 10 && b > 10\00", align 1
@implicit.llvmcond.4 = private unnamed_addr constant [4 x i8] c"%99\00", align 1
@implicit.vars.4 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.4 = private unnamed_addr constant [5 x i8] c"%100\00", align 1
@implicit.false.4 = private unnamed_addr constant [5 x i8] c"%106\00", align 1
@implicit.file.5 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.5 = private unnamed_addr constant [17 x i8] c"a > 10 && b > 10\00", align 1
@implicit.llvmcond.5 = private unnamed_addr constant [5 x i8] c"%103\00", align 1
@implicit.vars.5 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.5 = private unnamed_addr constant [5 x i8] c"%104\00", align 1
@implicit.false.5 = private unnamed_addr constant [5 x i8] c"%106\00", align 1
@implicit.file.6 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.6 = private unnamed_addr constant [13 x i8] c"(c * 2) > 25\00", align 1
@implicit.llvmcond.6 = private unnamed_addr constant [5 x i8] c"%132\00", align 1
@implicit.vars.6 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.6 = private unnamed_addr constant [5 x i8] c"%133\00", align 1
@implicit.false.6 = private unnamed_addr constant [5 x i8] c"%135\00", align 1
@implicit.file.7 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.7 = private unnamed_addr constant [15 x i8] c"p + q + r > 30\00", align 1
@implicit.llvmcond.7 = private unnamed_addr constant [5 x i8] c"%168\00", align 1
@implicit.vars.7 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.7 = private unnamed_addr constant [5 x i8] c"%169\00", align 1
@implicit.false.7 = private unnamed_addr constant [5 x i8] c"%171\00", align 1
@implicit.file.8 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.8 = private unnamed_addr constant [12 x i8] c"secret1 > 2\00", align 1
@implicit.llvmcond.8 = private unnamed_addr constant [5 x i8] c"%182\00", align 1
@implicit.vars.8 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.8 = private unnamed_addr constant [5 x i8] c"%183\00", align 1
@implicit.false.8 = private unnamed_addr constant [5 x i8] c"%203\00", align 1
@implicit.file.9 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.9 = private unnamed_addr constant [12 x i8] c"secret2 > 5\00", align 1
@implicit.llvmcond.9 = private unnamed_addr constant [5 x i8] c"%194\00", align 1
@implicit.vars.9 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.9 = private unnamed_addr constant [5 x i8] c"%195\00", align 1
@implicit.false.9 = private unnamed_addr constant [5 x i8] c"%202\00", align 1
@implicit.file.10 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.10 = private unnamed_addr constant [13 x i8] c"secret3 > 10\00", align 1
@implicit.llvmcond.10 = private unnamed_addr constant [5 x i8] c"%198\00", align 1
@implicit.vars.10 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.10 = private unnamed_addr constant [5 x i8] c"%199\00", align 1
@implicit.false.10 = private unnamed_addr constant [5 x i8] c"%201\00", align 1
@implicit.file.11 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.11 = private unnamed_addr constant [31 x i8] c"secret1 > 100 || secret2 > 100\00", align 1
@implicit.llvmcond.11 = private unnamed_addr constant [5 x i8] c"%214\00", align 1
@implicit.vars.11 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.11 = private unnamed_addr constant [5 x i8] c"%227\00", align 1
@implicit.false.11 = private unnamed_addr constant [5 x i8] c"%215\00", align 1
@implicit.file.12 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.12 = private unnamed_addr constant [31 x i8] c"secret1 > 100 || secret2 > 100\00", align 1
@implicit.llvmcond.12 = private unnamed_addr constant [5 x i8] c"%226\00", align 1
@implicit.vars.12 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.12 = private unnamed_addr constant [5 x i8] c"%227\00", align 1
@implicit.false.12 = private unnamed_addr constant [5 x i8] c"%229\00", align 1
@implicit.file.13 = private unnamed_addr constant [7 x i8] c"test.c\00", align 1
@implicit.condition.13 = private unnamed_addr constant [18 x i8] c"secret1 > secret2\00", align 1
@implicit.llvmcond.13 = private unnamed_addr constant [5 x i8] c"%251\00", align 1
@implicit.vars.13 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@implicit.true.13 = private unnamed_addr constant [5 x i8] c"%252\00", align 1
@implicit.false.13 = private unnamed_addr constant [5 x i8] c"%254\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !40 {
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
  %11 = alloca i8, align 1
  %12 = alloca i32, align 4
  %13 = alloca i8, align 1
  %14 = alloca i32, align 4
  %15 = alloca i8, align 1
  %16 = alloca i32, align 4
  %17 = alloca i8, align 1
  %18 = alloca i32, align 4
  %19 = alloca i8, align 1
  %20 = alloca i32, align 4
  %21 = alloca i8, align 1
  %22 = alloca i32, align 4
  %23 = alloca i8, align 1
  %24 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
    #dbg_declare(ptr %3, !45, !DIExpression(), !46)
  %25 = ptrtoint ptr %3 to i64, !dbg !46
  %26 = xor i64 %25, 87960930222080, !dbg !46
  %27 = inttoptr i64 %26 to ptr, !dbg !46
  store i32 0, ptr %27, align 1, !dbg !46
  store i32 5, ptr %3, align 4, !dbg !46
    #dbg_declare(ptr %4, !47, !DIExpression(), !48)
  %28 = ptrtoint ptr %4 to i64, !dbg !48
  %29 = xor i64 %28, 87960930222080, !dbg !48
  %30 = inttoptr i64 %29 to ptr, !dbg !48
  store i32 0, ptr %30, align 1, !dbg !48
  store i32 8, ptr %4, align 4, !dbg !48
    #dbg_declare(ptr %6, !49, !DIExpression(), !50)
  store i8 0, ptr %5, align 1, !dbg !50
  store i32 12, ptr %6, align 4, !dbg !50
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !51
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !52
    #dbg_declare(ptr %8, !53, !DIExpression(), !54)
  %31 = ptrtoint ptr %3 to i64, !dbg !55
  %32 = xor i64 %31, 87960930222080, !dbg !55
  %33 = inttoptr i64 %32 to ptr, !dbg !55
  %34 = load i32, ptr %33, align 1, !dbg !55
  %35 = lshr i32 %34, 16, !dbg !55
  %36 = or i32 %34, %35, !dbg !55
  %37 = lshr i32 %36, 8, !dbg !55
  %38 = or i32 %36, %37, !dbg !55
  %39 = trunc i32 %38 to i8, !dbg !55
  %40 = load i32, ptr %3, align 4, !dbg !55
  store i8 %39, ptr %7, align 1, !dbg !54
  store i32 %40, ptr %8, align 4, !dbg !54
  %41 = load i8, ptr %7, align 1, !dbg !56
  %42 = load i32, ptr %8, align 4, !dbg !56
  %43 = icmp sgt i32 %42, 10, !dbg !58
  %implicit.taken = zext i1 %43 to i8, !dbg !58
  call void @__implicit_conditional_callback(i8 %41, i32 1, i8 %implicit.taken, ptr @implicit.file.1, i32 16, i32 11, ptr @implicit.condition.1, ptr @implicit.llvmcond.1, ptr @implicit.vars.1, ptr @implicit.true.1, ptr @implicit.false.1), !dbg !58
  br i1 %43, label %44, label %46, !dbg !58

44:                                               ; preds = %0
  %45 = call i32 (ptr, ...) @printf(ptr noundef @.str), !dbg !59
  br label %46, !dbg !61

46:                                               ; preds = %44, %0
    #dbg_declare(ptr %10, !62, !DIExpression(), !63)
  %47 = ptrtoint ptr %4 to i64, !dbg !64
  %48 = xor i64 %47, 87960930222080, !dbg !64
  %49 = inttoptr i64 %48 to ptr, !dbg !64
  %50 = load i32, ptr %49, align 1, !dbg !64
  %51 = lshr i32 %50, 16, !dbg !64
  %52 = or i32 %50, %51, !dbg !64
  %53 = lshr i32 %52, 8, !dbg !64
  %54 = or i32 %52, %53, !dbg !64
  %55 = trunc i32 %54 to i8, !dbg !64
  %56 = load i32, ptr %4, align 4, !dbg !64
  store i8 %55, ptr %9, align 1, !dbg !63
  store i32 %56, ptr %10, align 4, !dbg !63
    #dbg_declare(ptr %12, !65, !DIExpression(), !66)
  %57 = load i8, ptr %5, align 1, !dbg !67
  %58 = load i32, ptr %6, align 4, !dbg !67
  store i8 %57, ptr %11, align 1, !dbg !66
  store i32 %58, ptr %12, align 4, !dbg !66
  %59 = load i8, ptr %9, align 1, !dbg !68
  %60 = load i32, ptr %10, align 4, !dbg !68
  %61 = load i8, ptr %11, align 1, !dbg !70
  %62 = load i32, ptr %12, align 4, !dbg !70
  %63 = or i8 %59, %61, !dbg !71
  %64 = add nsw i32 %60, %62, !dbg !71
  %65 = icmp sgt i32 %64, 20, !dbg !72
  %implicit.taken1 = zext i1 %65 to i8, !dbg !72
  call void @__implicit_conditional_callback(i8 %63, i32 2, i8 %implicit.taken1, ptr @implicit.file.2, i32 23, i32 15, ptr @implicit.condition.2, ptr @implicit.llvmcond.2, ptr @implicit.vars.2, ptr @implicit.true.2, ptr @implicit.false.2), !dbg !72
  br i1 %65, label %66, label %68, !dbg !72

66:                                               ; preds = %46
  %67 = call i32 (ptr, ...) @printf(ptr noundef @.str.1), !dbg !73
  br label %68, !dbg !75

68:                                               ; preds = %66, %46
  %69 = load i8, ptr %11, align 1, !dbg !76
  %70 = load i32, ptr %12, align 4, !dbg !76
  %71 = icmp sgt i32 %70, 20, !dbg !78
  %implicit.taken2 = zext i1 %71 to i8, !dbg !78
  call void @__implicit_conditional_callback(i8 %69, i32 3, i8 %implicit.taken2, ptr @implicit.file.3, i32 27, i32 11, ptr @implicit.condition.3, ptr @implicit.llvmcond.3, ptr @implicit.vars.3, ptr @implicit.true.3, ptr @implicit.false.3), !dbg !78
  br i1 %71, label %72, label %74, !dbg !78

72:                                               ; preds = %68
  %73 = call i32 (ptr, ...) @printf(ptr noundef @.str.2), !dbg !79
  br label %74, !dbg !81

74:                                               ; preds = %72, %68
    #dbg_declare(ptr %14, !82, !DIExpression(), !83)
  %75 = ptrtoint ptr %3 to i64, !dbg !84
  %76 = xor i64 %75, 87960930222080, !dbg !84
  %77 = inttoptr i64 %76 to ptr, !dbg !84
  %78 = load i32, ptr %77, align 1, !dbg !84
  %79 = lshr i32 %78, 16, !dbg !84
  %80 = or i32 %78, %79, !dbg !84
  %81 = lshr i32 %80, 8, !dbg !84
  %82 = or i32 %80, %81, !dbg !84
  %83 = trunc i32 %82 to i8, !dbg !84
  %84 = load i32, ptr %3, align 4, !dbg !84
  %85 = add nsw i32 %84, 5, !dbg !85
  store i8 %83, ptr %13, align 1, !dbg !83
  store i32 %85, ptr %14, align 4, !dbg !83
    #dbg_declare(ptr %16, !86, !DIExpression(), !87)
  %86 = ptrtoint ptr %4 to i64, !dbg !88
  %87 = xor i64 %86, 87960930222080, !dbg !88
  %88 = inttoptr i64 %87 to ptr, !dbg !88
  %89 = load i32, ptr %88, align 1, !dbg !88
  %90 = lshr i32 %89, 16, !dbg !88
  %91 = or i32 %89, %90, !dbg !88
  %92 = lshr i32 %91, 8, !dbg !88
  %93 = or i32 %91, %92, !dbg !88
  %94 = trunc i32 %93 to i8, !dbg !88
  %95 = load i32, ptr %4, align 4, !dbg !88
  %96 = mul nsw i32 %95, 2, !dbg !89
  store i8 %94, ptr %15, align 1, !dbg !87
  store i32 %96, ptr %16, align 4, !dbg !87
  %97 = load i8, ptr %13, align 1, !dbg !90
  %98 = load i32, ptr %14, align 4, !dbg !90
  %99 = icmp sgt i32 %98, 10, !dbg !92
  %implicit.taken3 = zext i1 %99 to i8, !dbg !93
  call void @__implicit_conditional_callback(i8 %97, i32 4, i8 %implicit.taken3, ptr @implicit.file.4, i32 34, i32 16, ptr @implicit.condition.4, ptr @implicit.llvmcond.4, ptr @implicit.vars.4, ptr @implicit.true.4, ptr @implicit.false.4), !dbg !93
  br i1 %99, label %100, label %106, !dbg !93

100:                                              ; preds = %74
  %101 = load i8, ptr %15, align 1, !dbg !94
  %102 = load i32, ptr %16, align 4, !dbg !94
  %103 = icmp sgt i32 %102, 10, !dbg !95
  %implicit.taken4 = zext i1 %103 to i8, !dbg !93
  call void @__implicit_conditional_callback(i8 %101, i32 5, i8 %implicit.taken4, ptr @implicit.file.5, i32 34, i32 16, ptr @implicit.condition.5, ptr @implicit.llvmcond.5, ptr @implicit.vars.5, ptr @implicit.true.5, ptr @implicit.false.5), !dbg !93
  br i1 %103, label %104, label %106, !dbg !93

104:                                              ; preds = %100
  %105 = call i32 (ptr, ...) @printf(ptr noundef @.str.3), !dbg !96
  br label %106, !dbg !98

106:                                              ; preds = %104, %100, %74
    #dbg_declare(ptr %18, !99, !DIExpression(), !100)
  %107 = ptrtoint ptr %3 to i64, !dbg !101
  %108 = xor i64 %107, 87960930222080, !dbg !101
  %109 = inttoptr i64 %108 to ptr, !dbg !101
  %110 = load i32, ptr %109, align 1, !dbg !101
  %111 = lshr i32 %110, 16, !dbg !101
  %112 = or i32 %110, %111, !dbg !101
  %113 = lshr i32 %112, 8, !dbg !101
  %114 = or i32 %112, %113, !dbg !101
  %115 = trunc i32 %114 to i8, !dbg !101
  %116 = load i32, ptr %3, align 4, !dbg !101
  %117 = ptrtoint ptr %4 to i64, !dbg !102
  %118 = xor i64 %117, 87960930222080, !dbg !102
  %119 = inttoptr i64 %118 to ptr, !dbg !102
  %120 = load i32, ptr %119, align 1, !dbg !102
  %121 = lshr i32 %120, 16, !dbg !102
  %122 = or i32 %120, %121, !dbg !102
  %123 = lshr i32 %122, 8, !dbg !102
  %124 = or i32 %122, %123, !dbg !102
  %125 = trunc i32 %124 to i8, !dbg !102
  %126 = load i32, ptr %4, align 4, !dbg !102
  %127 = or i8 %115, %125, !dbg !103
  %128 = add nsw i32 %116, %126, !dbg !103
  store i8 %127, ptr %17, align 1, !dbg !100
  store i32 %128, ptr %18, align 4, !dbg !100
  %129 = load i8, ptr %17, align 1, !dbg !104
  %130 = load i32, ptr %18, align 4, !dbg !104
  %131 = mul nsw i32 %130, 2, !dbg !106
  %132 = icmp sgt i32 %131, 25, !dbg !107
  %implicit.taken5 = zext i1 %132 to i8, !dbg !107
  call void @__implicit_conditional_callback(i8 %129, i32 6, i8 %implicit.taken5, ptr @implicit.file.6, i32 40, i32 17, ptr @implicit.condition.6, ptr @implicit.llvmcond.6, ptr @implicit.vars.6, ptr @implicit.true.6, ptr @implicit.false.6), !dbg !107
  br i1 %132, label %133, label %135, !dbg !107

133:                                              ; preds = %106
  %134 = call i32 (ptr, ...) @printf(ptr noundef @.str.4), !dbg !108
  br label %135, !dbg !110

135:                                              ; preds = %133, %106
    #dbg_declare(ptr %20, !111, !DIExpression(), !112)
  %136 = ptrtoint ptr %3 to i64, !dbg !113
  %137 = xor i64 %136, 87960930222080, !dbg !113
  %138 = inttoptr i64 %137 to ptr, !dbg !113
  %139 = load i32, ptr %138, align 1, !dbg !113
  %140 = lshr i32 %139, 16, !dbg !113
  %141 = or i32 %139, %140, !dbg !113
  %142 = lshr i32 %141, 8, !dbg !113
  %143 = or i32 %141, %142, !dbg !113
  %144 = trunc i32 %143 to i8, !dbg !113
  %145 = load i32, ptr %3, align 4, !dbg !113
  store i8 %144, ptr %19, align 1, !dbg !112
  store i32 %145, ptr %20, align 4, !dbg !112
    #dbg_declare(ptr %22, !114, !DIExpression(), !115)
  %146 = ptrtoint ptr %4 to i64, !dbg !116
  %147 = xor i64 %146, 87960930222080, !dbg !116
  %148 = inttoptr i64 %147 to ptr, !dbg !116
  %149 = load i32, ptr %148, align 1, !dbg !116
  %150 = lshr i32 %149, 16, !dbg !116
  %151 = or i32 %149, %150, !dbg !116
  %152 = lshr i32 %151, 8, !dbg !116
  %153 = or i32 %151, %152, !dbg !116
  %154 = trunc i32 %153 to i8, !dbg !116
  %155 = load i32, ptr %4, align 4, !dbg !116
  store i8 %154, ptr %21, align 1, !dbg !115
  store i32 %155, ptr %22, align 4, !dbg !115
    #dbg_declare(ptr %24, !117, !DIExpression(), !118)
  %156 = load i8, ptr %5, align 1, !dbg !119
  %157 = load i32, ptr %6, align 4, !dbg !119
  store i8 %156, ptr %23, align 1, !dbg !118
  store i32 %157, ptr %24, align 4, !dbg !118
  %158 = load i8, ptr %19, align 1, !dbg !120
  %159 = load i32, ptr %20, align 4, !dbg !120
  %160 = load i8, ptr %21, align 1, !dbg !122
  %161 = load i32, ptr %22, align 4, !dbg !122
  %162 = or i8 %158, %160, !dbg !123
  %163 = add nsw i32 %159, %161, !dbg !123
  %164 = load i8, ptr %23, align 1, !dbg !124
  %165 = load i32, ptr %24, align 4, !dbg !124
  %166 = or i8 %162, %164, !dbg !125
  %167 = add nsw i32 %163, %165, !dbg !125
  %168 = icmp sgt i32 %167, 30, !dbg !126
  %implicit.taken6 = zext i1 %168 to i8, !dbg !126
  call void @__implicit_conditional_callback(i8 %166, i32 7, i8 %implicit.taken6, ptr @implicit.file.7, i32 48, i32 19, ptr @implicit.condition.7, ptr @implicit.llvmcond.7, ptr @implicit.vars.7, ptr @implicit.true.7, ptr @implicit.false.7), !dbg !126
  br i1 %168, label %169, label %171, !dbg !126

169:                                              ; preds = %135
  %170 = call i32 (ptr, ...) @printf(ptr noundef @.str.5), !dbg !127
  br label %171, !dbg !129

171:                                              ; preds = %169, %135
  %172 = ptrtoint ptr %3 to i64, !dbg !130
  %173 = xor i64 %172, 87960930222080, !dbg !130
  %174 = inttoptr i64 %173 to ptr, !dbg !130
  %175 = load i32, ptr %174, align 1, !dbg !130
  %176 = lshr i32 %175, 16, !dbg !130
  %177 = or i32 %175, %176, !dbg !130
  %178 = lshr i32 %177, 8, !dbg !130
  %179 = or i32 %177, %178, !dbg !130
  %180 = trunc i32 %179 to i8, !dbg !130
  %181 = load i32, ptr %3, align 4, !dbg !130
  %182 = icmp sgt i32 %181, 2, !dbg !132
  %implicit.taken7 = zext i1 %182 to i8, !dbg !132
  call void @__implicit_conditional_callback(i8 %180, i32 8, i8 %implicit.taken7, ptr @implicit.file.8, i32 52, i32 17, ptr @implicit.condition.8, ptr @implicit.llvmcond.8, ptr @implicit.vars.8, ptr @implicit.true.8, ptr @implicit.false.8), !dbg !132
  br i1 %182, label %183, label %203, !dbg !132

183:                                              ; preds = %171
  %184 = ptrtoint ptr %4 to i64, !dbg !133
  %185 = xor i64 %184, 87960930222080, !dbg !133
  %186 = inttoptr i64 %185 to ptr, !dbg !133
  %187 = load i32, ptr %186, align 1, !dbg !133
  %188 = lshr i32 %187, 16, !dbg !133
  %189 = or i32 %187, %188, !dbg !133
  %190 = lshr i32 %189, 8, !dbg !133
  %191 = or i32 %189, %190, !dbg !133
  %192 = trunc i32 %191 to i8, !dbg !133
  %193 = load i32, ptr %4, align 4, !dbg !133
  %194 = icmp sgt i32 %193, 5, !dbg !136
  %implicit.taken8 = zext i1 %194 to i8, !dbg !136
  call void @__implicit_conditional_callback(i8 %192, i32 9, i8 %implicit.taken8, ptr @implicit.file.9, i32 53, i32 21, ptr @implicit.condition.9, ptr @implicit.llvmcond.9, ptr @implicit.vars.9, ptr @implicit.true.9, ptr @implicit.false.9), !dbg !136
  br i1 %194, label %195, label %202, !dbg !136

195:                                              ; preds = %183
  %196 = load i8, ptr %5, align 1, !dbg !137
  %197 = load i32, ptr %6, align 4, !dbg !137
  %198 = icmp sgt i32 %197, 10, !dbg !140
  %implicit.taken9 = zext i1 %198 to i8, !dbg !140
  call void @__implicit_conditional_callback(i8 %196, i32 10, i8 %implicit.taken9, ptr @implicit.file.10, i32 54, i32 25, ptr @implicit.condition.10, ptr @implicit.llvmcond.10, ptr @implicit.vars.10, ptr @implicit.true.10, ptr @implicit.false.10), !dbg !140
  br i1 %198, label %199, label %201, !dbg !140

199:                                              ; preds = %195
  %200 = call i32 (ptr, ...) @printf(ptr noundef @.str.6), !dbg !141
  br label %201, !dbg !143

201:                                              ; preds = %199, %195
  br label %202, !dbg !144

202:                                              ; preds = %201, %183
  br label %203, !dbg !145

203:                                              ; preds = %202, %171
  %204 = ptrtoint ptr %3 to i64, !dbg !146
  %205 = xor i64 %204, 87960930222080, !dbg !146
  %206 = inttoptr i64 %205 to ptr, !dbg !146
  %207 = load i32, ptr %206, align 1, !dbg !146
  %208 = lshr i32 %207, 16, !dbg !146
  %209 = or i32 %207, %208, !dbg !146
  %210 = lshr i32 %209, 8, !dbg !146
  %211 = or i32 %209, %210, !dbg !146
  %212 = trunc i32 %211 to i8, !dbg !146
  %213 = load i32, ptr %3, align 4, !dbg !146
  %214 = icmp sgt i32 %213, 100, !dbg !148
  %implicit.taken10 = zext i1 %214 to i8, !dbg !149
  call void @__implicit_conditional_callback(i8 %212, i32 11, i8 %implicit.taken10, ptr @implicit.file.11, i32 60, i32 23, ptr @implicit.condition.11, ptr @implicit.llvmcond.11, ptr @implicit.vars.11, ptr @implicit.true.11, ptr @implicit.false.11), !dbg !149
  br i1 %214, label %227, label %215, !dbg !149

215:                                              ; preds = %203
  %216 = ptrtoint ptr %4 to i64, !dbg !150
  %217 = xor i64 %216, 87960930222080, !dbg !150
  %218 = inttoptr i64 %217 to ptr, !dbg !150
  %219 = load i32, ptr %218, align 1, !dbg !150
  %220 = lshr i32 %219, 16, !dbg !150
  %221 = or i32 %219, %220, !dbg !150
  %222 = lshr i32 %221, 8, !dbg !150
  %223 = or i32 %221, %222, !dbg !150
  %224 = trunc i32 %223 to i8, !dbg !150
  %225 = load i32, ptr %4, align 4, !dbg !150
  %226 = icmp sgt i32 %225, 100, !dbg !151
  %implicit.taken11 = zext i1 %226 to i8, !dbg !149
  call void @__implicit_conditional_callback(i8 %224, i32 12, i8 %implicit.taken11, ptr @implicit.file.12, i32 60, i32 23, ptr @implicit.condition.12, ptr @implicit.llvmcond.12, ptr @implicit.vars.12, ptr @implicit.true.12, ptr @implicit.false.12), !dbg !149
  br i1 %226, label %227, label %229, !dbg !149

227:                                              ; preds = %215, %203
  %228 = call i32 (ptr, ...) @printf(ptr noundef @.str.7), !dbg !152
  br label %229, !dbg !154

229:                                              ; preds = %227, %215
  %230 = ptrtoint ptr %3 to i64, !dbg !155
  %231 = xor i64 %230, 87960930222080, !dbg !155
  %232 = inttoptr i64 %231 to ptr, !dbg !155
  %233 = load i32, ptr %232, align 1, !dbg !155
  %234 = lshr i32 %233, 16, !dbg !155
  %235 = or i32 %233, %234, !dbg !155
  %236 = lshr i32 %235, 8, !dbg !155
  %237 = or i32 %235, %236, !dbg !155
  %238 = trunc i32 %237 to i8, !dbg !155
  %239 = load i32, ptr %3, align 4, !dbg !155
  %240 = ptrtoint ptr %4 to i64, !dbg !157
  %241 = xor i64 %240, 87960930222080, !dbg !157
  %242 = inttoptr i64 %241 to ptr, !dbg !157
  %243 = load i32, ptr %242, align 1, !dbg !157
  %244 = lshr i32 %243, 16, !dbg !157
  %245 = or i32 %243, %244, !dbg !157
  %246 = lshr i32 %245, 8, !dbg !157
  %247 = or i32 %245, %246, !dbg !157
  %248 = trunc i32 %247 to i8, !dbg !157
  %249 = load i32, ptr %4, align 4, !dbg !157
  %250 = or i8 %238, %248, !dbg !158
  %251 = icmp sgt i32 %239, %249, !dbg !158
  %implicit.taken12 = zext i1 %251 to i8, !dbg !158
  call void @__implicit_conditional_callback(i8 %250, i32 13, i8 %implicit.taken12, ptr @implicit.file.13, i32 64, i32 17, ptr @implicit.condition.13, ptr @implicit.llvmcond.13, ptr @implicit.vars.13, ptr @implicit.true.13, ptr @implicit.false.13), !dbg !158
  br i1 %251, label %252, label %254, !dbg !158

252:                                              ; preds = %229
  %253 = call i32 (ptr, ...) @printf(ptr noundef @.str.8), !dbg !159
  br label %254, !dbg !161

254:                                              ; preds = %252, %229
  ret i32 0, !dbg !162
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

; Function Attrs: noinline nounwind optnone uwtable
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

declare void @__implicit_conditional_callback(i8, i32, i8, ptr, i32, i32, ptr, ptr, ptr, ptr, ptr)

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.dbg.cu = !{!29}
!llvm.module.flags = !{!31, !32, !33, !34, !35, !36, !37, !38}
!llvm.ident = !{!39}

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
!38 = !{i32 4, !"nosanitize_dataflow", i32 1}
!39 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!40 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 4, type: !41, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !44)
!41 = !DISubroutineType(types: !42)
!42 = !{!43}
!43 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!44 = !{}
!45 = !DILocalVariable(name: "secret1", scope: !40, file: !2, line: 6, type: !43)
!46 = !DILocation(line: 6, column: 9, scope: !40)
!47 = !DILocalVariable(name: "secret2", scope: !40, file: !2, line: 7, type: !43)
!48 = !DILocation(line: 7, column: 9, scope: !40)
!49 = !DILocalVariable(name: "secret3", scope: !40, file: !2, line: 8, type: !43)
!50 = !DILocation(line: 8, column: 9, scope: !40)
!51 = !DILocation(line: 10, column: 5, scope: !40)
!52 = !DILocation(line: 11, column: 5, scope: !40)
!53 = !DILocalVariable(name: "x", scope: !40, file: !2, line: 14, type: !43)
!54 = !DILocation(line: 14, column: 9, scope: !40)
!55 = !DILocation(line: 14, column: 13, scope: !40)
!56 = !DILocation(line: 16, column: 9, scope: !57)
!57 = distinct !DILexicalBlock(scope: !40, file: !2, line: 16, column: 9)
!58 = !DILocation(line: 16, column: 11, scope: !57)
!59 = !DILocation(line: 17, column: 9, scope: !60)
!60 = distinct !DILexicalBlock(scope: !57, file: !2, line: 16, column: 17)
!61 = !DILocation(line: 18, column: 5, scope: !60)
!62 = !DILocalVariable(name: "y", scope: !40, file: !2, line: 20, type: !43)
!63 = !DILocation(line: 20, column: 9, scope: !40)
!64 = !DILocation(line: 20, column: 13, scope: !40)
!65 = !DILocalVariable(name: "z", scope: !40, file: !2, line: 21, type: !43)
!66 = !DILocation(line: 21, column: 9, scope: !40)
!67 = !DILocation(line: 21, column: 13, scope: !40)
!68 = !DILocation(line: 23, column: 9, scope: !69)
!69 = distinct !DILexicalBlock(scope: !40, file: !2, line: 23, column: 9)
!70 = !DILocation(line: 23, column: 13, scope: !69)
!71 = !DILocation(line: 23, column: 11, scope: !69)
!72 = !DILocation(line: 23, column: 15, scope: !69)
!73 = !DILocation(line: 24, column: 9, scope: !74)
!74 = distinct !DILexicalBlock(scope: !69, file: !2, line: 23, column: 21)
!75 = !DILocation(line: 25, column: 5, scope: !74)
!76 = !DILocation(line: 27, column: 9, scope: !77)
!77 = distinct !DILexicalBlock(scope: !40, file: !2, line: 27, column: 9)
!78 = !DILocation(line: 27, column: 11, scope: !77)
!79 = !DILocation(line: 28, column: 9, scope: !80)
!80 = distinct !DILexicalBlock(scope: !77, file: !2, line: 27, column: 17)
!81 = !DILocation(line: 29, column: 5, scope: !80)
!82 = !DILocalVariable(name: "a", scope: !40, file: !2, line: 31, type: !43)
!83 = !DILocation(line: 31, column: 9, scope: !40)
!84 = !DILocation(line: 31, column: 13, scope: !40)
!85 = !DILocation(line: 31, column: 21, scope: !40)
!86 = !DILocalVariable(name: "b", scope: !40, file: !2, line: 32, type: !43)
!87 = !DILocation(line: 32, column: 9, scope: !40)
!88 = !DILocation(line: 32, column: 13, scope: !40)
!89 = !DILocation(line: 32, column: 21, scope: !40)
!90 = !DILocation(line: 34, column: 9, scope: !91)
!91 = distinct !DILexicalBlock(scope: !40, file: !2, line: 34, column: 9)
!92 = !DILocation(line: 34, column: 11, scope: !91)
!93 = !DILocation(line: 34, column: 16, scope: !91)
!94 = !DILocation(line: 34, column: 19, scope: !91)
!95 = !DILocation(line: 34, column: 21, scope: !91)
!96 = !DILocation(line: 35, column: 9, scope: !97)
!97 = distinct !DILexicalBlock(scope: !91, file: !2, line: 34, column: 27)
!98 = !DILocation(line: 36, column: 5, scope: !97)
!99 = !DILocalVariable(name: "c", scope: !40, file: !2, line: 38, type: !43)
!100 = !DILocation(line: 38, column: 9, scope: !40)
!101 = !DILocation(line: 38, column: 13, scope: !40)
!102 = !DILocation(line: 38, column: 23, scope: !40)
!103 = !DILocation(line: 38, column: 21, scope: !40)
!104 = !DILocation(line: 40, column: 10, scope: !105)
!105 = distinct !DILexicalBlock(scope: !40, file: !2, line: 40, column: 9)
!106 = !DILocation(line: 40, column: 12, scope: !105)
!107 = !DILocation(line: 40, column: 17, scope: !105)
!108 = !DILocation(line: 41, column: 9, scope: !109)
!109 = distinct !DILexicalBlock(scope: !105, file: !2, line: 40, column: 23)
!110 = !DILocation(line: 42, column: 5, scope: !109)
!111 = !DILocalVariable(name: "p", scope: !40, file: !2, line: 44, type: !43)
!112 = !DILocation(line: 44, column: 9, scope: !40)
!113 = !DILocation(line: 44, column: 13, scope: !40)
!114 = !DILocalVariable(name: "q", scope: !40, file: !2, line: 45, type: !43)
!115 = !DILocation(line: 45, column: 9, scope: !40)
!116 = !DILocation(line: 45, column: 13, scope: !40)
!117 = !DILocalVariable(name: "r", scope: !40, file: !2, line: 46, type: !43)
!118 = !DILocation(line: 46, column: 9, scope: !40)
!119 = !DILocation(line: 46, column: 13, scope: !40)
!120 = !DILocation(line: 48, column: 9, scope: !121)
!121 = distinct !DILexicalBlock(scope: !40, file: !2, line: 48, column: 9)
!122 = !DILocation(line: 48, column: 13, scope: !121)
!123 = !DILocation(line: 48, column: 11, scope: !121)
!124 = !DILocation(line: 48, column: 17, scope: !121)
!125 = !DILocation(line: 48, column: 15, scope: !121)
!126 = !DILocation(line: 48, column: 19, scope: !121)
!127 = !DILocation(line: 49, column: 9, scope: !128)
!128 = distinct !DILexicalBlock(scope: !121, file: !2, line: 48, column: 25)
!129 = !DILocation(line: 50, column: 5, scope: !128)
!130 = !DILocation(line: 52, column: 9, scope: !131)
!131 = distinct !DILexicalBlock(scope: !40, file: !2, line: 52, column: 9)
!132 = !DILocation(line: 52, column: 17, scope: !131)
!133 = !DILocation(line: 53, column: 13, scope: !134)
!134 = distinct !DILexicalBlock(scope: !135, file: !2, line: 53, column: 13)
!135 = distinct !DILexicalBlock(scope: !131, file: !2, line: 52, column: 22)
!136 = !DILocation(line: 53, column: 21, scope: !134)
!137 = !DILocation(line: 54, column: 17, scope: !138)
!138 = distinct !DILexicalBlock(scope: !139, file: !2, line: 54, column: 17)
!139 = distinct !DILexicalBlock(scope: !134, file: !2, line: 53, column: 26)
!140 = !DILocation(line: 54, column: 25, scope: !138)
!141 = !DILocation(line: 55, column: 17, scope: !142)
!142 = distinct !DILexicalBlock(scope: !138, file: !2, line: 54, column: 31)
!143 = !DILocation(line: 56, column: 13, scope: !142)
!144 = !DILocation(line: 57, column: 9, scope: !139)
!145 = !DILocation(line: 58, column: 5, scope: !135)
!146 = !DILocation(line: 60, column: 9, scope: !147)
!147 = distinct !DILexicalBlock(scope: !40, file: !2, line: 60, column: 9)
!148 = !DILocation(line: 60, column: 17, scope: !147)
!149 = !DILocation(line: 60, column: 23, scope: !147)
!150 = !DILocation(line: 60, column: 26, scope: !147)
!151 = !DILocation(line: 60, column: 34, scope: !147)
!152 = !DILocation(line: 61, column: 9, scope: !153)
!153 = distinct !DILexicalBlock(scope: !147, file: !2, line: 60, column: 41)
!154 = !DILocation(line: 62, column: 5, scope: !153)
!155 = !DILocation(line: 64, column: 9, scope: !156)
!156 = distinct !DILexicalBlock(scope: !40, file: !2, line: 64, column: 9)
!157 = !DILocation(line: 64, column: 19, scope: !156)
!158 = !DILocation(line: 64, column: 17, scope: !156)
!159 = !DILocation(line: 65, column: 9, scope: !160)
!160 = distinct !DILexicalBlock(scope: !156, file: !2, line: 64, column: 28)
!161 = !DILocation(line: 66, column: 5, scope: !160)
!162 = !DILocation(line: 68, column: 5, scope: !40)
