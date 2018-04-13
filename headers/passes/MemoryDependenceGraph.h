#ifndef APOLLO_PASSES_MEMDEPGRAPH
#define APOLLO_PASSES_MEMDEPGRAPH

// Pull in various LLVM structures necessary for writing the signatures.
#include "llvm/Pass.h"

// Pull in the base graph class.
#include "graphs/Graph.h"

// Avoid having to preface LLVM class names.
using namespace llvm;

// Shared namespace within the project.
namespace apollo {

// Use a pass over functions in the LLVM IR to construct a control-flow graph.
class MemoryDependencePass : public FunctionPass {

};

}

#endif