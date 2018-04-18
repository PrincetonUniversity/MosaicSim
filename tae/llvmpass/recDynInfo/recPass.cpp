#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"
#include "cxxabi.h"
#include <string>

// TODO: Construct a map for terminal instructions

using namespace llvm;
#define KERNEL_STR "_kernel_"

namespace {
struct recordDynamicInfo : public ModulePass {
  static char ID;
  Function *printBR;
  Function *printSw;
  Function *printMem;
  Module *mod;
  recordDynamicInfo() : ModulePass(ID) {}
  bool runOnModule(Module &M) override {
    mod = &(M);
    LLVMContext& ctx = M.getContext();
    Constant *printBRFunc = M.getOrInsertFunction("_Z11printBranchPciS_S_", Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx));
    std::vector<Type*> targs;
    targs.push_back(Type::getInt8PtrTy(ctx));
    targs.push_back(Type::getInt32Ty(ctx));
    targs.push_back(Type::getInt8PtrTy(ctx));
    targs.push_back(Type::getInt32Ty(ctx));
    FunctionType *sF = FunctionType::get(Type::getVoidTy(ctx), targs, true);
    Constant *printSwitchFunc = M.getOrInsertFunction("_Z7printSwPciS_iz", sF);
    Constant *printMemFunc = M.getOrInsertFunction("_Z8printMemPcbxi", Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt1Ty(ctx), Type::getInt64Ty(ctx), Type::getInt32Ty(ctx));
    printBR= cast<Function>(printBRFunc);
    printSw= cast<Function>(printSwitchFunc);
    printMem = cast<Function>(printMemFunc);
    for (Module::iterator fI = M.begin(), fE = M.end(); fI != fE; ++fI) {
        if (isFoI(*fI)) {
          errs() << fI->getName() << "\n";
          instInspect(*fI);
        }
    }
    return false;
  }
  int findID(BasicBlock *bb, Instruction *ins)
  {
    int ct = 0;

    for (Instruction &I : *bb) {
        Instruction *curins = &(I);
        if(ins == curins)
          return ct;
      ct++;
    }
    return 0;
    assert(false);
  }
  bool isFoI(Function &F) {
    return (F.getName().str().find(KERNEL_STR) != std::string::npos);
  }
  void printBranch(BranchInst *pi) {  
    IRBuilder<> Builder(pi);
    LLVMContext& ctx = mod->getContext();
    Value *v = pi->getCondition();
    StringRef n1;
    StringRef n2;
    if(v->getType()->isIntegerTy()) {
      n1 = pi->getSuccessor(1)->getName();
      n2 = pi->getSuccessor(0)->getName();
    }
    else
      assert(false);     
    errs() << "[Branch] " << *pi << " - " << n1 << " / " << n2 << "\n";
    Value *castI;
    castI = Builder.CreateZExt(v, Type::getInt32Ty(ctx), "castInst");
    Value *name = Builder.CreateGlobalStringPtr(pi->getParent()->getName());
    Value *name1= Builder.CreateGlobalStringPtr(n1);
    Value *name2= Builder.CreateGlobalStringPtr(n2);
    Value* args[] = {name, castI, name1, name2};
    CallInst* cI = Builder.CreateCall(printBR, args);
  }
  void printSwitch(SwitchInst *pi) {  
    IRBuilder<> Builder(pi);
    LLVMContext& ctx = mod->getContext();
    Value *v = pi->getCondition();
    if(!v->getType()->isIntegerTy()) {
      assert(false);
    }    
    errs() << "[Switch] " << pi->getNumCases() << " : " << *pi << "\n";
    Value *castI;
    castI = Builder.CreateZExt(v, Type::getInt32Ty(ctx), "castInst");
    std::vector<Value*> args;
    int len = pi->getNumCases();
    ConstantInt *length = llvm::ConstantInt::get(ctx, llvm::APInt(32, len , false));
    int id = findID(pi->getParent(), pi);
    std::string namestr = pi->getParent()->getName().str() + "/" + std::to_string(id);
    Value *name = Builder.CreateGlobalStringPtr(namestr);
    Value *def = Builder.CreateGlobalStringPtr(pi->getOperand(1)->getName());
    args.push_back(name);
    args.push_back(castI);
    args.push_back(def);
    args.push_back(length);
    for(int i=0; i<pi->getNumCases(); i++) {
      Value *n = Builder.CreateGlobalStringPtr(pi->getOperand(3+i*2)->getName());
      args.push_back(pi->getOperand(2+i*2));
      args.push_back(n);
    }
    CallInst* cI = Builder.CreateCall(printSw, args);
  }
  void printMemory(Instruction *pi) {
    IRBuilder<> Builder(pi);
    bool isLoad = false;
    LLVMContext& ctx = mod->getContext();
    DataLayout* dl = new DataLayout(mod);
    Value *v, *castI, *name;
    ConstantInt *ctype;
    if(auto *li = dyn_cast<LoadInst>(pi)) {
      isLoad = true;
      errs() << "[Load] " << *pi << "\n";
      v = li->getPointerOperand();
      name = Builder.CreateGlobalStringPtr(pi->getName());
      ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 0, false));
    }
    else if(auto *si = dyn_cast<StoreInst>(pi)) {
      isLoad = false;
      errs() << "[Store] " << *pi << "\n";
      v = si->getPointerOperand();
      int id = findID(pi->getParent(), pi);
      std::string namestr = pi->getParent()->getName().str() + "/" + std::to_string(id);
      name = Builder.CreateGlobalStringPtr(namestr);
      ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 1, false));
    }
    else 
      assert(false);
    castI = Builder.CreatePtrToInt(v, llvm::Type::getInt64Ty(ctx), "castInst");
    PointerType* vPtrType = cast<PointerType>(v->getType());                         
    uint64_t storeSize = dl->getTypeStoreSize(vPtrType->getPointerElementType());
    ConstantInt *csize = llvm::ConstantInt::get(ctx, llvm::APInt(32, storeSize, true));
    Value* args[] = {name, ctype, castI, csize};
    CallInst* cI = Builder.CreateCall(printMem, args);
  }
  void instInspect(Function &F) {
    for (inst_iterator iI = inst_begin(F), iE = inst_end(F);iI != iE; iI++) {
        Instruction *inst = &(*iI);
        if (auto *bI = dyn_cast<BranchInst>(inst)) {
          if(bI->isConditional()) {
            printBranch(bI);
          }
        }
        else if (auto *bI = dyn_cast<IndirectBrInst>(inst)) {
          // Not Supported
          assert(false);
        }
        else if (auto *bI = dyn_cast<SwitchInst>(inst)) {
          printSwitch(bI);
        }
        else if(auto *lI = dyn_cast<LoadInst>(inst))
          printMemory(inst);
        else if(auto *sI = dyn_cast<StoreInst>(inst))
          printMemory(inst);

    }
  }
};
} 
char recordDynamicInfo::ID = 0;
static RegisterPass<recordDynamicInfo> X("recdyninfo", "Record Dynamic Information for Oracle",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);