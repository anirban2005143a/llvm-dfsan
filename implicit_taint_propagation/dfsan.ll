; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [8 x i8] c"secret1\00", align 1, !dbg !0
@.str.1 = private unnamed_addr constant [8 x i8] c"secret2\00", align 1, !dbg !7
@.str.2 = private unnamed_addr constant [2 x i8] c"x\00", align 1, !dbg !9
@.str.3 = private unnamed_addr constant [2 x i8] c"y\00", align 1, !dbg !14
@.str.4 = private unnamed_addr constant [2 x i8] c"z\00", align 1, !dbg !16
@.str.5 = private unnamed_addr constant [2 x i8] c"a\00", align 1, !dbg !18
@.str.6 = private unnamed_addr constant [2 x i8] c"b\00", align 1, !dbg !20
@.str.7 = private unnamed_addr constant [2 x i8] c"c\00", align 1, !dbg !22
@.str.8 = private unnamed_addr constant [5 x i8] c"same\00", align 1, !dbg !24
@.str.9 = private unnamed_addr constant [9 x i8] c"%s = %u\0A\00", align 1, !dbg !29
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [7 x i8] c"printf\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 !dbg !47 {
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
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
    #dbg_declare(ptr %3, !52, !DIExpression(), !53)
  %13 = ptrtoint ptr %3 to i64, !dbg !53
  %14 = xor i64 %13, 87960930222080, !dbg !53
  %15 = inttoptr i64 %14 to ptr, !dbg !53
  store i32 0, ptr %15, align 1, !dbg !53
  store i32 1, ptr %3, align 4, !dbg !53
    #dbg_declare(ptr %4, !54, !DIExpression(), !55)
  %16 = ptrtoint ptr %4 to i64, !dbg !55
  %17 = xor i64 %16, 87960930222080, !dbg !55
  %18 = inttoptr i64 %17 to ptr, !dbg !55
  store i32 0, ptr %18, align 1, !dbg !55
  store i32 0, ptr %4, align 4, !dbg !55
    #dbg_declare(ptr %5, !56, !DIExpression(), !57)
  %19 = ptrtoint ptr %5 to i64, !dbg !57
  %20 = xor i64 %19, 87960930222080, !dbg !57
  %21 = inttoptr i64 %20 to ptr, !dbg !57
  store i32 0, ptr %21, align 1, !dbg !57
  store i32 0, ptr %5, align 4, !dbg !57
  call void @dfsan_set_label(i8 noundef zeroext 1, ptr noundef %3, i64 noundef 4), !dbg !58
  call void @dfsan_set_label(i8 noundef zeroext 2, ptr noundef %4, i64 noundef 4), !dbg !59
  call void @dfsan_set_label(i8 noundef zeroext 4, ptr noundef %5, i64 noundef 4), !dbg !60
    #dbg_declare(ptr %6, !61, !DIExpression(), !62)
  %22 = ptrtoint ptr %6 to i64, !dbg !62
  %23 = xor i64 %22, 87960930222080, !dbg !62
  %24 = inttoptr i64 %23 to ptr, !dbg !62
  store i32 0, ptr %24, align 1, !dbg !62
  store i32 0, ptr %6, align 4, !dbg !62
    #dbg_declare(ptr %7, !63, !DIExpression(), !64)
  %25 = ptrtoint ptr %7 to i64, !dbg !64
  %26 = xor i64 %25, 87960930222080, !dbg !64
  %27 = inttoptr i64 %26 to ptr, !dbg !64
  store i32 0, ptr %27, align 1, !dbg !64
  store i32 0, ptr %7, align 4, !dbg !64
    #dbg_declare(ptr %8, !65, !DIExpression(), !66)
  %28 = ptrtoint ptr %8 to i64, !dbg !66
  %29 = xor i64 %28, 87960930222080, !dbg !66
  %30 = inttoptr i64 %29 to ptr, !dbg !66
  store i32 0, ptr %30, align 1, !dbg !66
  store i32 0, ptr %8, align 4, !dbg !66
    #dbg_declare(ptr %9, !67, !DIExpression(), !68)
  %31 = ptrtoint ptr %9 to i64, !dbg !68
  %32 = xor i64 %31, 87960930222080, !dbg !68
  %33 = inttoptr i64 %32 to ptr, !dbg !68
  store i32 0, ptr %33, align 1, !dbg !68
  store i32 0, ptr %9, align 4, !dbg !68
    #dbg_declare(ptr %10, !69, !DIExpression(), !70)
  %34 = ptrtoint ptr %10 to i64, !dbg !70
  %35 = xor i64 %34, 87960930222080, !dbg !70
  %36 = inttoptr i64 %35 to ptr, !dbg !70
  store i32 0, ptr %36, align 1, !dbg !70
  store i32 0, ptr %10, align 4, !dbg !70
    #dbg_declare(ptr %11, !71, !DIExpression(), !72)
  %37 = ptrtoint ptr %11 to i64, !dbg !72
  %38 = xor i64 %37, 87960930222080, !dbg !72
  %39 = inttoptr i64 %38 to ptr, !dbg !72
  store i32 0, ptr %39, align 1, !dbg !72
  store i32 0, ptr %11, align 4, !dbg !72
    #dbg_declare(ptr %12, !73, !DIExpression(), !74)
  %40 = ptrtoint ptr %12 to i64, !dbg !74
  %41 = xor i64 %40, 87960930222080, !dbg !74
  %42 = inttoptr i64 %41 to ptr, !dbg !74
  store i32 0, ptr %42, align 1, !dbg !74
  store i32 0, ptr %12, align 4, !dbg !74
  %43 = ptrtoint ptr %3 to i64, !dbg !75
  %44 = xor i64 %43, 87960930222080, !dbg !75
  %45 = inttoptr i64 %44 to ptr, !dbg !75
  %46 = load i32, ptr %45, align 1, !dbg !75
  %47 = lshr i32 %46, 16, !dbg !75
  %48 = or i32 %46, %47, !dbg !75
  %49 = lshr i32 %48, 8, !dbg !75
  %50 = or i32 %48, %49, !dbg !75
  %51 = trunc i32 %50 to i8, !dbg !75
  %52 = load i32, ptr %3, align 4, !dbg !75
  %53 = icmp ne i32 %52, 0, !dbg !75
  call void @__dfsan_conditional_callback(i8 zeroext %51), !dbg !75
  br i1 %53, label %54, label %58, !dbg !75

