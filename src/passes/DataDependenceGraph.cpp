// Pull in the data-dependence graph class.
#include "passes/DataDependenceGraph.h"

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

  char DataDependencePass::ID = 0; // Default value.

  // Register this LLVM pass with the pass manager.
  RegisterPass<DataDependencePass>
    registerDDG("ddg", "Construct the data-dependence graph.");

  bool DataDependencePass::runOnFunction(Function &fun) {
    // Create nodes.
    for (auto &basicBlock : fun) {
      for (auto &inst : basicBlock) {
        const auto &instNode = new InstructionNode(&inst);
        ddeps.addNode(instNode);
      }
    }

    // Fill in the edges via adjacency determination. MUST be done second, as
    // the dependencies in the checks below can only occur after all nodes have
    // been created via the process above.
    for (auto &basicBlock : fun) {
      for (auto &inst : basicBlock) {
        const auto &instNode = new InstructionNode(&inst);
        for (const auto &user : inst.users()) {
          // Adjacent instruction, if the RTTI-cast passes.
          if (auto adjInst = dyn_cast<Instruction>(user)) {
            const auto &adjInstNode = new InstructionNode(adjInst);
            ddeps.addEdge(instNode, adjInstNode); // Add it to the mapping
          }
        }
      }
    }

    return false;
  }

  StringRef DataDependencePass::getPassName() const {
    return "data-dependence graph";
  }

  void DataDependencePass::releaseMemory() {
    ddeps.clear(); // "Free" all of the entries
  }

  void DataDependencePass::getAnalysisUsage(AnalysisUsage &mgr) const {
    mgr.setPreservesAll(); // No input transformation
  }

  const Graph<const BaseNode> DataDependencePass::getGraph() const {
    return ddeps;
  }

}