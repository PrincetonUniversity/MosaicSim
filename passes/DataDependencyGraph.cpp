// Pull in the data-dependency graph class.
#include "DataDependencyGraph.h"

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/PassSupport.h"
#include "llvm/IR/User.h"

// Pull in LLVM-style RTTI for castings.
#include "llvm/Support/Casting.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

  char DataDependencyPass::ID = 0; // Default value.

  // Register this LLVM pass with the pass manager.
  RegisterPass<DataDependencyPass>
    registerDDG("ddg", "Construct the data-dependency graph.");

  bool DataDependencyPass::runOnFunction(Function &fun) {
    // Create nodes.
    for (auto &basicBlock : fun) {
      for (auto &inst : basicBlock) {
        ddeps.addVertex(&inst);
      }
    }

    // Fill in the edges via adjacency determination. MUST be done second, as
    // the dependencies in the checks below can only occur after all nodes have
    // been created via the process above.
    for (auto &basicBlock : fun) {
      for (auto &inst : basicBlock) {
        for (const auto &user : inst.users()) {
          // Adjacent instruction, if the RTTI-cast passes.
          if (auto adjInst = dyn_cast<Instruction>(user)) {
            ddeps.addEdge(&inst, adjInst); // Add it to the mapping
          }
        }
      }
    }

    return false;
  }

  StringRef DataDependencyPass::getPassName() const {
    return "data-dependency graph";
  }

  void DataDependencyPass::releaseMemory() {
    ddeps.clear(); // "Free" all of the entries
  }

  void DataDependencyPass::getAnalysisUsage(AnalysisUsage &mgr) const {
    mgr.setPreservesAll(); // No input transformation
  }

  const Graph<Instruction> &DataDependencyPass::getGraph() const {
    return ddeps;
  }

}