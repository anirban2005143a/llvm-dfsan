; ModuleID = 'scanf.c'
source_filename = "scanf.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@__dfsan_arg_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_retval_tls = external thread_local(initialexec) global [100 x i64]
@__dfsan_arg_origin_tls = external thread_local(initialexec) global [200 x i32]
@__dfsan_retval_origin_tls = external thread_local(initialexec) global i32
@__dfsan_track_origins = weak_odr constant i32 0
@0 = private unnamed_addr constant [15 x i8] c"__isoc99_scanf\00", align 1
@1 = private unnamed_addr constant [15 x i8] c"__isoc99_scanf\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i8, align 1
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i8 0, ptr %1, align 1
  store i32 0, ptr %2, align 4
  call void @__dfsan_unimplemented(ptr @1)
  %4 = call i32 (ptr, ...) @__isoc99_scanf(ptr noundef @.str, ptr noundef %3)
  %5 = ptrtoint ptr %3 to i64
  %6 = xor i64 %5, 87960930222080
  %7 = inttoptr i64 %6 to ptr
  %8 = load i32, ptr %7, align 1
  %9 = lshr i32 %8, 16
  %10 = or i32 %8, %9
  %11 = lshr i32 %10, 8
  %12 = or i32 %10, %11
  %13 = trunc i32 %12 to i8
  %14 = load i32, ptr %3, align 4
  ret i32 %14
}

declare i32 @__isoc99_scanf(ptr noundef, ...) #1

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
  %1 = call i32 @main()
  store i8 0, ptr @__dfsan_retval_tls, align 2
  ret i32 %1
}

define linkonce_odr i32 @"dfsw$__isoc99_scanf"(ptr noundef %0, ...) #1 {
  call void @__dfsan_vararg_wrapper(ptr @0)
  unreachable
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind memory(read) }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{i32 4, !"nosanitize_dataflow", i32 1}
!6 = !{!"Ubuntu clang version 21.1.8 (6ubuntu1)"}
