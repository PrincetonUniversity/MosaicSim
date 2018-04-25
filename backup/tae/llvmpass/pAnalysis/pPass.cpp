#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include <string>

#define KERNEL_STR "_kernel_"
using namespace llvm;
namespace {
struct pAnalysis : public FunctionPass {
  static char ID;
  Function *func;
  DataLayout *dlp;
  MemorySSA *MSSA;
  AliasAnalysis *AA;

  pAnalysis() : FunctionPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequired<MemorySSAWrapperPass>();
    AU.addPreserved<MemorySSAWrapperPass>();
    AU.setPreservesAll();
  }
  bool runOnFunction(Function &F) override {
    func = &F;
    LLVMContext& ctx = F.getContext();
    dlp = new DataLayout(F.getParent());
    auto &AAR = getAnalysis<AAResultsWrapperPass>().getAAResults();
    auto &MSSAR = getAnalysis<MemorySSAWrapperPass>().getMSSA();
    AA = &AAR;
    MSSA = &MSSAR;  
    if (isFoI(F)) {
      errs() << F.getName() << "\n";
      instInspect(F);
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
  AliasResult pointerAlias(Value *P1, Value *P2) {
    DataLayout &DL = *dlp;
    uint64_t P1Size = MemoryLocation::UnknownSize;
    Type *P1ElTy = cast<PointerType>(P1->getType())->getElementType();
    if (P1ElTy->isSized()) {
      P1Size = DL.getTypeStoreSize(P1ElTy);
    }
    uint64_t P2Size = MemoryLocation::UnknownSize;
    Type *P2ElTy = cast<PointerType>(P2->getType())->getElementType();
    if (P2ElTy->isSized()) {
      P2Size = DL.getTypeStoreSize(P2ElTy);
      return AA->alias(P1, P1Size, P2, P2Size);
    }
  }
  Value* returnPointer(Instruction *inst) {
    if(inst == NULL) {
      assert(false);
    }
    LoadInst *lI = NULL;
    StoreInst *sI = NULL;
    Value *pval = NULL;
    if(isa<LoadInst>(inst)) {
      lI = dyn_cast<LoadInst>(inst);
      pval = lI->getPointerOperand();
    }
    else if(isa<StoreInst>(inst)) {
      sI = dyn_cast<StoreInst>(inst);
      pval = sI->getPointerOperand();
    }
    else
      assert(false);
    return pval;
  }
  void instInspect(Function &F) {
    for (inst_iterator iI = inst_begin(F), iE = inst_end(F);iI != iE; iI++) {
        Instruction *inst = &(*iI);
        if(inst == NULL)
          continue;
        if (isa<LoadInst>(inst) || isa<StoreInst>(inst)) {
          Value *pinst =returnPointer(inst);
          if(MSSA->getWalker() == NULL)
            assert(0);
          MemoryAccess *m = MSSA->getWalker()->getClobberingMemoryAccess(inst);
          if(m == NULL)
            assert(false);
          else if (auto *u = dyn_cast<MemoryUse>(m)) {
            Instruction *dI = u->getMemoryInst();
            if(dI != NULL) {
              Value *dP = returnPointer(dI);
              AliasResult ar = pointerAlias(pinst, dP);
              errs() << "Alias : " << ar << "\n";
              errs() << *dI <<"(" << *u << ")"  << " -> " << *inst << " (" << *(MSSA->getMemoryAccess(inst)) << ")" << "\n";

            }
          }
          else if(auto *d = dyn_cast<MemoryDef>(m)) {
            Instruction *dI = d->getMemoryInst();
            if(dI != NULL) {
              Value *dP = returnPointer(dI);
              AliasResult ar = pointerAlias(pinst, dP);
              errs() << "Alias : " << ar << "\n";
              errs() << *dI << "(" << *d << ")" << " -> " << *inst <<  " (" << *(MSSA->getMemoryAccess(inst)) << ")" <<"\n";
            }
          }
          else if(auto *p = dyn_cast<MemoryPhi>(m)) {
            errs() << "["<<p->getBlock()->getName()  << "]" << *p << " -> " << *inst <<  " (" << *(MSSA->getMemoryAccess(inst)) << ")" <<"\n";
          }
        }
    }
  }
};
}
char pAnalysis::ID = 0;
static RegisterPass<pAnalysis> X("pPass", "Pointer Analysis Test", false, false);