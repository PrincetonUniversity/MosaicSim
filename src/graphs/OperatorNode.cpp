// Pull in the function prototypes.
#include "graphs/OperatorNode.h"

// Pull in the various visitor classes.
#include "visitors/Visitors.h"

// Shared namespace within the project.
using namespace apollo;

OperatorNode::OperatorNode(Operator *op)
  : BaseNode(ApolloOperator), op(op) { }

void OperatorNode::accept(Visitor &v) {
  v.visit(this);
}

void OperatorNode::accept(VisualizationVisitor &v) {
  v.visit(this);
}