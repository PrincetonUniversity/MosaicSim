#ifndef APOLLO_GRAPHS_BBNODE
#define APOLLO_GRAPHS_BBNODE

// Pull in the other node classes needed.
#include "BaseNode.h"
#include "Type.h"

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/BasicBlock.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// All the types of visitors that are allowed to exist in our custom graph types.
// Necessary for the visit() and accept() functions when using the recursive
// Visitor pattern in node classes.
class Visitor;
class DependenceVisitor;

// A BasicBlockNode is a node at the basic-block level of granularity, used in
// control-flow graphs as the type of each node. Sometimes inserted as "dummy"
// nodes to handle the "phi node" situation in control flow.
class BasicBlockNode : public BaseNode {
public:
  /* Constructor for basic block nodes.
   *     [block]: A basic block of many LLVM instructions.
   *
   * Override: A simple wrapper around the provided parameter.
   */
  BasicBlockNode(BasicBlock *block);

  /* Destructor for nodes at the basic-block level of granularity.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~BasicBlockNode() override { }

  /* Returns the basic block around which this node wraps.
   *
   * Default: Non-overridable.
   */
  const BasicBlock *getBasicBlock() const {
    return block;
  }

  /* [accept] records actions from the generic visitor [v].
   *   Returns nothing.
   *     [v]: A generic visitor that operates on all types of graphs.
   *
   * Override: Accept generic visitors, but only to basic blocks.
   */
  virtual void accept(Visitor &v) override;

  /* [accept] records actions from the overall-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on dependence graphs.
   *
   * Override: Accept overall-dependence visitors, but only to basic blocks.
   */
  virtual void accept(DependenceVisitor &v) override;

  /* [classof] returns true if the dynamic type of [n] is BasicBlockNode and
   *   returns false otherwise. Necessary for LLVM-style RTTI support.
   *     [n]: A node of static type BaseNode and a to-be-determined dynamic type.
   */
  static bool classof(const BaseNode *n) {
    return n->getType() == ApolloBasicBlock;
  }

private:
  // The basic block around which this node wraps.
  const BasicBlock *block;
};

}

#endif
