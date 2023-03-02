; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @test() #0 {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %e = alloca i32, align 4
  %f = alloca i32, align 4
  store i32 0, i32* %a, align 4
  store i32 1, i32* %c, align 4
  br label %do.body

do.body:                                          ; preds = %do.cond, %entry
  %0 = load i32, i32* %a, align 4
  %add = add nsw i32 %0, 1
  store i32 %add, i32* %c, align 4
  %1 = load i32, i32* %c, align 4
  %2 = load i32, i32* %b, align 4
  %mul = mul nsw i32 %1, %2
  store i32 %mul, i32* %c, align 4
  %3 = load i32, i32* %b, align 4
  %cmp = icmp sgt i32 %3, 9
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %do.body
  %4 = load i32, i32* %d, align 4
  %5 = load i32, i32* %c, align 4
  %mul1 = mul nsw i32 %4, %5
  store i32 %mul1, i32* %f, align 4
  %6 = load i32, i32* %f, align 4
  %sub = sub nsw i32 %6, 3
  store i32 %sub, i32* %c, align 4
  br label %if.end

if.else:                                          ; preds = %do.body
  %7 = load i32, i32* %e, align 4
  %add2 = add nsw i32 %7, 1
  store i32 %add2, i32* %a, align 4
  %8 = load i32, i32* %d, align 4
  %div = sdiv i32 %8, 2
  store i32 %div, i32* %e, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %9 = load i32, i32* %b, align 4
  store i32 %9, i32* %a, align 4
  br label %do.cond

do.cond:                                          ; preds = %if.end
  %10 = load i32, i32* %a, align 4
  %cmp3 = icmp slt i32 %10, 9
  br i1 %cmp3, label %do.body, label %do.end, !llvm.loop !6

do.end:                                           ; preds = %do.cond
  %11 = load i32, i32* %a, align 4
  %add4 = add nsw i32 %11, 1
  store i32 %add4, i32* %a, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 noundef %argc, i8** noundef %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 8
  store i32 0, i32* %retval, align 4
  store i32 %argc, i32* %argc.addr, align 4
  store i8** %argv, i8*** %argv.addr, align 8
  call void @test()
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 14.0.0-1ubuntu1"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
