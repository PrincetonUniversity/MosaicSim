//===- InlineAlways.cpp - Code to inline always_inline functions ----------===//
 //
 //                     The LLVM Compiler Infrastructure
 //
 // This file is distributed under the University of Illinois Open Source
 // License. See LICENSE.TXT for details.
 //
 //===----------------------------------------------------------------------===//
 //
 // This file implements a custom inliner that handles only functions that
 // are marked as "always inline".
 //
 //===----------------------------------------------------------------------===//
 
 #include "llvm/Transforms/IPO/AlwaysInliner.h"
 #include "llvm/ADT/SetVector.h"
 #include "llvm/Analysis/AssumptionCache.h"
 #include "llvm/Analysis/InlineCost.h"
 #include "llvm/Analysis/TargetLibraryInfo.h"
 #include "llvm/IR/CallSite.h"
 #include "llvm/IR/CallingConv.h"
 #include "llvm/IR/DataLayout.h"
 #include "llvm/IR/Instructions.h"
 #include "llvm/IR/Module.h"
 #include "llvm/IR/Type.h"
 #include "llvm/Transforms/IPO.h"
 #include "llvm/Transforms/IPO/Inliner.h"
 #include "llvm/Transforms/Utils/Cloning.h"
 #include "llvm/Transforms/Utils/ModuleUtils.h"
#include <set> 
 using namespace llvm;
 
#define DEBUG_TYPE "inline"
#define KERNEL_STR "_kernel_"


std::set<Instruction*> notInlined;
std::set<Function*>* inlineSet = new std::set<Function*>();

bool isKernelFunction(Function &func) {
  return (func.getName().str().find(KERNEL_STR) != std::string::npos); 
}

void setInlineSet(Function* F, std::set<Function*>* inlineSet) { 
  for (auto& B:*F) {
    for (auto &I : B) {      
      if (isa<CallInst>(I)) {
        Instruction* Inst = &I;
        Function *f = ((CallInst*)Inst)->getCalledFunction();
        //errs() << "CALLED FUNCTION: " << f->getName() << "\n";
        if (!f->isDeclaration()  /*&&  isInlineViable(*f)*/) {
          inlineSet->insert(f);          
        }
        else {
          //errs() << "Couldn't INLINE: \n";
          notInlined.insert(&I);
        }
        setInlineSet(f, inlineSet);
        //I.dump();
        //errs() << I.getName().str() << "\n";
      }
    }
  }
}

