// Pull in the control-flow pass class and prerequisite pass classes.
#include "passes/MemoryDependencePass.h"
#include "passes/ControlFlowPass.h"

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/PassSupport.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

MemoryDependencePass::MemoryDependencePass()
  : FunctionPass(ID) { }

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
  // "Constructor" by pulling in the information from earlier passes.
  graph = Pass::getAnalysis<ControlFlowPass>().getGraph();
  return false;
}

StringRef MemoryDependencePass::getPassName() const {
  return "memory-dependence graph";
}

void MemoryDependencePass::releaseMemory() {
  graph.clear();
}

void MemoryDependencePass::getAnalysisUsage(AnalysisUsage &mgr) const {
  // Pull in earlier/prerequisite passes.
  mgr.addRequiredTransitive<ControlFlowPass>();

  // Analysis pass: No transformations on the program IR.
  mgr.setPreservesAll();
}

const Graph<const BaseNode> MemoryDependencePass::getGraph() const {
  return graph;
}