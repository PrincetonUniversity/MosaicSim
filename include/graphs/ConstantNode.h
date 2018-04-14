#ifndef APOLLO_GRAPHS_CONSTNODE
#define APOLLO_GRAPHS_CONSTNODE

// Pull in the other node classes needed.
#include "BaseNode.h"

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Constant.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// All the types of visitors that are allowed to exist in our custom graph types.
// Necessary for the visit() and accept() functions when using the recursive
// Visitor pattern in node classes.
class Visitor;
class DataDependenceVisitor;
class ControlFlowVisitor;
class MemoryDependenceVisitor;
class ProgramDependenceVisitor;

// A ConstantNode is a node representing a constant in an LLVM instruction.
class ConstantNode : public BaseNode {
public:
  /* Constructor for constant nodes.
   *     [con]: A constant in an LLVM instruction.
   *
   * Override: A simple wrapper around the provided parameter.
   */
  ConstantNode(Constant *con);

  /* Destructor for constant nodes.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~ConstantNode() override { }

  /* Returns the constant around which this node wraps.
   *
   * Default: Non-overridable.
   */
  const Constant *getConstant() const {
    return con;
  }

  /* [accept] records actions from the generic visitor [v].
   *   Returns nothing.
   *     [v]: A generic visitor that operates on all types of graphs.
   *
   * Override: TODO.
   */
  virtual void accept(Visitor &v) override;

  /* [accept] records actions from the data-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on data-dependence graphs.
   *
   * Override: TODO.
   */
  virtual void accept(DataDependenceVisitor &v) override;

  /* [accept] records actions from the control-flow visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on control-flow graphs.
   *
   * Override: TODO.
   */
  virtual void accept(ControlFlowVisitor &v) override;

  /* [accept] records actions from the memory-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on memory-dependence graphs.
   *
   * Override: TODO.
   */
  virtual void accept(MemoryDependenceVisitor &v) override;

  /* [accept] records actions from the program-dependence visitor [v].
   *   Returns nothing.
   *     [v]: A visitor that only operates on program-dependence graphs.
   *
   * Override: TODO.
   */
  virtual void accept(ProgramDependenceVisitor &v) override;

  /* [classof] returns true if the dynamic type of [n] is ConstantNode and
   *   returns false otherwise. Necessary for LLVM-style RTTI support.
   *     [n]: A node of static type BaseNode and a to-be-determined dynamic type.
   */
  static bool classof(const BaseNode *n) {
    return n->getType() == ApolloConstant;
  }

private:
  // The constant around which this node wraps.
  const Constant *con;
};

}

#endif