PreservedAnalyses AlwaysInlinerPass::run(Module &M, ModuleAnalysisManager &) {
  InlineFunctionInfo IFI;
  SmallSetVector<CallSite, 16> Calls;
  bool Changed = false;
  SmallVector<Function *, 16> InlinedFunctions; 
  for (Function &F : M) {
    if(isKernelFunction (F)) {
      setInlineSet(&F, inlineSet);
    }     
  }
  
  //errs() << "Couldn't Inline: \n";
  for (auto& I: notInlined) {
    I->dump();
  }
  
  for (Function &F : M) {
    if (!F.isDeclaration() && inlineSet->find(&F)!=inlineSet->end()) {
      Calls.clear();
      
      for (User *U : F.users())
        if (auto CS = CallSite(U))
          if (CS.getCalledFunction() == &F)
            Calls.insert(CS);
      
      for (CallSite CS : Calls)
        // FIXME: We really shouldn't be able to fail to inline at this point!
        // We should do something to log or check the inline failures here.
        Changed |=
          InlineFunction(CS, IFI, /*CalleeAAR=*/nullptr, InsertLifetime);
      
      // Remember to try and delete this function afterward. This both avoids
      // re-walking the rest of the module and avoids dealing with any iterator
      // invalidation issues while deleting functions.
      InlinedFunctions.push_back(&F);
    }
    
    // Remove any live functions.
    erase_if(InlinedFunctions, [&](Function *F) {
                                 F->removeDeadConstantUsers();
                                 return !F->isDefTriviallyDead();
                               });
    
    // Delete the non-comdat ones from the module and also from our vector.
    auto NonComdatBegin = partition(
                                    InlinedFunctions, [&](Function *F) { return F->hasComdat(); });
    for (Function *F : make_range(NonComdatBegin, InlinedFunctions.end()))
      M.getFunctionList().erase(F);
    InlinedFunctions.erase(NonComdatBegin, InlinedFunctions.end());
    
    if (!InlinedFunctions.empty()) {
      // Now we just have the comdat functions. Filter out the ones whose comdats
      // are not actually dead.
      filterDeadComdatFunctions(M, InlinedFunctions);
      // The remaining functions are actually dead.
      for (Function *F : InlinedFunctions)
        M.getFunctionList().erase(F);
    }
  }
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

namespace {
 
 /// Inliner pass which only handles "always inline" functions.
 ///
 /// Unlike the \c AlwaysInlinerPass, this uses the more heavyweight \c Inliner
 /// base class to provide several facilities such as array alloca merging.
  class AlwaysInlinerLegacyPass : public LegacyInlinerBase {
 
 public:
   AlwaysInlinerLegacyPass() : LegacyInlinerBase(ID, /*InsertLifetime*/ true) {
     initializeAlwaysInlinerLegacyPassPass(*PassRegistry::getPassRegistry());
   }

 
   AlwaysInlinerLegacyPass(bool InsertLifetime)
       : LegacyInlinerBase(ID, InsertLifetime) {
     initializeAlwaysInlinerLegacyPassPass(*PassRegistry::getPassRegistry());
   }
 
   /// Main run interface method.  We override here to avoid calling skipSCC().
   bool runOnSCC(CallGraphSCC &SCC) override {
     return inlineCalls(SCC); }
 
   static char ID; // Pass identification, replacement for typeid
 
   InlineCost getInlineCost(CallSite CS) override;
   virtual StringRef getPassName() const override;
   using llvm::Pass::doFinalization;
   bool doFinalization(CallGraph &CG) override {
     return removeDeadFunctions(CG, /*AlwaysInlineOnly=*/true);
   }
 };
 }



 char AlwaysInlinerLegacyPass::ID = 0;
 INITIALIZE_PASS_BEGIN(AlwaysInlinerLegacyPass, "alwaysinline",
                       "Inliner for always_inline functions", false, false)
 INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
 INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
 INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
 INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
 INITIALIZE_PASS_END(AlwaysInlinerLegacyPass, "alwaysinline",
                     "Inliner for always_inline functions", false, false)
 
 Pass *llvm::createAlwaysInlinerLegacyPass(bool InsertLifetime) {
   return new AlwaysInlinerLegacyPass(InsertLifetime);
 }

/* 
 Get the inline cost for the always-inliner.
 
 The always inliner *only* handles functions which are marked with the
 attribute to force inlining. As such, it is dramatically simpler and avoids
 using the powerful (but expensive) inline cost analysis. Instead it uses
 a very simple and boring direct walk of the instructions looking for
 impossible-to-inline constructs.
 
 Note, it would be possible to go to some lengths to cache the information
 computed here, but as we only expect to do this for relatively few and
 small functions which have the explicit attribute to force inlining, it is
 likely not worth it in practice.
*/

std::set<Function*> knownDescendants;

std::set<Function*> getParents(Function* F) {
  std::set<Function*> parentSet;  
  for (User *U : F->users()) {
    if (auto CS = CallSite(U)) {
      Function* Caller = CS.getInstruction()->getParent()->getParent();
      parentSet.insert(Caller);
    }
  }
  return parentSet;
}

bool isKernelDescendant(Function* F) {
  if (F->getName().str().find("_libc_start_main")!= std::string::npos) {
    return false;
  }    
  std::set<Function*> parentSet=getParents(F);
  for (auto* Parent: parentSet) {
    if (isKernelFunction(*Parent) || knownDescendants.find(Parent)!=knownDescendants.end()) {
      knownDescendants.insert(F);
      return true;
    }
    else {
      return isKernelDescendant(Parent);
    }
  }
  return false;  
}

InlineCost AlwaysInlinerLegacyPass::getInlineCost(CallSite CS) {
 
  Function *Callee = CS.getCalledFunction();
  /*
  if(isKernelDescendant(Callee)) {
    errs() << "KERNEL DESCENDANT: " << Callee->getName() << "\n";
  }
  else {
    errs() << "NOT DESCENDANT: " << Callee->getName() << "\n";

  }
  */
   // Only inline direct calls to functions with always-inline attributes
   // that are viable for inlining. FIXME: We shouldn't even get here for
   // declarations.
  if (Callee && !Callee->isDeclaration() /*&& isInlineViable(*Callee)*/ && isKernelDescendant(Callee))
    return InlineCost::getAlways();
  else {
    //errs() << "really couldn't inline \n";
  }
  return InlineCost::getNever();
}

//char AlwaysInlinerLegacyPass::ID = 0;
static RegisterPass<AlwaysInlinerLegacyPass> rp("alwaysinline", "Pass to always inline", false, true);
StringRef AlwaysInlinerLegacyPass::getPassName() const {
  return "Always Inline Pass";
}
