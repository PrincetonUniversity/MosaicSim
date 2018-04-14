#ifndef APOLLO_VISITORS_PROGDEPVISITOR
#define APOLLO_VISITORS_PROGDEPVISITOR

// Pull in the default visitor behaviors.
#include "Visitor.h"

// Shared namespace within the project.
namespace apollo {

// All the types of nodes that are allowed to exist in our custom graph types.
// Based on the types of users allowed by LLVM, but with augmentations for our
// custom graph handling (e.g. "basic block nodes"). This allows for multiple
// types of graphs to utilize this interface cleanly.
class BaseNode;
class GraphNode;
class ConstantNode;
class InstructionNode;
class OperatorNode;
class BasicBlockNode;

// Use the Visitor pattern to operate over our custom graph types.
class ProgramDependenceVisitor : public Visitor {
public:
  /* Destructor for program-dependence Visitors.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~ProgramDependenceVisitor() { }

  /* [visit] performs a stateful action on the top-level graph node [n].
   *   Returns nothing.
   *     [n]: A graph to visit.
   *
   * Override: TODO.
   */
  virtual void visit(GraphNode *n) override;

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
