// Pull in file-handling stream files.
#include <fstream>

// Pull in the dependence visitor behaviors.
#include "visitors/VisualizationVisitor.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

VisualizationVisitor::VisualizationVisitor(const std::string name)
  : name(name) { }

VisualizationVisitor::~VisualizationVisitor() { }

void VisualizationVisitor::visit(Graph<const BaseNode> *g) {
  // Create a stream through which to write to the visualization file
  std::ofstream fout;

  // Create metadata for the file to open
  const char *file = (name + ".dot").c_str();
  auto mode = std::ofstream::out | std::ofstream::app; // write, append
  
  // Open a DOT file with the appropriate name based on the saved information
  fout.open(file, mode);

  // TODO: Do proper logic to recursively visit via calls to accept()

  // Temporary: Just print something to see if it works???
  fout << "digraph temp {\n";
  fout << "  rankdir=\"LR\"\n";
  fout << "  node [shape=\"rectangle\"]\n";
  fout << "  0 -> 1\n";
  fout << "}\n";

  // Close the stream handler
  fout.close();
  return;
}

void VisualizationVisitor::visit(ConstantNode *n) {
  return;
}

void VisualizationVisitor::visit(InstructionNode *n) {
  return;
}

void VisualizationVisitor::visit(OperatorNode *n) {
  return;
}

void VisualizationVisitor::visit(BasicBlockNode *n) {
  return;
}