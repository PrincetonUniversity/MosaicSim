#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include <string>
#include <set>
#include <sstream>
#include <fstream>
#include "graph.h"
#define KERNEL_STR "_kernel_"

using namespace llvm;
namespace apollo {

// Identify Induction Variable
// Track MemoryPHI

struct graphGen : public FunctionPass {
  static char ID;
  DataLayout *dlp;
  MemorySSA *MSSA;
  MemoryDependenceResults *MDA;
  AliasAnalysis *AA;
  Graph g;

  graphGen() : FunctionPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequiredTransitive<AAResultsWrapperPass>();
    AU.addRequiredTransitive<AssumptionCacheTracker>();
    AU.addRequiredTransitive<TargetLibraryInfoWrapperPass>();
    AU.addRequiredTransitive<MemorySSAWrapperPass>();
    AU.addRequiredTransitive<MemoryDependenceWrapperPass>();
    AU.addRequired<DependenceAnalysisWrapperPass>();
    AU.setPreservesAll();
  }

  bool runOnFunction(Function &F) override {
    LLVMContext& ctx = F.getContext();
    dlp = new DataLayout(F.getParent());
    auto &AAR = getAnalysis<AAResultsWrapperPass>().getAAResults();
    auto &MSSAR = getAnalysis<MemorySSAWrapperPass>().getMSSA();
    auto &MDAR = getAnalysis<MemoryDependenceWrapperPass>().getMemDep();
    AA = &AAR;
    MSSA = &MSSAR;
    MDA = &MDAR;
    if (isFoI(F)) {
      errs() << F.getName() << "\n";
      dataDependence(F);
      controlDependence(F);
      //memoryDependence(F);
      dependenceCheck(F);
      memdepCheck(F);
      g.visualize();
    }
    return false;
  }
  
  bool isFoI(Function &F) {
    return (F.getName().str().find(KERNEL_STR) != std::string::npos);
  }
  void memdepCheck(Function &F) { 
    for (auto &I : instructions(F)) {
     Instruction *inst = &I;
     if (!inst->mayReadFromMemory() && !inst->mayWriteToMemory())
       continue;
     MemDepResult Res = MDA->getDependency(inst);
     if(Res.getInst() != NULL)
      errs() << *inst << " Depends on " << *(Res.getInst()) << "\n";
     if(Res.isNonLocal()) {
       SmallVector<NonLocalDepResult, 4> nldi;
       MDA->getNonLocalPointerDependency(inst, nldi);
       for (const NonLocalDepResult &I : nldi) {
         MemDepResult nlRes = I.getResult();
         if(nlRes.getInst() != NULL)
          errs() << *inst << "Non-locally Depends on " << *(nlRes.getInst()) << "\n";
       }
     }
   }
  }
  void dependenceCheck(Function &F) {
    DependenceInfo *depinfo = &getAnalysis<DependenceAnalysisWrapperPass>().getDI();
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
      for (inst_iterator J = I; J != E; J++) {
        Instruction *src = (&*I);
        Instruction *dst = (&*J);
        if(isa<LoadInst>(src) && isa<LoadInst>(dst))
          continue;
        if(!isa<LoadInst>(src) && !isa<StoreInst>(src))
          continue;
        if(!isa<LoadInst>(dst) && !isa<StoreInst>(dst))
          continue;
        std::unique_ptr<Dependence> infoPtr;
        infoPtr = depinfo->depends(src, dst, true);
        Dependence *dep = infoPtr.get();
        if (dep != NULL && !dep->isInput()) {
          if(dep->isConfused()) {
            errs() << "[C]";
          }
          else if (dep->isLoopIndependent())  {
            if(dep->isConsistent())
              g.addEdge(src, dst, 2);
            else
              g.addEdge(src, dst, 4);
          }
          else {
            
            unsigned Levels = dep->getLevels();
            int dist = 0;
            for (unsigned II = 1; II <= Levels; ++II) {
              const SCEV *Distance = dep->getDistance(II);
              if(Distance != NULL) {
                 if (const SCEVConstant *SC = dyn_cast<SCEVConstant>(Distance))
                    dist = SC->getValue()->getSExtValue();
              }
            }
            if(dep->isConsistent())
              g.addEdge(src, dst, 5, dist);
            else
              g.addEdge(src, dst, 6, dist);
          }   
          dep->getDst()->print(errs(), false);
          errs() << "   ---> ";
          dep->getSrc()->print(errs(), false);
          errs() << "\n";
        }
      }
    }
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
    // FIXME
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
          //g.addEdge(b, inst, 2);
        }
      }
    }
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


*/