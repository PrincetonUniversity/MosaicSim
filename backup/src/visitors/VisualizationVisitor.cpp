// Pull in file-handling stream files.
#include <fstream>

// Pull in some standard data structures.
#include <map>
#include <set>

// Pull in the dependence visitor behaviors.
#include "visitors/VisualizationVisitor.h"

// Pull in the actual node classes.
#include "graphs/BaseNode.h"
#include "graphs/ConstantNode.h"
#include "graphs/InstructionNode.h"
#include "graphs/OperatorNode.h"
#include "graphs/BasicBlockNode.h"

// Shared namespace within the project.
using namespace apollo;

// See header file for comments.

VisualizationVisitor::VisualizationVisitor(const std::string name)
  : name(name) { }

VisualizationVisitor::~VisualizationVisitor() { }

void VisualizationVisitor::visit(Graph<const BaseNode> g) {

  // Name mappings for the graph nodes.
  std::map<const BaseNode*, int> nums;

  // Cycle through a series of numbers.
  int curr = 0;

  // Create numerical mappings for each of the nodes.
  for (const auto &entry : g) {
    auto node  = entry.first;
    nums[node] = curr;
    curr++;
  }

  // Create a stream through which to write to the visualization file
  std::ofstream fout;

  // Create metadata for the file to open
  const char *file = (name + ".dot").c_str();
  auto mode = std::ofstream::out | std::ofstream::app; // write, append
  
  // Open a DOT file with the appropriate name based on the saved information
  fout.open(file, mode);

  // Create the actual DOT structure based on the graph DSL.
  fout << "digraph temp {\n";
  fout << "  rankdir=\"LR\"\n";
  fout << "  node [shape=\"rectangle\"]\n";

  for (const auto &entry : g) {
    auto node = entry.first;
    auto adjs = entry.second;
    for (const auto &adj : adjs) {
      fout << "  " << node << " -> " << adj << "\n";
    }
  }

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