54:                                               ; preds = %0
  %55 = ptrtoint ptr %6 to i64, !dbg !77
  %56 = xor i64 %55, 87960930222080, !dbg !77
  %57 = inttoptr i64 %56 to ptr, !dbg !77
  store i32 0, ptr %57, align 1, !dbg !77
  store i32 10, ptr %6, align 4, !dbg !77
  br label %96, !dbg !79

58:                                               ; preds = %0
  %59 = ptrtoint ptr %4 to i64, !dbg !80
  %60 = xor i64 %59, 87960930222080, !dbg !80
  %61 = inttoptr i64 %60 to ptr, !dbg !80
  %62 = load i32, ptr %61, align 1, !dbg !80
  %63 = lshr i32 %62, 16, !dbg !80
  %64 = or i32 %62, %63, !dbg !80
  %65 = lshr i32 %64, 8, !dbg !80
  %66 = or i32 %64, %65, !dbg !80
  %67 = trunc i32 %66 to i8, !dbg !80
  %68 = load i32, ptr %4, align 4, !dbg !80
  %69 = icmp ne i32 %68, 0, !dbg !80
  call void @__dfsan_conditional_callback(i8 zeroext %67), !dbg !80
  br i1 %69, label %70, label %74, !dbg !80

70:                                               ; preds = %58
  %71 = ptrtoint ptr %7 to i64, !dbg !82
  %72 = xor i64 %71, 87960930222080, !dbg !82
  %73 = inttoptr i64 %72 to ptr, !dbg !82
  store i32 0, ptr %73, align 1, !dbg !82
  store i32 40, ptr %7, align 4, !dbg !82
  br label %95, !dbg !84

