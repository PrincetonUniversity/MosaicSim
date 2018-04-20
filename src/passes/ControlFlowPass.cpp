// Pull in the control-flow pass class and prerequisite pass classes.
#include "passes/ControlFlowPass.h"
#include "passes/DataDependencePass.h"

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/PassSupport.h"

// Pull in the appropriate node classes.
#include "graphs/Node.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

ControlFlowPass::ControlFlowPass()
  : FunctionPass(ID) { }

ControlFlowPass::~ControlFlowPass() {
  for (auto &entry: graph) {
    delete entry.first;
  }
}

char ControlFlowPass::ID = 0;

// Register this LLVM pass with the pass manager.
RegisterPass<ControlFlowPass>
  registerCFG("cfg", "Construct the control-flow graph.");

bool ControlFlowPass::runOnFunction(Function &fun) {
  // "Constructor" by pulling in the information from earlier passes.
  graph = Pass::getAnalysis<DataDependencePass>().getGraph();
  return false;
}

void ControlFlowPass::releaseMemory() {
  graph.clear();
}

void ControlFlowPass::getAnalysisUsage(AnalysisUsage &mgr) const {
  // Pull in earlier/prerequisite passes.
  mgr.addRequiredTransitive<DataDependencePass>();

  // Analysis pass: No transformations on the program IR.
  mgr.setPreservesAll();
}

const Graph<const BaseNode> ControlFlowPass::getGraph() const {
  return graph;
}