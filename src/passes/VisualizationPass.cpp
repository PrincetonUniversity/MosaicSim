// Pull in the visualization pass class and prerequisite pass classes.
#include "passes/VisualizationPass.h"
#include "passes/ProgramPass.h"

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"

// Pull in the various visitor classes.
#include "visitors/Visitors.h"

// Pull in the appropriate node classes.
#include "graphs/Node.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

VisualizationPass::VisualizationPass()
  : FunctionPass(ID) { }

VisualizationPass::~VisualizationPass() {
  for (auto &entry: graph) {
    delete entry.first;
  }
}

char VisualizationPass::ID = 0;

// Register this LLVM pass with the pass manager.
RegisterPass<VisualizationPass>
  registerVisualizer("viz", "Visualize the program's dependence graph.");

bool VisualizationPass::runOnFunction(Function &fun) {
  // "Constructor" by pulling in the information from earlier passes.
  graph = Pass::getAnalysis<ProgramPass>(fun).getGraph();

  // Get the name so that it can be passed into the visualization file name
  auto name = (fun.getName() + "-"
            +  fun.getParent()->getSourceFileName()).str();

  // Visit each of the nodes in the graph using a visualization algorithm
  VisualizationVisitor vv(name);
  vv.visit(graph);

  return false;
}

void VisualizationPass::releaseMemory() {
  graph.clear();
}

void VisualizationPass::getAnalysisUsage(AnalysisUsage &mgr) const {
  // Pull in earlier/prerequisite passes.
  mgr.addRequiredTransitive<ProgramPass>();

  // Analysis pass: No transformations on the program IR.
  mgr.setPreservesAll();
}