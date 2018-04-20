// Pull in the function prototypes.
#include "graphs/BasicBlockNode.h"

// Pull in the various visitor classes.
#include "visitors/Visitors.h"

// Shared namespace within the project.
using namespace apollo;

BasicBlockNode::BasicBlockNode(BasicBlock *block)
  : BaseNode(ApolloBasicBlock), block(block) { }

BasicBlockNode::~BasicBlockNode()
  { }

void BasicBlockNode::accept(Visitor &v) {
  v.visit(this);
}

void BasicBlockNode::accept(VisualizationVisitor &v) {
  v.visit(this);
}