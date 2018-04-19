// Pull in the function prototypes.
#include "graphs/BasicBlockNode.h"

// Pull in the various visitor classes.
#include "visitors/Visitors.h"

// Shared namespace within the project.
using namespace apollo;

BasicBlockNode::BasicBlockNode(BasicBlock *block)
  : BaseNode(ApolloBasicBlock), block(block) { }

void BasicBlockNode::accept(Visitor &v) {
  v.visit(this);
}

void BasicBlockNode::accept(DependenceVisitor &v) {
  v.visit(this);
}