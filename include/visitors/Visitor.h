#ifndef APOLLO_VISITORS_VISITOR
#define APOLLO_VISITORS_VISITOR

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
class Visitor {
public:
  /* Destructor for all Visitors.
   *
   * Default: Use C++'s default destruction process.
   */
  virtual ~Visitor() { }

  /* [visit] performs a stateful action on the top-level graph node [n].
   *   Returns nothing.
   *     [n]: A graph to visit.
   *
   * Default: Purely virtual. TODO: Fix.
   */
  virtual void visit(GraphNode *n) = 0;

  /* [visit] performs a stateful action on the constant node [n] in a graph.
   *   Returns nothing.
   *     [n]: A constant to visit.
   *
   * Default: Purely virtual. TODO: Fix.
   */
  virtual void visit(ConstantNode *n) = 0;

  /* [visit] performs a stateful action on the instruction node [n] in a graph.
   *   Returns nothing.
   *     [n]: An instruction to visit.
   *
   * Default: Purely virtual. TODO: Fix.
   */
  virtual void visit(InstructionNode *n) = 0;

  /* [visit] performs a stateful action on the operator node [n] in a graph.
   *   Returns nothing.
   *     [n]: An operator to visit.
   *
   * Default: Purely virtual. TODO: Fix.
   */
  virtual void visit(OperatorNode *n) = 0;

  /* [visit] performs a stateful action on the basic block node [n] in a graph.
   *   Returns nothing.
   *     [n]: A basic block to visit.
   *
   * Default: Purely virtual. TODO: Fix.
   */
  virtual void visit(BasicBlockNode *n) = 0;
};

}

#endif
