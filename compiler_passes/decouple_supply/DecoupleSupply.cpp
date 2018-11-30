#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/InstIterator.h"

#define KERNEL_STR "_kernel_"
#define MAIN_STR "main"
#define SUPPLY_FLAG 0
using namespace std;
using namespace llvm;

namespace {

  int LOADS = 0;
  int STORES = 0;

  Function *supply_consume_i8;
  Function *supply_consume_i32;
  Function *supply_consume_i64;
  Function *supply_consume_ptr;

  Function *supply_produce_i8;
  Function *supply_produce_i32;
  Function *supply_produce_i64;
  Function *supply_produce_ptr;

  Function *desc_init;
  Function *desc_cleanup;


  vector<Instruction *> to_erase;

  void populate_functions(Module &M) {
    
    supply_consume_i8 = (Function *) M.getOrInsertFunction("desc_supply_consume_i8",
							   FunctionType::get(Type::getVoidTy(M.getContext()), Type::getInt8PtrTy(M.getContext())));
    
    supply_consume_i32 = (Function *) M.getOrInsertFunction("desc_supply_consume_i32",
							    FunctionType::get(Type::getVoidTy(M.getContext()), Type::getInt32PtrTy(M.getContext())));
    
    supply_consume_i64 = (Function *) M.getOrInsertFunction("desc_supply_consume_i64",
							    FunctionType::get(Type::getVoidTy(M.getContext()), Type::getInt64PtrTy(M.getContext())));
    
    // using *i8 and will cast later
    supply_consume_ptr = (Function *) M.getOrInsertFunction("desc_supply_consume_ptr",
							    FunctionType::get(Type::getVoidTy(M.getContext()), Type::getInt8PtrTy(M.getContext())->getPointerTo()));
    
    supply_produce_i8 = (Function *) M.getOrInsertFunction("desc_supply_produce_i8",
							   FunctionType::get(Type::getInt8Ty(M.getContext()),Type::getInt8Ty(M.getContext())));
    
    supply_produce_i32 = (Function *) M.getOrInsertFunction("desc_supply_produce_i32",
							    FunctionType::get(Type::getInt32Ty(M.getContext()),Type::getInt32Ty(M.getContext())->getPointerTo()));
    
    supply_produce_i64 = (Function *) M.getOrInsertFunction("desc_supply_produce_i64",
							    FunctionType::get(Type::getInt64Ty(M.getContext()),Type::getInt64Ty(M.getContext())->getPointerTo()));
    
    // using *i8 and will cast later
    supply_produce_ptr = (Function *) M.getOrInsertFunction("desc_supply_produce_ptr",
							    FunctionType::get(Type::getInt8PtrTy(M.getContext()),Type::getInt8PtrTy(M.getContext())->getPointerTo()));
    
    desc_init = (Function *) M.getOrInsertFunction("desc_init",
						   FunctionType::get(Type::getVoidTy(M.getContext()),Type::getInt32Ty(M.getContext())));
    
    desc_cleanup = (Function *) M.getOrInsertFunction("desc_cleanup",
						   FunctionType::get(Type::getVoidTy(M.getContext()), false));

  }
  

  bool isKernelFunction(Function &func) {
    return (func.getName().str().find(KERNEL_STR) != std::string::npos); 
  }

  bool isMain(Function &func) {
    return (func.getName().str().find(MAIN_STR) != std::string::npos); 
  }


  bool non_ptr_load(Instruction *I) {
    return I->getType()->isIntegerTy(8) || I->getType()->isIntegerTy(32) || I->getType()->isIntegerTy(64);
  }

  bool ptr_load(Instruction *I) {
    return I->getType()->isPointerTy();
  }


  Function *get_non_ptr_produce(Instruction *I) {
    if (I->getType()->isIntegerTy(8)) {
      return supply_produce_i8;
    }
    else if (I->getType()->isIntegerTy(32)) {
      return supply_produce_i32;
    }
    else if (I->getType()->isIntegerTy(64)) {
      return supply_produce_i64;
    }
    else {
      errs() << "[Error: unsupported non-pointer type]\n";
      errs() << *I << "\n";
      errs() << *(I->getType()) << "\n";
      assert(0);      
    }	
  }

  void instrument_load(Module &M, LoadInst *I) {
    LOADS++;
    Instruction *Intr;
    Value *Intr2;
    Value *Intr_pre;
    IRBuilder<> Builder(I);

    if (non_ptr_load(I)) {
      Function *supply_produce_non_ptr_func = get_non_ptr_produce(I);
      Intr = Builder.CreateCall(supply_produce_non_ptr_func, {I->getPointerOperand()});
      I->replaceAllUsesWith(Intr);
      to_erase.push_back(I);
    }
    else if (ptr_load(I)) {
      Intr_pre = Builder.CreatePointerCast(I->getPointerOperand(), Type::getInt8PtrTy(M.getContext())->getPointerTo());
      Intr = Builder.CreateCall(supply_produce_ptr,{Intr_pre});
      Intr2 = Builder.CreatePointerCast(Intr, I->getType());
      I->replaceAllUsesWith(Intr2);
      to_erase.push_back(I);      
    }
    else {
      errs() << "[Error: Could not find a type for the load instruction]\n";
      errs() << *I << "\n";
      errs() << *(I->getType()) << "\n";
      assert(0);      
    }
  }

