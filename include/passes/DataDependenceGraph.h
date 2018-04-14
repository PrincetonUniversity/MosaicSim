#ifndef APOLLO_PASSES_DATADEPGRAPH
#define APOLLO_PASSES_DATADEPGRAPH

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"

// Pull in the base graph class.
#include "graphs/Graph.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// Use a pass over functions in the LLVM IR to construct a data-dependence graph.
class DataDependencePass : public FunctionPass {
public:
  // Identifier for this pass.
  static char ID;

  // Simple constructor that just invokes the parent constructor by default.
  DataDependencePass() : FunctionPass(ID) { }

  /* [runOnFunction] is called on every function [fun] present in the original
   *   program's IR. It statically-analyzes the program structure to compress a
   *   graph in which edges represent read-after-write dependencies. No other
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
   *   pass depends on. In this case, the pass does not transform the input at
   *   all by calling other passes.
   *     [mgr]: The pass state manager that tracks pass usages over time.
   */
  void getAnalysisUsage(AnalysisUsage &mgr) const override;

  /* [getGraph] returns the data-dependence graph in an unmodifiable form.
   */
  const Graph<Instruction> &getGraph() const;

private:
  // Data-dependence graph, defined at the instruction-level granularity.
  Graph<Instruction> ddeps;
};

}

#endif