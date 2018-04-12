// Pull in the control-flow graph class.
#include "ControlFlowGraph.h"

// Pull in some standard data structures.
#include <map>
#include <set>
#include <stack>
#include <vector>

// For smart pointers (shared_ptr and unique_ptr);
#include <memory>

// For pairs.
#include <utility>

// Pull in various LLVM structures necessary for writing the pass.
#include "llvm/Analysis/PostDominators.h"
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
  RegisterPass<ControlFlowGraph>
    registerGraph("cfg", "Construct the control-flow graph.");

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

  void ControlFlowPass::getAnalysisUsage(AnalysisUsage &info) const {
    info.setPreservesAll(); // Retain all of the original dependencies
    info.addRequired<PostDominatorTree>(); // Add in the post-dominator analysis
  }

  const Graph<Instruction> &ControlFlowPass::getGraph() const {
    return cflows;
  }

  std::stack<DomTreeNode*> traverse(const PostDominatorTree &tree) const {
    // Keep track of the parents left while traversing up
    std::stack<DomTreeNode*> parentNodes(tree.getRootNode());
    return NULL;
  }

  std::map<BasicBlock*, set<BasicBlock*>>
    getFrontier(const PostDominatorTree &tree, std::stack<DomTreeNode*> &&visitedNodes) const {
    return NULL;
  }

}