// Pull in the control-flow graph class.
#include "passes/ControlFlowGraph.h"

// For smart pointers (shared_ptr and unique_ptr);
#include <memory>

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/IR/CFG.h"
#include "llvm/PassSupport.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

  // See header file for comments.

  char ControlFlowPass::ID = 0; // Default value.

  // Register this LLVM pass with the pass manager.
  RegisterPass<ControlFlowPass>
    registerCFG("cfg", "Construct the control-flow graph.");

  std::stack<const DomTreeNode*> traverse(const PostDominatorTree &tree) {
    
    // Keep track of parents (since reversed) while traversing depth-first
    std::stack<const DomTreeNode*> parentNodes;
    parentNodes.push(tree.getRootNode());

    std::stack<const DomTreeNode*> visitedNodes;

    while (!parentNodes.empty()) {
      auto node = parentNodes.top(); // Get current node
      parentNodes.pop();

      // Enforce only one exit point for each basic block
      if (node->getBlock()) { // Multiple = eventually popped by other iterations
        visitedNodes.push(node);
      }

      // Push depth-first to get to the next level of the parent tree
      for (auto &block : *node) {
        parentNodes.push(block);
      }
    }

    return visitedNodes;
  }

  std::map<BasicBlock*, std::set<BasicBlock*>>
    getFrontier(const PostDominatorTree &tree, std::stack<DomTreeNode*> &&visitedNodes) {

    std::map<BasicBlock*, std::set<BasicBlock*>> frontier;

    while (!visitedNodes.empty()) {
      auto node = visitedNodes.top(); // Get current node
      visitedNodes.pop();

      BasicBlock *block = node->getBlock(); // Get current block

      // Loop over all of the predecessors of the basic block
      for (const auto &pred : predecessors(block)) {
        // Add pred to the frontier if node is NOT its immediate post-dominator
        if (node != tree.getNode(pred)->getIDom()) {
          frontier[block].insert(pred);
        }
      }

      // Loop over all of the post-dominated nodes for the current node
      for (auto &postDomNode : *node) {
        // Get the child of the post-dominated node
        BasicBlock *child = postDomNode->getBlock();

        // Loop over all of the frontier blocks, conditioning on post-domination
        for (const auto &frontBlock : frontier[child]) {
          if (node != tree.getNode(frontBlock)->getIDom()) {
            frontier[block].insert(frontBlock);
          }
        }
      }
    }

    return frontier;
  }

  bool ControlFlowPass::runOnFunction(Function &fun) {
    const auto &tree = Pass::getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    auto visitedNodes = traverse(tree);

    // Note: std::move to handle rvalue copying/perfect forwarding
    auto frontier = getFrontier(tree, std::move(visitedNodes)); 

    // Loop through the post-dominant frontier and create CFG nodes.
    for (auto &entry : frontier) {
      BasicBlock *block = entry.first;
      cflows.addVertex(block);
    }

    // Fill in the edges via the frontier. MUST be done second, as both nodes 
    // may not exist yet if this loop is done at the same time as the one above.
    for (auto &entry : frontier) {
      BasicBlock *sink = entry.first;

      // Loop over all of the sources and create an edge for each one
      for (auto &source : entry.second) {
        cflows.addEdge(source, sink);
      }
    }

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

}