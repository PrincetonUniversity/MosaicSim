// Pull in the function prototypes.
#include "graphs/InstructionNode.h"

// Pull in the various visitor classes.
#include "visitors/Visitors.h"

// Shared namespace within the project.
using namespace apollo;

InstructionNode::InstructionNode(Instruction *inst)
  : BaseNode(ApolloInstruction), inst(inst) { }