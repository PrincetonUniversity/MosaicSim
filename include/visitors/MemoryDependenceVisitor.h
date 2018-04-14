#ifndef APOLLO_VISITORS_MEMDEPVISITOR
#define APOLLO_VISITORS_MEMDEPVISITOR

// Pull in the default visitor behaviors.
#include "Visitor.h"

// Shared namespace within the project.
namespace apollo {

// Use the Visitor pattern to operate over our custom graph types.
class MemoryDependenceVisitor : public Visitor {
public:
  /* Destructor for memory-dependence Visitors.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~MemoryDependenceVisitor() { }

  /* [visit] performs a stateful action on the constant node [n] in a graph.
   *   Returns nothing.
   *     [n]: A constant to visit.
   *
   * Override: TODO.
   */
  virtual void visit(ConstantNode *n) override;

  /* [visit] performs a stateful action on the instruction node [n] in a graph.
   *   Returns nothing.
   *     [n]: An instruction to visit.
   *
   * Override: TODO.
   */
  virtual void visit(InstructionNode *n) override;

  /* [visit] performs a stateful action on the operator node [n] in a graph.
   *   Returns nothing.
   *     [n]: An operator to visit.
   *
   * Override: TODO.
   */
  virtual void visit(OperatorNode *n) override;

  /* [visit] performs a stateful action on the basic block node [n] in a graph.
   *   Returns nothing.
   *     [n]: A basic block to visit.
   *
   * Override: TODO.
   */
  virtual void visit(BasicBlockNode *n) override;
};

}

#endif
