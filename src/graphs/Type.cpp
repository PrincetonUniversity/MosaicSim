// Pull in standard library for strings.
#include <string>

// For "assert false" defaults in switch statements.
#include <assert.h>

// Pull in the function prototypes.
#include "graphs/Type.h"

// Shared namespace within the project.
using namespace apollo;

std::string Type::toString(NodeType t) {
  // Better than chaining together if-then-else statements.
  switch (t) {
    case ApolloConstant:
      return "Constant";
    case ApolloInstruction:
      return "Instruction";
    case ApolloOperator:
      return "Operator";
    case ApolloBasicBlock:
      return "BasicBlock";
    default:
      assert(false);
  }
}