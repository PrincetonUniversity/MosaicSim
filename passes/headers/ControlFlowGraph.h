#ifndef APOLLO_CTRL_FLOW_GRAPH
#define APOLLO_CTRL_FLOW_GRAPH

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"

// Pull in the base graph class.
#include "Graph.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// Use a pass over functions in the LLVM IR to construct a control-flow graph.
class ControlFlowPass : public FunctionPass {
public:
  // Identifier for this pass.
  static char ID;

  // Simple constructor that just invokes the parent constructor by default.
  ControlFlowPass() : FunctionPass(ID) { }

  /* [runOnFunction] is called on every function [fun] present in the original
   *   program's IR. It statically-analyzes the program structure to compress a
   *   graph in which edges represent branching paths in the code. No other
   *   relationships are captured in this single pass. Returns true if the pass
   *   modifies the IR in any way, and returns false otherwise.
   *     [fun]: The function on which to run this pass.
   *
   * Requires: [fun] is present in the original program's IR.
   */
  bool runOnFunction(Function &fun) override;

  /* [getPassName] returns a string specifying a customized name of the pass.
   */
  StringRef getPassName() const override;

  /* [releaseMemory] frees the pass in the statistics calculation and ends its
   *   lifetime from the perspective of usage analyses.
   */
  void releaseMemory() override;

  /* [getAnalysisUsage] fills [info] with the list of passes that the current
   *   pass depends on. In this case, the control-flow graph will rely on an
   *   existing pass that constructs a standard post-dominator tree on the IR.
   *     [info]: The pass state manager that tracks pass usages over time.
   */
  void getAnalysisUsage(AnalysisUsage &info) const override;

  /* [getGraph] returns the control-flow graph in an unmodifiable form.
   */
  const Graph<BasicBlock> &getGraph() const;

private:
  // Control-flow graph, defined at the basic block-level granularity.
  Graph<BasicBlock> cflows;

  /* [traverse] uses [tree] to construct a stack of nodes corresponding to a
   *   bottom-up version of a depth-first traversal on the post-dominator tree.
   *   The traversal is a reverse post-ordering on the nodes (due to the stack).
   *     [tree]: The post-dominator tree for the IR.
   */
  std::stack<DomTreeNode*> traverse(const PostDominatorTree &tree) const;

  /* [getFrontier] uses [tree] and [visitedNodes] to construct the post-dominator
   *   frontier as seen in a standard compiler techniques class. The frontier
   *   consists of basic blocks which are mapped to the set of basic blocks that
   *   they can reach directly.
   *     [tree]: The post-dominator tree for the IR.
   *     [visitedNodes]: The stack of nodes corresponding to a bottom-up traversal
   *                     of the post-dominator tree [tree].
   */
  std::map<BasicBlock*, set<BasicBlock*>>
    getFrontier(const PostDominatorTree &tree, std::stack<DomTreeNode*> &&visitedNodes) const;
};

}

#endif