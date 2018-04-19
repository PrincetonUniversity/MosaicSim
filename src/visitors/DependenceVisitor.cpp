// Pull in the dependence visitor behaviors.
#include "visitors/DependenceVisitor.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

void DependenceVisitor::visit(Graph<const BaseNode> *g) {
  return;
}

void DependenceVisitor::visit(ConstantNode *n) {
  return;
}

void DependenceVisitor::visit(InstructionNode *n) {
  return;
}

void DependenceVisitor::visit(OperatorNode *n) {
  return;
}

void DependenceVisitor::visit(BasicBlockNode *n) {
  return;
}