74:                                               ; preds = %58
  %75 = ptrtoint ptr %5 to i64, !dbg !85
  %76 = xor i64 %75, 87960930222080, !dbg !85
  %77 = inttoptr i64 %76 to ptr, !dbg !85
  %78 = load i32, ptr %77, align 1, !dbg !85
  %79 = lshr i32 %78, 16, !dbg !85
  %80 = or i32 %78, %79, !dbg !85
  %81 = lshr i32 %80, 8, !dbg !85
  %82 = or i32 %80, %81, !dbg !85
  %83 = trunc i32 %82 to i8, !dbg !85
  %84 = load i32, ptr %5, align 4, !dbg !85
  %85 = icmp ne i32 %84, 0, !dbg !85
  call void @__dfsan_conditional_callback(i8 zeroext %83), !dbg !85
  br i1 %85, label %86, label %90, !dbg !85

86:                                               ; preds = %74
  %87 = ptrtoint ptr %8 to i64, !dbg !87
  %88 = xor i64 %87, 87960930222080, !dbg !87
  %89 = inttoptr i64 %88 to ptr, !dbg !87
  store i32 0, ptr %89, align 1, !dbg !87
  store i32 40, ptr %8, align 4, !dbg !87
  br label %94, !dbg !89

90:                                               ; preds = %74
  %91 = ptrtoint ptr %8 to i64, !dbg !90
  %92 = xor i64 %91, 87960930222080, !dbg !90
  %93 = inttoptr i64 %92 to ptr, !dbg !90
  store i32 0, ptr %93, align 1, !dbg !90
  store i32 50, ptr %8, align 4, !dbg !90
  br label %94

94:                                               ; preds = %90, %86
  br label %95

95:                                               ; preds = %94, %70
  br label %96

96:                                               ; preds = %95, %54
  %97 = ptrtoint ptr %3 to i64, !dbg !92
  %98 = xor i64 %97, 87960930222080, !dbg !92
  %99 = inttoptr i64 %98 to ptr, !dbg !92
  %100 = load i32, ptr %99, align 1, !dbg !92
  %101 = lshr i32 %100, 16, !dbg !92
  %102 = or i32 %100, %101, !dbg !92
  %103 = lshr i32 %102, 8, !dbg !92
  %104 = or i32 %102, %103, !dbg !92
  %105 = trunc i32 %104 to i8, !dbg !92
  %106 = load i32, ptr %3, align 4, !dbg !92
  %107 = icmp ne i32 %106, 0, !dbg !92
  call void @__dfsan_conditional_callback(i8 zeroext %105), !dbg !92
  br i1 %107, label %108, label %112, !dbg !92

108:                                              ; preds = %96
  %109 = ptrtoint ptr %12 to i64, !dbg !94
  %110 = xor i64 %109, 87960930222080, !dbg !94
  %111 = inttoptr i64 %110 to ptr, !dbg !94
  store i32 0, ptr %111, align 1, !dbg !94
  store i32 7, ptr %12, align 4, !dbg !94
  br label %116, !dbg !96

112:                                              ; preds = %96
  %113 = ptrtoint ptr %12 to i64, !dbg !97
  %114 = xor i64 %113, 87960930222080, !dbg !97
  %115 = inttoptr i64 %114 to ptr, !dbg !97
  store i32 0, ptr %115, align 1, !dbg !97
  store i32 7, ptr %12, align 4, !dbg !97
  br label %116

