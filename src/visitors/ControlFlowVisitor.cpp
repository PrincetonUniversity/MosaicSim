// Pull in the control-flow visitor behaviors.
#include "visitors/ControlFlowVisitor.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

void ControlFlowVisitor::visit(Graph<const BaseNode> *g) {
  return;
}

void ControlFlowVisitor::visit(ConstantNode *n) {
  return;
}

void ControlFlowVisitor::visit(InstructionNode *n) {
  return;
}

void ControlFlowVisitor::visit(OperatorNode *n) {
  return;
}

void ControlFlowVisitor::visit(BasicBlockNode *n) {
  return;
}