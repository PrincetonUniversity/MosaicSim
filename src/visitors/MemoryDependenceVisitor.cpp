// Pull in the memory-dependence visitor behaviors.
#include "visitors/MemoryDependenceVisitor.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

void MemoryDependenceVisitor::visit(ConstantNode *n) {
  return;
}

void MemoryDependenceVisitor::visit(InstructionNode *n) {
  return;
}

void MemoryDependenceVisitor::visit(OperatorNode *n) {
  return;
}

void MemoryDependenceVisitor::visit(BasicBlockNode *n) {
  return;
}