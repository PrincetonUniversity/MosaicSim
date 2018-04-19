// Pull in the data-dependence pass class.
#include "passes/DataDependencePass.h"

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

  DataDependencePass::DataDependencePass()
    : FunctionPass(ID) {

  }

  DataDependencePass::~DataDependencePass() {
    for (auto &entry: graph) {
      delete entry.first;
    }
  }

  char DataDependencePass::ID = 0;

  // Register this LLVM pass with the pass manager.
  RegisterPass<DataDependencePass>
    registerDDG("ddg", "Construct the data-dependence graph.");

  bool DataDependencePass::runOnFunction(Function &fun) {
    // Create nodes.
    for (auto &basicBlock : fun) {
      for (auto &inst : basicBlock) {
        const auto &instNode = new InstructionNode(&inst);
        graph.addNode(instNode);
      }
    }

    // Fill in the edges via adjacency determination. MUST be done second, as
    // the dependencies in the checks below can only occur after all nodes have
    // been created via the process above.
    for (auto &basicBlock : fun) {
      for (auto &inst : basicBlock) {
        const auto &instNode = new InstructionNode(&inst);
        for (const auto &user : inst.users()) {
          // General declaration to make life easier.
          const BaseNode *adjNode;
          // Adjacent instruction, if the RTTI-cast passes.
          if (auto adjInst = dyn_cast<Instruction>(user)) {
            adjNode = new InstructionNode(adjInst);
          } else if (auto adjCon = dyn_cast<Constant>(user)) {
            adjNode = new ConstantNode(adjCon);
          } else if (auto adjOp = dyn_cast<Operator>(user)) {
            adjNode = new OperatorNode(adjOp);
          } else {
            // assert(false)
            adjNode = nullptr; // No way to express this node type
          }
          // Regardless of the dynamic type, just add it to the mapping
          graph.addEdge(instNode, adjNode);
        }
      }
    }

    return false;
  }

  StringRef DataDependencePass::getPassName() const {
    return "data-dependence graph";
  }

  void DataDependencePass::releaseMemory() {
    graph.clear();
  }

  void DataDependencePass::getAnalysisUsage(AnalysisUsage &mgr) const {
    // Analysis pass: No transformations on the program IR.
    mgr.setPreservesAll();
  }

  const Graph<const BaseNode> DataDependencePass::getGraph() const {
    return graph;
  }

}