  bool non_ptr_store(StoreInst *stI) {
    Value *I = stI->getValueOperand();
    return I->getType()->isIntegerTy(8) || I->getType()->isIntegerTy(32) || I->getType()->isIntegerTy(64);
  }

  bool ptr_store(StoreInst *stI) {
    Value *I = stI->getValueOperand();
    return I->getType()->isPointerTy();
  }


  Function *get_non_ptr_consume(StoreInst *stI) {
    Value *I = stI->getValueOperand();
    if (I->getType()->isIntegerTy(8)) {
      return supply_consume_i8;
    }
    else if (I->getType()->isIntegerTy(32)) {
      return supply_consume_i32;
    }
    else if (I->getType()->isIntegerTy(64)) {
      return supply_consume_i64;
    }
    else {
      errs() << "[Error: unsupported non-pointer type]\n";
      errs() << *I << "\n";
      errs() << *(I->getType()) << "\n";
      assert(0);      
    }	
  }
  
  void instrument_store(Module &M, StoreInst *I) {
    STORES++;
    Instruction *Intr;
    Value *Intr2;
    IRBuilder<> Builder(I);

    if (non_ptr_store(I)) {
      Function *supply_consume_non_ptr_func = get_non_ptr_consume(I);
      Intr = Builder.CreateCall(supply_consume_non_ptr_func, {I->getPointerOperand()});
      to_erase.push_back(I);
    }
    else if (ptr_store(I)) {
      //Intr2 = Builder.CreatePointerCast(I->getPointerOperand(), Type::getInt8PtrTy(M.getContext())->getPointerTo());
      //Intr = Builder.CreateCall(supply_consume_ptr,{Intr2});

      //to_erase.push_back(I);      
    }
    else {
      errs() << "[Error: Could not find a type for the load instruction]\n";
      errs() << *I << "\n";
      errs() << *(I->getType()) << "\n";
      assert(0);      
    }
  }


  void instrument_compute(Module &M, Function &f) {
    for (inst_iterator iI = inst_begin(&f), iE = inst_end(&f); iI != iE; ++iI) {
      if (isa<LoadInst>(*iI)) {
	instrument_load(M, dyn_cast<LoadInst>(&(*iI)));
      }
      if (isa<StoreInst>(*iI)) {
	instrument_store(M, dyn_cast<StoreInst>(&(*iI)));
      }
    }
    
    for (int i = 0; i < to_erase.size(); i++) {
      to_erase[i]->eraseFromParent();
    }
  }

  void insert_desc_main_init(Module &M, Function &f) {
    inst_iterator in = inst_begin(f);
    Instruction *InsertPoint = &(*in);
    IRBuilder<> Builder(InsertPoint);
    Builder.CreateCall(desc_init, {ConstantInt::get(Type::getInt32Ty(M.getContext()),SUPPLY_FLAG)});
  }
  
  void insert_desc_main_cleanup(Function &f) {
					      
    for (inst_iterator iI = inst_begin(f), iE = inst_end(f); iI != iE; ++iI) {
      if (ReturnInst::classof(&(*iI))) {
	Instruction *InsertPoint = (&(*iI));
	IRBuilder<> Builder(InsertPoint);
	Builder.CreateCall(desc_cleanup);
      }
    }
  }


  void instrument_main(Module &M, Function &f) {
    insert_desc_main_init(M, f);
    insert_desc_main_cleanup(f);    
  }

  struct DecoupleSupplyPass : public ModulePass {
    static char ID;
    DecoupleSupplyPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M) {
      errs() << "[Decouple Supply Pass Begin]\n";

      populate_functions(M);

      for (Module::iterator fI = M.begin(), fE = M.end(); fI != fE; ++fI) {
	if (isKernelFunction(*fI)) {
	  errs() << "[Found Kernel]\n";	  
	  errs() << "[" << fI->getName().str() << "]\n";
	  instrument_compute(M, *fI);
	}
      }

      for (Module::iterator fI = M.begin(), fE = M.end(); fI != fE; ++fI) {
	if (isMain(*fI)) {
	  errs() << "[Found Main]\n";	  
	  errs() << "[" << fI->getName().str() << "]\n";
	  instrument_main(M, *fI);
	}
      }

      errs() << "[STORES: " << STORES << "]\n";
      errs() << "[LOADS:  " << LOADS << "]\n";
      errs() << "[Decouple Supply Pass Finished]\n";
      return true;
    }
  };
}

char DecoupleSupplyPass::ID = 0;
static RegisterPass<DecoupleSupplyPass> X("decouplesupply", "Decouple Supply Pass", false, false);
