#ifndef APOLLO_GRAPHS_OPNODE
#define APOLLO_GRAPHS_OPNODE

// Pull in the other node classes needed.
#include "BaseNode.h"

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Operator.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// All the types of visitors that are allowed to exist in our custom graph types.
// Necessary for the visit() and accept() functions when using the recursive
// Visitor pattern in node classes.
class Visitor;
class DependenceVisitor;

// An OperatorNode is a node representing an LLVM operator (not an instruction).
class OperatorNode : public BaseNode {
public:
  /* Constructor for operator nodes.
   *     [op]: An LLVM operator (not an operator within an instruction).
   *
   * Override: A simple wrapper around the provided parameter.
   */
  OperatorNode(Operator *op);

  /* Destructor for operator nodes.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~OperatorNode() override { }

  /* Returns the operator around which this node wraps.
   *
   * Default: Non-overridable.
   */
  const Operator *getOperator() const {
    return op;
  }

  /* [accept] records actions from the generic visitor [v].
   *   Returns nothing.
   *     [v]: A generic visitor that operates on all types of graphs.
   *
   * Override: Accept generic visitors, but only to operators.
   */
  virtual void accept(Visitor &v) override;

  /* [accept] records actions from the overall-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on dependence graphs.
   *
   * Override: Accept overall-dependence visitors, but only to operators.
   */
  virtual void accept(DependenceVisitor &v) override;

  /* [classof] returns true if the dynamic type of [n] is OperatorNode and
   *   returns false otherwise. Necessary for LLVM-style RTTI support.
   *     [n]: A node of static type BaseNode and a to-be-determined dynamic type.
   */
  static bool classof(const BaseNode *n) {
    return n->getType() == ApolloOperator;
  }

private:
  // The operator around which this node wraps.
  const Operator *op;
};

}

#endif
