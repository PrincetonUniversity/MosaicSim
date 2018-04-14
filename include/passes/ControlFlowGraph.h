#ifndef APOLLO_PASSES_CTRLFLOWGRAPH
#define APOLLO_PASSES_CTRLFLOWGRAPH

// Pull in some standard data structures.
#include <map>
#include <set>
#include <stack>

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Function.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"

// Pull in the all-encompassing graph and node classes.
#include "graphs/Graph.h"
#include "graphs/Node.h"

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

  // Destructor that deletes the contents of the underlying graph by removing
  // the nodes one by one.
  ~ControlFlowPass();

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

  /* [getAnalysisUsage] fills [mgr] with the list of passes that the current
   *   pass depends on. In this case, the control-flow graph will rely on an
   *   existing pass that constructs a standard post-dominator tree on the IR.
   *     [mgr]: The pass state manager that tracks pass usages over time.
   */
  void getAnalysisUsage(AnalysisUsage &mgr) const override;

  /* [getGraph] returns the control-flow graph in an unmodifiable form. */
  const Graph<const BaseNode> getGraph() const;

private:
  // Control-flow graph, defined at the basic block level of granularity.
  Graph<const BaseNode> cflows;

  /* [traverse] uses [tree] to construct a stack of nodes corresponding to a
   *   bottom-up version of a depth-first traversal on the post-dominator tree.
   *   The traversal is a reverse post-ordering on the nodes (due to the stack).
   *     [tree]: The post-dominator tree for the IR.
   */
  std::stack<DomTreeNode*> traverse(const PostDominatorTree &tree);

  /* [getFrontier] uses [tree] and [visitedNodes] to construct the post-dominator
   *   frontier as seen in a standard compiler techniques class. The frontier
   *   consists of basic blocks which are mapped to the set of basic blocks that
   *   they can reach directly.
   *     [tree]: The post-dominator tree for the IR.
   *     [visitedNodes]: The stack of nodes corresponding to a bottom-up traversal
   *                     of the post-dominator tree [tree].
   */
  const std::map<BasicBlock*, std::set<BasicBlock*>>
    getFrontier(const PostDominatorTree &tree, std::stack<DomTreeNode*> &&visitedNodes);
};

}

#endif