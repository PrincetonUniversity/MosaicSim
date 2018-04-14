// Pull in the program-dependence visitor behaviors.
#include "visitors/ProgramDependenceVisitor.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

void ProgramDependenceVisitor::visit(Graph<const BaseNode> *g) {
  return;
}

void ProgramDependenceVisitor::visit(ConstantNode *n) {
  return;
}

void ProgramDependenceVisitor::visit(InstructionNode *n) {
  return;
}

void ProgramDependenceVisitor::visit(OperatorNode *n) {
  return;
}

void ProgramDependenceVisitor::visit(BasicBlockNode *n) {
  return;
}