// Pull in the control-flow graph class.
#include "ControlFlowGraph.h"

// For smart pointers (shared_ptr and unique_ptr);
#include <memory>

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/IR/CFG.h"
#include "llvm/PassSupport.h"

// Pull in LLVM-style RTTI for castings.
#include "llvm/Support/Casting.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

  char ControlFlowPass::ID = 0; // Default value.

  // Register this LLVM pass with the pass manager.
  RegisterPass<ControlFlowPass>
    registerCFG("cfg", "Construct the control-flow graph.");

  bool ControlFlowPass::runOnFunction(Function &fun) {
    // Does not perform any in-place modification of the IR, so unchanged.
    return false;
  }

  StringRef ControlFlowPass::getPassName() const {
    return "control-flow graph";
  }

  void ControlFlowPass::releaseMemory() {
    cflows.clear(); // "Free" all of the entries
  }

  void ControlFlowPass::getAnalysisUsage(AnalysisUsage &mgr) const {
    mgr.addRequired<PostDominatorTreeWrapperPass>(); // Add in the post-dominator analysis
    mgr.setPreservesAll(); // Retain all of the original dependencies
  }

  const Graph<BasicBlock> &ControlFlowPass::getGraph() const {
    return cflows;
  }

  std::stack<const DomTreeNode*> traverse(const PostDominatorTree &tree) {
    // Keep track of the parents left while traversing up
    std::stack<const DomTreeNode*> parentNodes;
    parentNodes.push(tree.getRootNode());
    // Store the result of the visited nodes
    std::stack<const DomTreeNode*> visitedNodes;

    // Iterate through the tree
    while (!parentNodes.empty()) {
      auto node = parentNodes.top(); // Get current node
      parentNodes.pop();

      // Enforce only one exit point for each basic block
      // If multiple, will eventually be popped by other iterations of the loop
      if (node->getBlock()) {
        visitedNodes.push(node);
      }

      // Push depth-first to get to the next level of the parent tree
      for (auto &block : *node) {
        parentNodes.push(block);
      }
    }

    // Finally, return all of the visited nodes
    return visitedNodes;
  }

  std::map<BasicBlock*, std::set<BasicBlock*>>
    getFrontier(const PostDominatorTree &tree, std::stack<DomTreeNode*> &&visitedNodes) {
    // Store the result of the mapped frontier
    std::map<BasicBlock*, std::set<BasicBlock*>> frontier;

    // Get the current "top" node on the visited stack
    auto node = visitedNodes.top();
    visitedNodes.pop();

    // Get the basic block corresponding to the node
    BasicBlock *block = node->getBlock();

    // Loop over all of the predecessors of the basic block
    for (const auto &pred : predecessors(block)) {
      // Add pred to the frontier if node is NOT its immediate post-dominator
      if (node != tree.getNode(pred)->getIDom()) {
        frontier[block].insert(pred);
      }
    }

    // Loop over all of the post-dominated nodes for the current node
    for (auto &pdnode : *node) {
      // Get the child of the post-dominated node
      BasicBlock *child = pdnode->getBlock();

      // Loop over all of the frontier blocks, conditioning on post-domination as above
      for (const auto &fblock : frontier[child]) {
        if (node != tree.getNode(fblock)->getIDom()) {
          frontier[block].insert(fblock);
        }
      }
    }

    // Finally, return the mapped frontier
    return frontier;
  }

}