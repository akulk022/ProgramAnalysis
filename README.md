# ProgramAnalysis
Different types of program analysis code that I've tried implementing

Added Interprocedural analysis for getting information on available expressions using an LLVM pass.

Initial compile
cmake .

Subsequent compiles
make

Running the pass
opt -load ./libAvailExpression.so -AvailExpression testAvail.ll -enable-new-pm=0
