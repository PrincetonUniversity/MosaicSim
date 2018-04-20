// Pull in the control-flow pass class and prerequisite pass classes.
#include "passes/ProgramPass.h"
#include "passes/MemoryDependencePass.h"

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/PassSupport.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

ProgramPass::ProgramPass()
  : FunctionPass(ID) {
  auto passedGraph = Pass::getAnalysis<MemoryDependencePass>().getGraph();
  graph = passedGraph;
}

ProgramPass::~ProgramPass() {
  for (auto &entry: graph) {
    delete entry.first;
  }
}

char ProgramPass::ID = 0;

// Register this LLVM pass with the pass manager.
RegisterPass<ProgramPass>
  registerPDG("pdg", "Construct the program's dependence graph.");

bool ProgramPass::runOnFunction(Function &fun) {
  return false;
}

StringRef ProgramPass::getPassName() const {
  return "dependence graph";
}

void ProgramPass::releaseMemory() {
  graph.clear();
}

void ProgramPass::getAnalysisUsage(AnalysisUsage &mgr) const {
  // Pull in earlier/prerequisite passes.
  mgr.addRequiredTransitive<MemoryDependencePass>();

  // Analysis pass: No transformations on the program IR.
  mgr.setPreservesAll();
}

const Graph<const BaseNode> ProgramPass::getGraph() const {
  return graph;
}