#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include <string>
#include <sstream>
#include <fstream>
#include "graph.h"
#define KERNEL_STR "_kernel_"

using namespace llvm;
namespace apollo {

struct graphGen : public FunctionPass {
  static char ID;
  DataLayout *dlp;
  MemorySSA *MSSA;
  AliasAnalysis *AA;
  Graph g;

  graphGen() : FunctionPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequired<MemorySSAWrapperPass>();
    AU.addPreserved<MemorySSAWrapperPass>();
    AU.setPreservesAll();
  }

  bool runOnFunction(Function &F) override {
    LLVMContext& ctx = F.getContext();
    dlp = new DataLayout(F.getParent());
    auto &AAR = getAnalysis<AAResultsWrapperPass>().getAAResults();
    auto &MSSAR = getAnalysis<MemorySSAWrapperPass>().getMSSA();
    AA = &AAR;
    MSSA = &MSSAR;
    if (isFoI(F)) {
      errs() << F.getName() << "\n";
      dataDependence(F);
      controlDependence(F);
      memoryDependence(F);
      g.visualize();
    }
    return false;
  }
  
  bool isFoI(Function &F) {
    return (F.getName().str().find(KERNEL_STR) != std::string::npos);
  }

  void controlDependence(Function &F) {
    for(auto &basicBlock: F) {
      BasicBlock *bb = &basicBlock;
      g.insertNode(bb);
    }
    for(auto &basicBlock: F) {
      BasicBlock *bb = &basicBlock;
      TerminatorInst *t = bb->getTerminator();
      for(int i=0; i<t->getNumSuccessors(); i++) {
        g.addEdge(t, t->getSuccessor(i),0);
      } 
      for (auto &phis : bb->phis()) {
          g.addEdge(bb, &phis, 0);
          for(int i=0; i<phis.getNumIncomingValues(); i++) {
              g.addEdge(phis.getIncomingValue(i),&(phis), 3);
          }
      }
      /*for(auto &instruction: basicBlock) {
        Instruction *inst = &instruction;
        g.addEdge(bb, inst, 3);
        if(inst != t)
          g.addEdge(inst, t, 4);
      }*/
    }
  }

  void dataDependence(Function &F) {
    for (inst_iterator iI = inst_begin(F), iE = inst_end(F);iI != iE; iI++) {
      Instruction *inst = &(*iI);
      g.insertNode(inst);
      if(isa<PHINode>(inst))
        continue;
      for (Use &U : inst->operands()) {
        Value *v = U.get();
        if(isa<BasicBlock>(v))
          continue;
        g.insertNode(v);
        g.addEdge(v, inst, 1);
      }
    }
  }

  void memoryDependence(Function &F) {
    for (inst_iterator iI = inst_begin(F), iE = inst_end(F);iI != iE; iI++) {
      Instruction *inst = &(*iI);
      if (isa<LoadInst>(inst) || isa<StoreInst>(inst)) {
        MemoryAccess *m = MSSA->getWalker()->getClobberingMemoryAccess(inst);
        if (auto *u = dyn_cast<MemoryUse>(m)) {
          Instruction *cinst = u->getMemoryInst();
          if(cinst != NULL)
            g.addEdge(cinst, inst, 2);
        }
        else if(auto *d = dyn_cast<MemoryDef>(m)) {
          Instruction *cinst = d->getMemoryInst();
          if(cinst != NULL)
            g.addEdge(cinst, inst, 2);
        }
        else if(auto *p = dyn_cast<MemoryPhi>(m)) {
          BasicBlock *b = p->getBlock();
          g.addEdge(b, inst, 2);
        }
      }
    }
    /*for(auto &basicBlock: F) {
      BasicBlock *bb = &basicBlock;
      MemoryPhi *phi = MSSA->getMemoryAccess(bb);
    }*/
  }
};
char graphGen::ID = 0;
static RegisterPass<graphGen> X("graphgen", "IR Dependence Graph Generator", false, false);
}

/* 


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
  }
  return AA->alias(P1, P1Size, P2, P2Size);
}

Value* getPointer(Instruction *inst) {
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
int getAliasType(Instruction *i, Instruction *j) {
  if(!isa<LoadInst>(i) && !isa<StoreInst>(i))
    assert(false);
  if(!isa<LoadInst>(j) && !isa<StoreInst>(j))
    assert(false);
  Value *vI = getPointer(i);
  Value *vJ = getPointer(j);
  int res = pointerAlias(vI, vJ);
  // No, May, Partial, Always
  return res;
}

*/