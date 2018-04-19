// Pull in the control-flow pass class.
#include "passes/ControlFlowPass.h"

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/PassSupport.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

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
    return false;
  }

  StringRef ControlFlowPass::getPassName() const {
    return "control-flow graph";
  }

  void ControlFlowPass::releaseMemory() {
    graph.clear();
  }

  void ControlFlowPass::getAnalysisUsage(AnalysisUsage &mgr) const {
    // Analysis pass: No transformations on the program IR.
    mgr.setPreservesAll();
  }

  const Graph<const BaseNode> ControlFlowPass::getGraph() const {
    return graph;
  }

}