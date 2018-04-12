// Pull in the data dependency graph class.
#include "DataDependencyGraph.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

  char DataDependencyPass::ID = 0; // Default value.

  bool DataDependencyPass::runOnFunction(Function &F) {
    return false;
  }

  StringRef DataDependencyPass::getPassName() const {
    return "";
  }

  void DataDependencyPass::releaseMemory() {
    return;
  }

  void DataDependencyPass::getAnalysisUsage(AnalysisUsage &info) const {
    return;
  }

  const Graph<Instruction> &DataDependencyPass::getGraph() const {
    return ddeps;
  }

}