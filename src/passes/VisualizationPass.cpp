// Pull in the visualization pass class and prerequisite pass classes.
#include "passes/VisualizationPass.h"
#include "passes/ProgramPass.h"

// Pull in the various visitor classes.
#include "visitors/Visitors.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

  VisualizationPass::VisualizationPass()
    : ModulePass(ID) {
    auto passedGraph = Pass::getAnalysis<ProgramPass>().getGraph();
    graph = passedGraph;
  }

  VisualizationPass::~VisualizationPass() {
    for (auto &entry: graph) {
      delete entry.first;
    }
  }

  char VisualizationPass::ID = 0;

  // Register this LLVM pass with the pass manager.
  RegisterPass<VisualizationPass>
    registerVisualizer("viz", "Visualize the program's dependence graph.");

  bool VisualizationPass::runOnModule(Module &mdl) {
    // Visit each of the nodes in the graph using a visualization algorithm
    VisualizationVisitor vv;
    vv.visit(&graph);

    return false;
  }

  StringRef VisualizationPass::getPassName() const {
    return "dependence graph visualizer";
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

}