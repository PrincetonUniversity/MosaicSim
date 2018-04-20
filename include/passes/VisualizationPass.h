#ifndef APOLLO_PASSES_VISUALPASS
#define APOLLO_PASSES_VISUALPASS

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/Pass.h"

// Pull in the all-encompassing graph and node classes.
#include "graphs/Graph.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// All the types of nodes that are allowed to exist in our custom graph types.
// Based on the types of users allowed by LLVM, but with augmentations for our
// custom graph handling (e.g. "basic block nodes"). This allows for multiple
// types of graphs to utilize this interface cleanly.
class BaseNode;
class ConstantNode;
class InstructionNode;
class OperatorNode;
class BasicBlockNode;

// Use a pass over programs in the LLVM IR to visualize an overall-dependence graph.
class VisualizationPass : public FunctionPass {
public:
  // Identifier for this pass.
  static char ID;

  /* Simple constructor that just invokes the parent constructor by default and
   *   initializes the internal state variables.
   */
  VisualizationPass();

  /* Destructor that deletes the contents of the underlying graph (i.e. the
   *   internal state) by removing the nodes one by one.
   */
  virtual ~VisualizationPass() override;


  /* [runOnFunction] is called on every function [fun] present in the original
   *   program's IR. It statically-analyzes the program structure to visualize
   *   the overall dependency graph. Returns true if the pass modifies the IR in
   *   any way, and returns false otherwise.
   *     [fun]: The function on which to run this pass.
   *
   * Requires: [fun] is present in the original program's IR.
   */
  virtual bool runOnFunction(Function &fun) override;

  /* [releaseMemory] frees the pass in the statistics calculation and ends its
   *   lifetime from the perspective of usage analyses.
   */
  virtual void releaseMemory() override;

  /* [getAnalysisUsage] fills [mgr] with the appropriate metadata on whether this
   *   pass analyzes or transforms the program IR.
   *     [mgr]: The pass state manager that tracks pass usages over time.
   */
  virtual void getAnalysisUsage(AnalysisUsage &mgr) const override;

private:
  // Internal graph state, defined at the most general granularity due to the
  // heterogeneity of structures across the various passes.
  Graph<const BaseNode> graph;
};

}

#endif