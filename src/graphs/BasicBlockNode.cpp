// Pull in the function prototypes.
#include "graphs/BasicBlockNode.h"

// Shared namespace within the project.
using namespace apollo;

BasicBlockNode::BasicBlockNode(BasicBlock *block)
  : BaseNode(ApolloBasicBlock), block(block) { }
