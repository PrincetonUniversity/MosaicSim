// Pull in the function prototypes.
#include "graphs/ConstantNode.h"

// Pull in the various visitor classes.
#include "visitors/VisualizationVisitor.h"

// Shared namespace within the project.
using namespace apollo;

ConstantNode::ConstantNode(Constant *con)
  : BaseNode(ApolloConstant), con(con) { }

ConstantNode::~ConstantNode()
  { }

void ConstantNode::accept(VisualizationVisitor &v) {
  v.visit(this);
}