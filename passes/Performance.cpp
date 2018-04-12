#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Anonymous namespace (local to this file).
namespace {
  struct PerformancePass : public FunctionPass {
    // Link in the ID defined below.
    static char ID;

    // Very simple constructor that just invokes the superclass' constructor.
    PerformancePass() : FunctionPass(ID) { }

    // The transformation that is run on all functions of the input program.
    virtual bool runOnFunction(Function &function) {
      // The actual function is never modified.
      return false;
    }
  };
}

// This initial pass will have a simple identifier.
char PerformancePass::ID = 0;

// Register the pass with LLVM.
static RegisterPass<PerformancePass>
  X("estimate", "Analyze constraints in basic blocks to estimate performance.");