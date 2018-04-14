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
    case Base:
      return "Base";
    case Graph:
      return "Graph";
    case Constant:
      return "Constant";
    case Instruction:
      return "Instruction";
    case Operator:
      return "Operator";
    case BasicBlock:
      return "BasicBlock";
    default:
      assert(false);
  }
}