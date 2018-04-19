// Pull in the memory-dependence pass class.
#include "passes/MemoryDependencePass.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

  MemoryDependencePass::~MemoryDependencePass() {
    for (auto &entry: graph) {
      delete entry.first;
    }
  }

  char MemoryDependencePass::ID = 0;

  // Register this LLVM pass with the pass manager.
  RegisterPass<MemoryDependencePass>
    registerMDG("mdg", "Construct the memory-dependence graph.");

  bool MemoryDependencePass::runOnFunction(Function &fun) {
    return false;
  }

  StringRef MemoryDependencePass::getPassName() const {
    return "memory-dependence graph";
  }

  void MemoryDependencePass::releaseMemory() {
    graph.clear();
  }

  void MemoryDependencePass::getAnalysisUsage(AnalysisUsage &mgr) const {
    // Analysis pass: No transformations on the program IR.
    mgr.setPreservesAll();
  }

  const Graph<const BaseNode> MemoryDependencePass::getGraph() const {
    return graph;
  }

}