#ifndef APOLLO_GRAPHS_INSTNODE
#define APOLLO_GRAPHS_INSTNODE

// Pull in the other node classes needed.
#include "BaseNode.h"

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

// An InstructionNode is a node representing an LLVM instruction.
class InstructionNode : public BaseNode {
public:
  /* Constructor for instruction nodes.
   *
   * Override: Just call the default BaseNode's constructor.
   */
  InstructionNode();

  /* Destructor for instruction nodes.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~InstructionNode() override { }

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

  /* [classof] returns true if the dynamic type of [n] is InstructionNode and
   *   returns false otherwise. Necessary for LLVM-style RTTI support.
   *     [n]: A node of static type BaseNode and a to-be-determined dynamic type.
   */
  static bool classof(const BaseNode *n) {
    return n->getType() == Instruction;
  }

};

}

#endif
