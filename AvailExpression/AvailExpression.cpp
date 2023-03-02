#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <set>
#include <queue>

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "AvailExpression"

namespace
{
struct AvailExpression : public FunctionPass
{
  static char ID;
  //Here we calculate local transfer functions if a basic block contains no calls
  unordered_map<string,string> localTransferFunctions;
  //store summary functions for a whole function
  unordered_map<string,string> summaryFunctions;
  //Here we store information on whether a basic block contains a Call
  unordered_map<string,bool> bbContainsCall;
  set<Function*> calledFunctions; 
  AvailExpression() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override
  { 
    //hardcoded expression
    string exp = "b+c";

    //Fetching all expressions from the code to have summary functions for all expressions
    set<string> allExpressionsSet;
    for (auto &basic_block: F)
    {
      for (Instruction &instruct: basic_block)
      {
        if (isa<BinaryOperator> (instruct))
        {
          allExpressionsSet.insert(getExpFromInstruct(&instruct));
        }
      }
    }
    for(auto &basic_block: F){
      string bbname = basic_block.getName().str();
      bool callPresent=false;
      for (Instruction &instruct : basic_block)
      {
        if(isa<CallInst>(instruct)){
          callPresent=true;
        }
      }
      bbContainsCall[bbname]=callPresent;
    }
    //errs() << "FUNCTION: " << F.getName().str() << "\n";
    for(auto exp:allExpressionsSet){
      //errs() << "Running For: " << F.getName().str() << "\t" << "Expression: " << exp << "\n";
      //runSummaryFunctionLogic(&F,exp);
      //errs() << "\n";
      //runSummaryFunctionLogic(&F,"e+c");
    }
    runSummaryFunctionLogic(&F,"b+c");
    //for(auto exp:allExpressionsSet){
    //  errs() << "Hi" << exp << "\n";
    //}
    return true;
  }
  void runSummaryFunctionLogic(Function *F,string exp){
    for(auto &basic_block: *F){
      string funcName = F->getName().str();
      string bbname = basic_block.getName().str();
      //calculate local transfer function for basic blocks that do not contain a call
      if(!bbContainsCall[bbname]){
        string localFunc = computeBBTransferFunc(&basic_block,exp);
        //
        //errs() << "Function name: " << F->getName().str() << "\n";  
        errs() << "Basic Block: " << bbname << "\n";
        errs() << "Local Transfer function: " << localFunc << "\n";
        localTransferFunctions[funcName+bbname] = localFunc;
      }else{//If the block contains a call we make it F Omega for initial case
        localTransferFunctions[funcName+bbname] = "fO";
        errs() << "Basic Block: " << bbname << "\n";
        errs() << "Local Transfer function: " << localTransferFunctions[funcName+bbname] << "\n";
        for (Instruction &instruct : basic_block)
        {
          if(isa<CallInst>(instruct)){
          Function *func = getCallInstruction(&instruct);
          calledFunctions.insert(func); 
          }
        }
      }
    }
    for(auto func:calledFunctions){
      string summaryFunc;
      summaryFunc = getSummaryTransferFunc(func);
      errs() << "Expression: " << exp << "\t" << "Summary Function: " << summaryFunc << "\n";
    }
  }
  string getSummaryTransferFunc(Function *function){
    string summaryFunc;
    string funcName = function->getName().str();
    string multPathComposedFunc[2];
    //errs() << "function name: " << funcName << "\n";
    string composedSummary;
    string composedFunc;
    for(auto &basic_block:*function){
      string bbname = basic_block.getName().str();
      //first basic block
      if (predecessors(&basic_block).empty()){
        composedSummary=localTransferFunctions[funcName+bbname];
      }
      string prevSuccName;
      string prevComposed;
      for(BasicBlock *successor:successors(&basic_block)){
        string succName = successor->getName().str();
        //errs()<<"successor Name: "<<succName<<"\n";
        //errs()<<"1. "<<composedSummary<<"\n";
        //errs()<<"2. "<<localTransferFunctions[funcName+succName]<<"\n";
        composedFunc = compositionOperator(composedSummary,localTransferFunctions[funcName+succName]);
        if(prevSuccName == succName){
          composedFunc = meetOperator(composedFunc,prevComposed);
        }
        prevSuccName = succName;
        prevComposed = composedFunc;
        //errs()<<composedFunc<<"\n";         
      }
      //errs() << "blah" <<composedFunc << "\n";
      composedSummary = composedFunc;
      //errs()<<composedSummary<<"\n";
      if(successors(&basic_block).empty()){
        summaryFunc = composedFunc;  
      }
    }
    return summaryFunc;
  }
  //IMPORTANT
  Function* getCallInstruction(Instruction *instruct)
  {
    Function *func;
    if (isa<CallInst> (*instruct)){
      CallInst *callInst = dyn_cast<CallInst> (instruct);
      Value *val = callInst->getCalledOperand();
      func = dyn_cast<Function>(val); 
      errs() << "Function name: " << func->getName().str() <<"\n";
      /*for(auto &basic_block:*func){
        for(BasicBlock *successor:successors(&basic_block)){

        }
      }*/
    }
    return func;
  }
  /*string composeTransferFuncs(){

  }*/
  //Composition Operator
  string compositionOperator(string func1, string func2){
    string composedResult="fO";
    if(func2=="fT")
      composedResult = "fT";
    else if(func2=="fB")
      composedResult = "fB";
    else if(func2=="id")
      composedResult=func1;
    else if(func2=="fO")
      composedResult=func2;  
    return composedResult;
  }
  //Meet Operator for available expressions
  string meetOperator(string func1, string func2){
    string meetResult = "fO";
    //If we don't know one path, we keep the meet as the other
    if(func1=="fO")
      meetResult = func2;
    else if(func2=="fO")
      meetResult = func1;
    //if any one path makes the expression unavailable it is unavailable
    else if(func1=="fB"||func2=="fB")
      meetResult = "fB";
    else if(func1=="fT"&&func2=="fT")
      meetResult = "fT";
    return meetResult;
  }
  //Method that takes a basic block and returns its local transfer funtion
  //id fT fB
  string computeBBTransferFunc(BasicBlock *basicBlock,string exp){
    string func;
    string bbname = basicBlock->getName().str();
    //errs() << "getting bb here: "<< bbname << "\n";
    if(expressionComputedinBB(basicBlock,exp)){
      func = "fT";//Setting to TOP i.e given expression gets generated in the block
    }else if(expGetsKilledinBB(basicBlock,exp)){
      func = "fB";//setting to bottom
    }else{
      func = "id";
    }
    return func;
  }
  //returns true if an expression gets killed in a basic block
  bool expGetsKilledinBB(BasicBlock *basicBlock,string exp){
    bool isKilled = false;
    string var1 = exp.substr(0,1);
    string var2 = exp.substr(2,1);
    //errs()  <<  "Variables: " << var1 << "\t " << var2 << "\n";
    for(Instruction &instruct:*basicBlock){
      if(isa<StoreInst>(instruct)){
        string var = getVarFromInstruct(&instruct);
        if(var==var1||var==var2)
          isKilled = true;
      }
      if(isa<BinaryOperator>(instruct)){
        string expression = getExpFromInstruct(&instruct);
        if(exp==expression){
          isKilled = false;
        }
      }
    }
    return isKilled;
  }
  //checks if the basic block computes the given expression
  bool expressionComputedinBB(BasicBlock *basicBlock,string exp){
    bool isComputed = false;
    string var1 = exp.substr(0,1);
    string var2 = exp.substr(2,1);
    for(Instruction &instruct:*basicBlock){
      if(isa<BinaryOperator>(instruct)){
        string expression = getExpFromInstruct(&instruct);
        //make it true if we encounter an expression that equals the one being considered.
        if(exp==expression){
          isComputed = true;
        }
      }
      if(isa<StoreInst>(instruct)){
        string var = getVarFromInstruct(&instruct);
        if(var==var1||var==var2)
          isComputed = false;
      }
    }
    return isComputed;
  }
  string getExpFromInstruct(Instruction *instruct){
    BinaryOperator *binaryOp = dyn_cast<BinaryOperator>(instruct);
    string var1 = returnNameFromVal(binaryOp->getOperand(0));
    string var2 = returnNameFromVal(binaryOp->getOperand(1));
    if (isa<LoadInst> (binaryOp->getOperand(0)))
    {
      LoadInst *loadInst1 = dyn_cast<LoadInst> (binaryOp->getOperand(0));
      Value *val1 = loadInst1->getPointerOperand();
      var1 = val1->getName().str();
    }
    if (isa<LoadInst> (binaryOp->getOperand(1)))
    {
      LoadInst *loadInst2 = dyn_cast<LoadInst> (binaryOp->getOperand(1));
      Value *val2 = loadInst2->getPointerOperand();
      var2 = val2->getName().str();
    }
    string operatorName = binaryOp->getOpcodeName();
    string op = getOpFromOpName(operatorName);
    string expression = var1 + op + var2;
    //errs() << "exp: " << expression << "\n";
    return expression;
  }
  string getVarFromInstruct(Instruction *instruct)
  {
    string result;
    if (isa<LoadInst> (*instruct))
    {
      LoadInst *loadInst = dyn_cast<LoadInst> (instruct);
      Value *useVal = loadInst->getPointerOperand();
      result = useVal->getName().str();
    }
    if (isa<StoreInst> (*instruct))
    {
      StoreInst *storeInst = dyn_cast<StoreInst> (instruct);
      Value *defVal = storeInst->getPointerOperand();
      result = defVal->getName().str();
    }
    return result;
  }
  string getOpFromOpName(string operatorName)
  {
    string op;
    if (operatorName == "add")
    {
      op = "+";
    }
    else if (operatorName == "sub")
    {
      op = "-";
    }
    else if (operatorName == "mul")
    {
      op = "*";
    }
    else if (operatorName == "sdiv")
    {
      op = "/";
    }
    return op;
  }
  string returnNameFromVal(Value *V)
    {
      string block_address;
      raw_string_ostream string_stream(block_address);
      V->printAsOperand(string_stream, false);
      return string_stream.str();
    }
}; // end of struct AvailExpression
} // end of anonymous namespace

char AvailExpression::ID = 0;
static RegisterPass<AvailExpression> X("AvailExpression", "AvailExpression Pass",
                                      false /* Only looks at CFG */,
                                      true /* Analysis Pass */);
