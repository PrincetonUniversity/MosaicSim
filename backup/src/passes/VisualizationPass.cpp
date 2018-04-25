// Pull in the visualization pass class and prerequisite pass classes.
#include "passes/VisualizationPass.h"

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Module.h"
#include "llvm/PassSupport.h"
#include "llvm/IR/User.h"

// Pull in the various visitor classes.
#include "visitors/VisualizationVisitor.h"

// Pull in the appropriate node classes.
#include "graphs/BaseNode.h"
#include "graphs/ConstantNode.h"
#include "graphs/InstructionNode.h"
#include "graphs/OperatorNode.h"
#include "graphs/BasicBlockNode.h"

// Pull in LLVM-style RTTI for castings.
#include "llvm/Support/Casting.h"

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
  // Analysis pass: No transformations on the program IR.
  mgr.setPreservesAll();
}