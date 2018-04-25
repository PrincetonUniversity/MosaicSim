#ifndef APOLLO_GRAPHS_TYPE
#define APOLLO_GRAPHS_TYPE

// Pull in standard library for strings.
#include <string>

// Shared namespace within the project.
namespace apollo {

// The underlying enumerated type that captures the information.
enum NodeType {
  ApolloConstant,
  ApolloInstruction,
  ApolloOperator,
  ApolloBasicBlock
};

// A Type is a dynamic type of a node in a graph.
class Type {
public:
  /* [toString] returns a string representation of the NodeType [t].
   *     [t]: The dynamic type of a node in a graph.
   */
  static std::string toString(NodeType t);
};

}

#endif
