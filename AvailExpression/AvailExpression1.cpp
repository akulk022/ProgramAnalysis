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
  AvailExpression() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override
  {
    errs() << "AvailExpression: ";
    errs() << F.getName() << "\n";
    unordered_map<string, set<string>> gensBB; // 
    unordered_map<string, set<string>> killsBB; //
    unordered_map<string, set<string>> insBB; //
    unordered_map<string, set<string>> outsBB; //

    set<string> allExpressions;
    for(auto &basic_block: F){
      
    }

    set<string> currentExpressions;
    for(auto &basic_block: F){
      std::string bbname = basic_block.getName().str();
      errs() << "Basic Block: " << bbname << "\n";
      set<string> generatedExpressions = getGeneratedExpressions(&basic_block);
      
      for (string element : generatedExpressions){
        currentExpressions.insert(element);
      }

      set<string> killedVariables = getKilledExpressions(&basic_block);
      set<string> killedExpressions;
      for(string temp_string : killedVariables){
        for(string exp : currentExpressions){
          // check the variable kills a previously generated expression
          if (!exp.find(temp_string)){ 
            killedExpressions.insert(exp);
          }
        }
      }

      gensBB[bbname] = generatedExpressions;
      killsBB[bbname] = killedExpressions;
      outsBB[bbname] = gensBB[bbname]; // by default

      //updating trackExp with all the expressions generated from a basic block

    }
    unordered_map<string, set<string>> outOld;
    unordered_map<string, bool> exitCondition;
    bool changed=true;
    do {
      changed=true; 
       
      set<string> intersectSet;
      for (auto &basic_block : F) {
        std::string bbname = basic_block.getName().str();
        errs() << "Block Name - "<<bbname<<"\n";
        outOld[bbname] = outsBB[bbname];
        intersectSet.clear();
        intersectSet = pred_begin(&basic_block) ;
        for (BasicBlock *pred : predecessors(&basic_block)) {
            string bbname = pred->getName().str();
            std::set_intersection(intersectSet.begin(), intersectSet.end(),
                              outsBB[bbname].begin(), outsBB[bbname].end(),
                              intersectSet.begin());
        }
        insBB[bbname] = intersectSet;

        set<string> disjointSet;
        std::set_difference(intersectSet.begin(), intersectSet.end(),
                            killsBB[bbname].begin(), killsBB[bbname].end(),
                            inserter(disjointSet, disjointSet.end()));
        disjointSet.insert(gensBB[bbname].begin(), gensBB[bbname].end());
        outsBB[bbname] = disjointSet;
        if (outsBB[bbname] == outOld[bbname]) {
            exitCondition[bbname] = true;
            errs() << "\n" << bbname << " block - No change in Input\n";
        }
        else{
          	exitCondition[bbname] = false;
        }
      }
      //method to check if all blocks have reached exit condition, if all have remained unchanged then this will help exit the do-while loop
      for (const auto &element : exitCondition) {
        changed = changed && element.second;
      }


    }while(!changed);
    return true;
  }
  /* 1. First we fetch the expression along with it's variables
     2. Add the expression in generated expressions
     3. Variables are put in a set for checking
     4. If we encounter a definition for one of these variables,
        we remove the expression from generated expressions */
  set<string> getGeneratedExpressions(BasicBlock* bb){
    //Iterating over the instructions in the basic block
    set<string> genExpressions;
    set<string> defCheck;
    for(Instruction &instruct:*bb){
      vector<string> expressionVec;
      //We only care about BinaryOperator Instructions for expressions
      if(isa<BinaryOperator>(instruct)){
        expressionVec = getExpressionFromInstruct(&instruct);
        errs() << "Expression: " << expressionVec[2] << "\n";
        genExpressions.insert(expressionVec[2]);
        defCheck.insert(expressionVec[0]);
        defCheck.insert(expressionVec[1]); 
      }
      //Checking if a definition belongs to a variable from expression from block
      if(isa<StoreInst>(instruct)){
        string var = getVarFromInstruct(&instruct);
        if(existsKey(defCheck,var)){
          genExpressions.erase(genExpressions.find(var));
        }
      }
    }
    return genExpressions;
  }
  set<string> getKilledExpressions(BasicBlock* bb,set<string> trackExp = {}){
    set<string> killedExpressions;

    for (Instruction &instruct : *bb) {
      if(isa<StoreInst>(instruct)){
        string var = getVarFromInstruct(&instruct);
        killedExpressions.insert(var);
      }
    }
    return killedExpressions;


  }
  //Method will extract expression from a binary instruction
  vector<string> getExpressionFromInstruct(Instruction *instruct){
    vector<string> expressionVec;
    BinaryOperator *binaryOp = dyn_cast<BinaryOperator>(instruct);
    LoadInst *loadInst1 = dyn_cast<LoadInst>(binaryOp->getOperand(0));
    LoadInst *loadInst2 = dyn_cast<LoadInst>(binaryOp->getOperand(1));
    Value* val1 = loadInst1->getPointerOperand();
    Value* val2 = loadInst2->getPointerOperand();
    string var1 = val1->getName().str();
    expressionVec.push_back(var1);
    string var2 = val2->getName().str();
    expressionVec.push_back(var2);
    string operatorName = binaryOp->getOpcodeName();
    string op = getOpFromOpName(operatorName);
    string expression = var1 + op + var2 ;
    expressionVec.push_back(expression);
    return expressionVec;
  }
  string getOpFromOpName(string operatorName){
    string op;
    if(operatorName == "add"){
      op = "+";
    }else if(operatorName == "sub"){
      op = "-";
    }else if(operatorName == "mul"){
      op = "*";
    }else if(operatorName == "sdiv"){
      op = "/";
    }
    return op;
  }
  string getVarFromInstruct(Instruction *instruct){
	  string result;
	  if(isa<LoadInst>(*instruct)){
		  LoadInst *loadInst = dyn_cast<LoadInst>(instruct);
		  Value* useVal = loadInst->getPointerOperand();
		  result = useVal->getName().str();
	  }
	  if(isa<StoreInst>(*instruct)){
		  StoreInst *storeInst = dyn_cast<StoreInst>(instruct);
		  Value* defVal = storeInst->getPointerOperand();
		  result = defVal->getName().str();
	  }
	  return result;
  }
  bool existsKey(set<string> set, string key){
	  if(set.find(key) != set.end()){
		  return true;
	  }else{
		  return false;
	  }
  }
}; // end of struct AvailExpression
} // end of anonymous namespace

char AvailExpression::ID = 0;
static RegisterPass<AvailExpression> X("AvailExpression", "AvailExpression Pass",
                                      false /* Only looks at CFG */,
                                      true /* Analysis Pass */);
