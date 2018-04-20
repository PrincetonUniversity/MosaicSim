#ifndef APOLLO_VISITORS_VISUALVISITOR
#define APOLLO_VISITORS_VISUALVISITOR

// Pull in the default visitor behaviors.
#include "Visitor.h"

// Shared namespace within the project.
namespace apollo {

// Use the Visitor pattern to operate over our custom graph types.
class VisualizationVisitor : public Visitor {
public:
  /* Constructor for visualization Visitors. Saves the name of the program.
   *     [name]: The name of the program being run.
   *
   * Override: Construct a visualization Visitor with a source program's name.
   */
  VisualizationVisitor(const std::string name);

  /* Destructor for visualization Visitors.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~VisualizationVisitor() override;

  /* [visit] performs a stateful action on the top-level graph [g].
   *   Returns nothing.
   *     [g]: A graph to visit.
   *
   * Override: Visualize the top-level dependence graph.
   */
  virtual void visit(Graph<const BaseNode> g) override;

  /* [visit] performs a stateful action on the constant node [n] in a graph.
   *   Returns nothing.
   *     [n]: A constant to visit.
   *
   * Override: Visualize constants in the dependence graph.
   */
  virtual void visit(ConstantNode *n) override;

  /* [visit] performs a stateful action on the instruction node [n] in a graph.
   *   Returns nothing.
   *     [n]: An instruction to visit.
   *
   * Override: Visualize instructions in the dependence graph.
   */
  virtual void visit(InstructionNode *n) override;

  /* [visit] performs a stateful action on the operator node [n] in a graph.
   *   Returns nothing.
   *     [n]: An operator to visit.
   *
   * Override: Visualize operators in the dependence graph.
   */
  virtual void visit(OperatorNode *n) override;

  /* [visit] performs a stateful action on the basic block node [n] in a graph.
   *   Returns nothing.
   *     [n]: A basic block to visit.
   *
   * Override: Visualize basic blocks in the dependence graph.
   */
  virtual void visit(BasicBlockNode *n) override;

private:
  // Save the name of the program, to be used when outputting a DOT file.
  const std::string name;
};

}

#endif