116:                                              ; preds = %112, %108
  %117 = ptrtoint ptr %6 to i64, !dbg !99
  %118 = xor i64 %117, 87960930222080, !dbg !99
  %119 = inttoptr i64 %118 to ptr, !dbg !99
  %120 = load i32, ptr %119, align 1, !dbg !99
  %121 = lshr i32 %120, 16, !dbg !99
  %122 = or i32 %120, %121, !dbg !99
  %123 = lshr i32 %122, 8, !dbg !99
  %124 = or i32 %122, %123, !dbg !99
  %125 = trunc i32 %124 to i8, !dbg !99
  %126 = load i32, ptr %6, align 4, !dbg !99
  %127 = ptrtoint ptr %9 to i64, !dbg !100
  %128 = xor i64 %127, 87960930222080, !dbg !100
  %129 = inttoptr i64 %128 to ptr, !dbg !100
  %130 = getelementptr i8, ptr %129, i32 0, !dbg !100
  store i8 %125, ptr %130, align 1, !dbg !100
  %131 = getelementptr i8, ptr %129, i32 1, !dbg !100
  store i8 %125, ptr %131, align 1, !dbg !100
  %132 = getelementptr i8, ptr %129, i32 2, !dbg !100
  store i8 %125, ptr %132, align 1, !dbg !100
  %133 = getelementptr i8, ptr %129, i32 3, !dbg !100
  store i8 %125, ptr %133, align 1, !dbg !100
  store i32 %126, ptr %9, align 4, !dbg !100
  %134 = ptrtoint ptr %7 to i64, !dbg !101
  %135 = xor i64 %134, 87960930222080, !dbg !101
  %136 = inttoptr i64 %135 to ptr, !dbg !101
  %137 = load i32, ptr %136, align 1, !dbg !101
  %138 = lshr i32 %137, 16, !dbg !101
  %139 = or i32 %137, %138, !dbg !101
  %140 = lshr i32 %139, 8, !dbg !101
  %141 = or i32 %139, %140, !dbg !101
  %142 = trunc i32 %141 to i8, !dbg !101
  %143 = load i32, ptr %7, align 4, !dbg !101
  %144 = ptrtoint ptr %10 to i64, !dbg !102
  %145 = xor i64 %144, 87960930222080, !dbg !102
  %146 = inttoptr i64 %145 to ptr, !dbg !102
  %147 = getelementptr i8, ptr %146, i32 0, !dbg !102
  store i8 %142, ptr %147, align 1, !dbg !102
  %148 = getelementptr i8, ptr %146, i32 1, !dbg !102
  store i8 %142, ptr %148, align 1, !dbg !102
  %149 = getelementptr i8, ptr %146, i32 2, !dbg !102
  store i8 %142, ptr %149, align 1, !dbg !102
  %150 = getelementptr i8, ptr %146, i32 3, !dbg !102
  store i8 %142, ptr %150, align 1, !dbg !102
  store i32 %143, ptr %10, align 4, !dbg !102
  %151 = ptrtoint ptr %8 to i64, !dbg !103
  %152 = xor i64 %151, 87960930222080, !dbg !103
  %153 = inttoptr i64 %152 to ptr, !dbg !103
  %154 = load i32, ptr %153, align 1, !dbg !103
  %155 = lshr i32 %154, 16, !dbg !103
  %156 = or i32 %154, %155, !dbg !103
  %157 = lshr i32 %156, 8, !dbg !103
  %158 = or i32 %156, %157, !dbg !103
  %159 = trunc i32 %158 to i8, !dbg !103
  %160 = load i32, ptr %8, align 4, !dbg !103
  %161 = ptrtoint ptr %11 to i64, !dbg !104
  %162 = xor i64 %161, 87960930222080, !dbg !104
  %163 = inttoptr i64 %162 to ptr, !dbg !104
  %164 = getelementptr i8, ptr %163, i32 0, !dbg !104
  store i8 %159, ptr %164, align 1, !dbg !104
  %165 = getelementptr i8, ptr %163, i32 1, !dbg !104
  store i8 %159, ptr %165, align 1, !dbg !104
  %166 = getelementptr i8, ptr %163, i32 2, !dbg !104
  store i8 %159, ptr %166, align 1, !dbg !104
  %167 = getelementptr i8, ptr %163, i32 3, !dbg !104
  store i8 %159, ptr %167, align 1, !dbg !104
  store i32 %160, ptr %11, align 4, !dbg !104
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !105
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !105
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !105
  call void @print_label.dfsan(ptr noundef @.str, ptr noundef %3, i64 noundef 4), !dbg !105
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !106
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !106
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !106
  call void @print_label.dfsan(ptr noundef @.str.1, ptr noundef %4, i64 noundef 4), !dbg !106
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !107
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !107
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !107
  call void @print_label.dfsan(ptr noundef @.str.2, ptr noundef %6, i64 noundef 4), !dbg !107
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !108
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !108
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !108
  call void @print_label.dfsan(ptr noundef @.str.3, ptr noundef %7, i64 noundef 4), !dbg !108
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !109
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !109
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !109
  call void @print_label.dfsan(ptr noundef @.str.4, ptr noundef %8, i64 noundef 4), !dbg !109
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !110
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !110
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !110
  call void @print_label.dfsan(ptr noundef @.str.5, ptr noundef %9, i64 noundef 4), !dbg !110
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !111
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !111
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !111
  call void @print_label.dfsan(ptr noundef @.str.6, ptr noundef %10, i64 noundef 4), !dbg !111
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !112
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !112
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !112
  call void @print_label.dfsan(ptr noundef @.str.7, ptr noundef %11, i64 noundef 4), !dbg !112
  store i8 0, ptr @__dfsan_arg_tls, align 2, !dbg !113
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2, !dbg !113
  store i8 0, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2, !dbg !113
  call void @print_label.dfsan(ptr noundef @.str.8, ptr noundef %12, i64 noundef 4), !dbg !113
  ret i32 0, !dbg !114
}

