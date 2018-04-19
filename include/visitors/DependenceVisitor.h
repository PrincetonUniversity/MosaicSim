#ifndef APOLLO_VISITORS_DEPVISITOR
#define APOLLO_VISITORS_DEPVISITOR

// Pull in the default visitor behaviors.
#include "Visitor.h"

// Shared namespace within the project.
namespace apollo {

// Use the Visitor pattern to operate over our custom graph types.
class DependenceVisitor : public Visitor {
public:
  /* Destructor for dependence Visitors.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~DependenceVisitor() { }

  /* [visit] performs a stateful action on the top-level graph [g].
   *   Returns nothing.
   *     [g]: A graph to visit.
   *
   * Override: Visit the top-level dependence graph.
   */
  virtual void visit(Graph<const BaseNode> *g) override;

  /* [visit] performs a stateful action on the constant node [n] in a graph.
   *   Returns nothing.
   *     [n]: A constant to visit.
   *
   * Override: Visit constants in the dependence graph.
   */
  virtual void visit(ConstantNode *n) override;

  /* [visit] performs a stateful action on the instruction node [n] in a graph.
   *   Returns nothing.
   *     [n]: An instruction to visit.
   *
   * Override: Visit instructions in the dependence graph.
   */
  virtual void visit(InstructionNode *n) override;

  /* [visit] performs a stateful action on the operator node [n] in a graph.
   *   Returns nothing.
   *     [n]: An operator to visit.
   *
   * Override: Visit operators in the dependence graph.
   */
  virtual void visit(OperatorNode *n) override;

  /* [visit] performs a stateful action on the basic block node [n] in a graph.
   *   Returns nothing.
   *     [n]: A basic block to visit.
   *
   * Override: Visit basic blocks in the dependence graph.
   */
  virtual void visit(BasicBlockNode *n) override;
};

}

#endif
