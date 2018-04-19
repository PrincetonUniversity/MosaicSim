#ifndef APOLLO_PASSES_VISUALPASS
#define APOLLO_PASSES_VISUALPASS

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/Pass.h"

// Pull in the all-encompassing graph and node classes.
#include "graphs/Graph.h"
#include "graphs/Node.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// Use a pass over programs in the LLVM IR to visualize an overall-dependence graph.
class VisualizationPass : public ModulePass {
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
  ~VisualizationPass();

  /* [runOnModule] is called on the overall module [mdl] present in the original
   *   program's IR. It statically-analyzes the program visualize the resulting
   *   overall dependency graph. Returns true if the pass modifies the IR in any
   *   way, and returns false otherwise.
   *     [mdl]: The module on which to run this pass.
   *
   * Requires: [mdl] is present in the original program's IR.
   */
  bool runOnModule(Module &mdl) override;

  /* [getPassName] returns a string specifying a customized name of the pass. */
  StringRef getPassName() const override;

  /* [releaseMemory] frees the pass in the statistics calculation and ends its
   *   lifetime from the perspective of usage analyses.
   */
  void releaseMemory() override;

  /* [getAnalysisUsage] fills [mgr] with the appropriate metadata on whether this
   *   pass analyzes or transforms the program IR.
   *     [mgr]: The pass state manager that tracks pass usages over time.
   */
  void getAnalysisUsage(AnalysisUsage &mgr) const override;

private:
  // Internal graph state, defined at the most general granularity due to the
  // heterogeneity of structures across the various passes.
  Graph<const BaseNode> graph;
};

}

#endif