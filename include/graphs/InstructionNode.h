#ifndef APOLLO_GRAPHS_INSTNODE
#define APOLLO_GRAPHS_INSTNODE

// Pull in the other node classes needed.
#include "BaseNode.h"

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/IR/Instruction.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// All the types of visitors that are allowed to exist in our custom graph types.
// Necessary for the visit() and accept() functions when using the recursive
// Visitor pattern in node classes.
class VisualizationVisitor;

// An InstructionNode is a node representing an LLVM instruction.
class InstructionNode : public BaseNode {
public:
  /* Constructor for instruction nodes.
   *     [inst]: An LLVM instruction itself.
   *
   * Override: A simple wrapper around the provided parameter.
   */
  InstructionNode(Instruction *inst);

  /* Destructor for instruction nodes.
   *
   * Override: Use C++'s default destruction process.
   */
  virtual ~InstructionNode() override;

  /* Returns the instruction around which this node wraps.
   *
   * Default: Non-overridable.
   */
  const Instruction *getInstruction() const {
    return inst;
  }

  /* [accept] records actions from the visualization visitor [v].
   *   Returns nothing.
   *     [v]: A visualization visitor that operates on dependence graphs.
   *
   * Override: Accept visualization visitors, but only to instructions.
   */
  virtual void accept(VisualizationVisitor &v) override;

  /* [classof] returns true if the dynamic type of [n] is InstructionNode and
   *   returns false otherwise. Necessary for LLVM-style RTTI support.
   *     [n]: A node of static type BaseNode and a to-be-determined dynamic type.
   */
  static bool classof(const BaseNode *n) {
    return n->getType() == ApolloInstruction;
  }

private:
  // The instruction around which this node wraps.
  const Instruction *inst;
};

}

#endif