declare void @dfsan_set_label(i8 noundef zeroext, ptr noundef, i64 noundef) #1

; Function Attrs: noinline nounwind uwtable
define internal void @print_label.dfsan(ptr noundef %0, ptr noundef %1, i64 noundef %2) #0 !dbg !115 {
  %4 = load i8, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 4) to ptr), align 2
  %5 = load i8, ptr inttoptr (i64 add (i64 ptrtoint (ptr @__dfsan_arg_tls to i64), i64 2) to ptr), align 2
  %6 = load i8, ptr @__dfsan_arg_tls, align 2
  %7 = alloca i8, align 1
  %8 = alloca ptr, align 8
  %9 = alloca i8, align 1
  %10 = alloca ptr, align 8
  %11 = alloca i8, align 1
  %12 = alloca i64, align 8
  store i8 %6, ptr %7, align 1
  store ptr %0, ptr %8, align 8
    #dbg_declare(ptr %8, !125, !DIExpression(), !126)
  store i8 %5, ptr %9, align 1
  store ptr %1, ptr %10, align 8
    #dbg_declare(ptr %10, !127, !DIExpression(), !128)
  store i8 %4, ptr %11, align 1
  store i64 %2, ptr %12, align 8
    #dbg_declare(ptr %12, !129, !DIExpression(), !130)
  %13 = load i8, ptr %7, align 1, !dbg !131
  %14 = load ptr, ptr %8, align 8, !dbg !131
  %15 = load i8, ptr %9, align 1, !dbg !132
  %16 = load ptr, ptr %10, align 8, !dbg !132
  %17 = load i8, ptr %11, align 1, !dbg !133
  %18 = load i64, ptr %12, align 8, !dbg !133
  %19 = call zeroext i8 @dfsan_read_label(ptr noundef %16, i64 noundef %18), !dbg !134
  %20 = zext i8 %19 to i32, !dbg !135
  %21 = call i32 (ptr, ...) @printf(ptr noundef @.str.9, ptr noundef %14, i32 noundef %20), !dbg !136
  ret void, !dbg !137
}

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

