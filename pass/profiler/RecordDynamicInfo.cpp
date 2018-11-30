#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/CommandLine.h"
#include "cxxabi.h"
#include <string>

using namespace llvm;
#define KERNEL_STR "_kernel_"

namespace {
  struct RecordDynamicInfo : public ModulePass {
    static char ID;
    Function *printuBR;
    Function *printBR;
    Function *printSw;
    Function *printMem;
    Module *mod;
    std::map<BasicBlock*, int> bb_id_table;
    std::map<Instruction*, int> id_table;
    RecordDynamicInfo() : ModulePass(ID) {}
    bool runOnModule(Module &M) override {
      mod = &(M);
      std::string k1 = "_Z12printuBranchPcS_";
      std::string k2 = "_Z11printBranchPciS_S_";
      std::string k3 = "_Z7printSwPciS_iz";
      std::string k4 = "_Z8printMemPcbxi";

      LLVMContext& ctx = M.getContext();
      Constant *printuBRFunc = M.getOrInsertFunction(k1, Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx));
      Constant *printBRFunc = M.getOrInsertFunction(k2, Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt32Ty(ctx), Type::getInt8PtrTy(ctx), Type::getInt8PtrTy(ctx));
      std::vector<Type*> targs;
      targs.push_back(Type::getInt8PtrTy(ctx));
      targs.push_back(Type::getInt32Ty(ctx));
      targs.push_back(Type::getInt8PtrTy(ctx));
      targs.push_back(Type::getInt32Ty(ctx));
      FunctionType *sF = FunctionType::get(Type::getVoidTy(ctx), targs, true);
      Constant *printSwitchFunc = M.getOrInsertFunction(k3, sF);
      Constant *printMemFunc = M.getOrInsertFunction(k4, Type::getVoidTy(ctx), Type::getInt8PtrTy(ctx), Type::getInt1Ty(ctx), Type::getInt64Ty(ctx), Type::getInt32Ty(ctx));
      printuBR = cast<Function>(printuBRFunc);
      printBR= cast<Function>(printBRFunc);
      printSw= cast<Function>(printSwitchFunc);
      printMem = cast<Function>(printMemFunc);
      for (Module::iterator fI = M.begin(), fE = M.end(); fI != fE; ++fI) {
        if (isFoI(*fI)) {
          assignID(*fI);
          errs() << fI->getName() << "\n";
          instInspect(*fI);
        }
      }
      return false;
    }
    void assignID(Function &F) {
      int bb_id = 0;
      int id = 0;
      for (BasicBlock &B : F)  {
        bb_id_table.insert(std::make_pair(&B, bb_id));
        bb_id++;
        for(Instruction &I : B) {
          id_table.insert(std::make_pair(&I, id));
          id++;
        }
      }
    }
    int findBID(BasicBlock *bb)
    {
      return bb_id_table.at(bb);
    }
    int findID(Instruction *ins)
    {
      return id_table.at(ins);
    }
    bool isFoI(Function &F) {
      return (F.getName().str().find(KERNEL_STR) != std::string::npos);
    }
    void printInvoke(InvokeInst *pi) {
      IRBuilder<> Builder(pi);
      LLVMContext& ctx = mod->getContext();
      BasicBlock *bb = pi->getNormalDest();
      std::string temp= std::to_string(findBID(bb));
      StringRef p = temp;
      errs() << "[invoke] " << *pi << " - " << p << "\n";
      std::string bbname = std::to_string(findBID(pi->getParent()));
      Value *name = Builder.CreateGlobalStringPtr(bbname);
      Value *name1= Builder.CreateGlobalStringPtr(p);
      Value* args[] = {name, name1};
      CallInst* cI = Builder.CreateCall(printuBR, args);
    }
    void printBranch(BranchInst *pi, int conditional) {
      IRBuilder<> Builder(pi);
      LLVMContext& ctx = mod->getContext();
      if(conditional) {
        std::string temp1, temp2;
        Value *v = pi->getCondition();
        if(v->getType()->isIntegerTy()) {
          BasicBlock *b1 = pi->getSuccessor(1);
          BasicBlock *b2 = pi->getSuccessor(0);
          temp1 = std::to_string(findBID(b1));
          temp2 = std::to_string(findBID(b2));
        }
        else
          assert(false);
        StringRef p1, p2;
        Value *castI;
        p1 = temp1;
        p2 = temp2;
        errs() << "[Branch] " << *pi << " - " << p1 << " / " << p2 << "\n";
        castI = Builder.CreateZExt(v, Type::getInt32Ty(ctx), "castInst");
        std::string bbname = std::to_string(findBID(pi->getParent()));
        Value *name = Builder.CreateGlobalStringPtr(bbname);
        Value *name1= Builder.CreateGlobalStringPtr(p1);
        Value *name2= Builder.CreateGlobalStringPtr(p2);
        Value* args[] = {name, castI, name1, name2};
        CallInst* cI = Builder.CreateCall(printBR, args);
      }
      else {
        BasicBlock *bb = pi->getSuccessor(0);
        std::string temp= std::to_string(findBID(bb));
        StringRef p = temp;
        errs() << "[uBranch] " << *pi << " - " << p << "\n";
        std::string bbname = std::to_string(findBID(pi->getParent()));
        Value *name = Builder.CreateGlobalStringPtr(bbname);
        Value *name1= Builder.CreateGlobalStringPtr(p);
        Value* args[] = {name, name1};
        CallInst* cI = Builder.CreateCall(printuBR, args);
      }
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
      std::string namestr = std::to_string(findBID(pi->getParent()));//pi->getParent()->getName().str() + "/" + std::to_string(id);
      Value *name = Builder.CreateGlobalStringPtr(namestr);
      BasicBlock *bdefault = dyn_cast<BasicBlock>(pi->getOperand(1));
      std::string defstr = std::to_string(findBID(bdefault));
      Value *def = Builder.CreateGlobalStringPtr(defstr);
      args.push_back(name);
      args.push_back(castI);
      args.push_back(def);
      args.push_back(length);
      for(int i=0; i<pi->getNumCases(); i++) {
        BasicBlock *bop = dyn_cast<BasicBlock>(pi->getOperand(3+i*2));
        std::string bbstr = std::to_string(findBID(bop));
        errs() << "BB : " << bbstr << "\n";
        Value *n = Builder.CreateGlobalStringPtr(bbstr);
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
        std::string namestr = std::to_string(findID(pi));//pi->getParent()->getName().str() + "/" + std::to_string(id);
        name = Builder.CreateGlobalStringPtr(namestr);
        ctype = llvm::ConstantInt::get(ctx, llvm::APInt(1, 0, false));
      }
      else if(auto *si = dyn_cast<StoreInst>(pi)) {
        isLoad = false;
        errs() << "[Store] " << *pi << "\n";
        v = si->getPointerOperand();
        std::string namestr = std::to_string(findID(pi));//pi->getParent()->getName().str() + "/" + std::to_string(id);
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
            printBranch(bI, 1);
          }
          else
            printBranch(bI, 0);
        }
        else if (auto *bI = dyn_cast<IndirectBrInst>(inst)) {
          assert(false);
        }
        else if (auto *bI = dyn_cast<SwitchInst>(inst)) {
          printSwitch(bI);
        }
        else if(auto *lI = dyn_cast<LoadInst>(inst))
          printMemory(inst);
        else if(auto *sI = dyn_cast<StoreInst>(inst))
          printMemory(inst);
        else if(auto *iI = dyn_cast<InvokeInst>(inst))
          printInvoke(iI);
        else if(auto *tI = dyn_cast<TerminatorInst>(inst))
          errs() << "[WARNING] Terminator Instruction not handled : " << *inst <<"\n";
      }
    }
  };
}
char RecordDynamicInfo::ID = 0;
static RegisterPass<RecordDynamicInfo> X("recorddynamicinfo", "Record Dynamic Information for Oracle",
                                         false /* Only looks at CFG */,
                                         false /* Analysis Pass */);
