// Pull in the data-dependence visitor behaviors.
#include "visitors/DataDependenceVisitor.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

void DataDependenceVisitor::visit(Graph<const BaseNode> *g) {
  return;
}

void DataDependenceVisitor::visit(ConstantNode *n) {
  return;
}

void DataDependenceVisitor::visit(InstructionNode *n) {
  return;
}

void DataDependenceVisitor::visit(OperatorNode *n) {
  return;
}

void DataDependenceVisitor::visit(BasicBlockNode *n) {
  return;
}