!llvm.dbg.cu = !{!34}
!llvm.module.flags = !{!38, !39, !40, !41, !42, !43, !44, !45}
!llvm.ident = !{!46}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(scope: null, file: !2, line: 67, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "test.c", directory: "/home/anirban2005/dfsan/implicit_taint_propagation", checksumkind: CSK_MD5, checksum: "6bc3e7d8b1c0638cf143dcb030306ce8")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 64, elements: !5)
!4 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!5 = !{!6}
!6 = !DISubrange(count: 8)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(scope: null, file: !2, line: 72, type: !3, isLocal: true, isDefinition: true)
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(scope: null, file: !2, line: 77, type: !11, isLocal: true, isDefinition: true)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 16, elements: !12)
!12 = !{!13}
!13 = !DISubrange(count: 2)
!14 = !DIGlobalVariableExpression(var: !15, expr: !DIExpression())
!15 = distinct !DIGlobalVariable(scope: null, file: !2, line: 82, type: !11, isLocal: true, isDefinition: true)
!16 = !DIGlobalVariableExpression(var: !17, expr: !DIExpression())
!17 = distinct !DIGlobalVariable(scope: null, file: !2, line: 87, type: !11, isLocal: true, isDefinition: true)
!18 = !DIGlobalVariableExpression(var: !19, expr: !DIExpression())
!19 = distinct !DIGlobalVariable(scope: null, file: !2, line: 92, type: !11, isLocal: true, isDefinition: true)
!20 = !DIGlobalVariableExpression(var: !21, expr: !DIExpression())
!21 = distinct !DIGlobalVariable(scope: null, file: !2, line: 97, type: !11, isLocal: true, isDefinition: true)
!22 = !DIGlobalVariableExpression(var: !23, expr: !DIExpression())
!23 = distinct !DIGlobalVariable(scope: null, file: !2, line: 102, type: !11, isLocal: true, isDefinition: true)
!24 = !DIGlobalVariableExpression(var: !25, expr: !DIExpression())
!25 = distinct !DIGlobalVariable(scope: null, file: !2, line: 107, type: !26, isLocal: true, isDefinition: true)
!26 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 40, elements: !27)
!27 = !{!28}
!28 = !DISubrange(count: 5)
!29 = !DIGlobalVariableExpression(var: !30, expr: !DIExpression())
!30 = distinct !DIGlobalVariable(scope: null, file: !2, line: 11, type: !31, isLocal: true, isDefinition: true)
!31 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 72, elements: !32)
!32 = !{!33}
!33 = !DISubrange(count: 9)
!34 = distinct !DICompileUnit(language: DW_LANG_C11, file: !2, producer: "Ubuntu clang version 21.1.8 (6ubuntu1)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !35, globals: !37, splitDebugInlining: false, nameTableKind: None)
!35 = !{!36}
!36 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!37 = !{!0, !7, !9, !14, !16, !18, !20, !22, !24, !29}
!38 = !{i32 7, !"Dwarf Version", i32 5}
!39 = !{i32 2, !"Debug Info Version", i32 3}
!40 = !{i32 1, !"wchar_size", i32 4}
!41 = !{i32 8, !"PIC Level", i32 2}
!42 = !{i32 7, !"PIE Level", i32 2}
!43 = !{i32 7, !"uwtable", i32 2}
!44 = !{i32 7, !"frame-pointer", i32 2}
!45 = !{i32 4, !"nosanitize_dataflow", i32 1}
!46 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
!47 = distinct !DISubprogram(name: "main", scope: !2, file: !2, line: 16, type: !48, scopeLine: 17, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !34, retainedNodes: !51)
!48 = !DISubroutineType(types: !49)
!49 = !{!50}
!50 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!51 = !{}
!52 = !DILocalVariable(name: "secret1", scope: !47, file: !2, line: 18, type: !50)
!53 = !DILocation(line: 18, column: 9, scope: !47)
!54 = !DILocalVariable(name: "secret2", scope: !47, file: !2, line: 19, type: !50)
!55 = !DILocation(line: 19, column: 9, scope: !47)
!56 = !DILocalVariable(name: "secret3", scope: !47, file: !2, line: 20, type: !50)
!57 = !DILocation(line: 20, column: 9, scope: !47)
!58 = !DILocation(line: 22, column: 5, scope: !47)
!59 = !DILocation(line: 27, column: 5, scope: !47)
!60 = !DILocation(line: 31, column: 5, scope: !47)
!61 = !DILocalVariable(name: "x", scope: !47, file: !2, line: 36, type: !50)
!62 = !DILocation(line: 36, column: 9, scope: !47)
!63 = !DILocalVariable(name: "y", scope: !47, file: !2, line: 37, type: !50)
!64 = !DILocation(line: 37, column: 9, scope: !47)
!65 = !DILocalVariable(name: "z", scope: !47, file: !2, line: 38, type: !50)
!66 = !DILocation(line: 38, column: 9, scope: !47)
!67 = !DILocalVariable(name: "a", scope: !47, file: !2, line: 40, type: !50)
!68 = !DILocation(line: 40, column: 9, scope: !47)
!69 = !DILocalVariable(name: "b", scope: !47, file: !2, line: 41, type: !50)
!70 = !DILocation(line: 41, column: 9, scope: !47)
!71 = !DILocalVariable(name: "c", scope: !47, file: !2, line: 42, type: !50)
!72 = !DILocation(line: 42, column: 9, scope: !47)
!73 = !DILocalVariable(name: "same", scope: !47, file: !2, line: 44, type: !50)
!74 = !DILocation(line: 44, column: 9, scope: !47)
!75 = !DILocation(line: 46, column: 9, scope: !76)
!76 = distinct !DILexicalBlock(scope: !47, file: !2, line: 46, column: 9)
!77 = !DILocation(line: 47, column: 11, scope: !78)
!78 = distinct !DILexicalBlock(scope: !76, file: !2, line: 46, column: 18)
!79 = !DILocation(line: 48, column: 5, scope: !78)
!80 = !DILocation(line: 48, column: 16, scope: !81)
!81 = distinct !DILexicalBlock(scope: !76, file: !2, line: 48, column: 16)
!82 = !DILocation(line: 49, column: 11, scope: !83)
!83 = distinct !DILexicalBlock(scope: !81, file: !2, line: 48, column: 25)
!84 = !DILocation(line: 50, column: 5, scope: !83)
!85 = !DILocation(line: 50, column: 16, scope: !86)
!86 = distinct !DILexicalBlock(scope: !81, file: !2, line: 50, column: 16)
!87 = !DILocation(line: 51, column: 11, scope: !88)
!88 = distinct !DILexicalBlock(scope: !86, file: !2, line: 50, column: 25)
!89 = !DILocation(line: 52, column: 5, scope: !88)
!90 = !DILocation(line: 53, column: 11, scope: !91)
!91 = distinct !DILexicalBlock(scope: !86, file: !2, line: 52, column: 12)
!92 = !DILocation(line: 56, column: 9, scope: !93)
!93 = distinct !DILexicalBlock(scope: !47, file: !2, line: 56, column: 9)
!94 = !DILocation(line: 57, column: 14, scope: !95)
!95 = distinct !DILexicalBlock(scope: !93, file: !2, line: 56, column: 18)
!96 = !DILocation(line: 58, column: 5, scope: !95)
!97 = !DILocation(line: 59, column: 14, scope: !98)
!98 = distinct !DILexicalBlock(scope: !93, file: !2, line: 58, column: 12)
!99 = !DILocation(line: 62, column: 9, scope: !47)
!100 = !DILocation(line: 62, column: 7, scope: !47)
!101 = !DILocation(line: 63, column: 9, scope: !47)
!102 = !DILocation(line: 63, column: 7, scope: !47)
!103 = !DILocation(line: 64, column: 9, scope: !47)
!104 = !DILocation(line: 64, column: 7, scope: !47)
!105 = !DILocation(line: 66, column: 5, scope: !47)
!106 = !DILocation(line: 71, column: 5, scope: !47)
!107 = !DILocation(line: 76, column: 5, scope: !47)
!108 = !DILocation(line: 81, column: 5, scope: !47)
!109 = !DILocation(line: 86, column: 5, scope: !47)
!110 = !DILocation(line: 91, column: 5, scope: !47)
!111 = !DILocation(line: 96, column: 5, scope: !47)
!112 = !DILocation(line: 101, column: 5, scope: !47)
!113 = !DILocation(line: 106, column: 5, scope: !47)
!114 = !DILocation(line: 111, column: 5, scope: !47)
!115 = distinct !DISubprogram(name: "print_label", scope: !2, file: !2, line: 5, type: !116, scopeLine: 9, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !34, retainedNodes: !51)
!116 = !DISubroutineType(types: !117)
!117 = !{null, !118, !120, !122}
!118 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !119, size: 64)
!119 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !4)
!120 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !121, size: 64)
!121 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!122 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !123, line: 18, baseType: !124)
!123 = !DIFile(filename: "/usr/lib/llvm-21/lib/clang/21/include/__stddef_size_t.h", directory: "", checksumkind: CSK_MD5, checksum: "2c44e821a2b1951cde2eb0fb2e656867")
!124 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!125 = !DILocalVariable(name: "name", arg: 1, scope: !115, file: !2, line: 6, type: !118)
!126 = !DILocation(line: 6, column: 17, scope: !115)
!127 = !DILocalVariable(name: "p", arg: 2, scope: !115, file: !2, line: 7, type: !120)
!128 = !DILocation(line: 7, column: 17, scope: !115)
!129 = !DILocalVariable(name: "n", arg: 3, scope: !115, file: !2, line: 8, type: !122)
!130 = !DILocation(line: 8, column: 12, scope: !115)
!131 = !DILocation(line: 12, column: 9, scope: !115)
!132 = !DILocation(line: 13, column: 36, scope: !115)
!133 = !DILocation(line: 13, column: 39, scope: !115)
!134 = !DILocation(line: 13, column: 19, scope: !115)
!135 = !DILocation(line: 13, column: 9, scope: !115)
!136 = !DILocation(line: 10, column: 5, scope: !115)
!137 = !DILocation(line: 14, column: 1, scope: !115)
