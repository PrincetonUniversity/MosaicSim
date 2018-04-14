#ifndef APOLLO_GRAPHS_GRAPH
#define APOLLO_GRAPHS_GRAPH

// Pull in some standard data structures.
#include <map>
#include <set>

// For pairs.
#include <utility>

// Shared namespace within the project.
namespace apollo {

// Base class for all dependency graphs, parameterized on a node type.
template <typename Node>
class Graph {
public:
  // Iterator on the actual nodes themselves.
  typedef typename std::map<const Node*, std::set<const Node*>>::iterator
    iterator;
  // Iterator on the adjacencies.
  typedef typename std::set<const Node*>::iterator
    adj_iterator;
  // Constant version of the iterator on actual nodes themselves.
  typedef typename std::map<const Node*, std::set<const Node*>>::const_iterator
    const_iterator;
  // Constant version of the iterator on the adjacencies.
  typedef typename std::set<const Node*>::const_iterator
    adj_const_iterator;


  /* [isEmpty] returns true if the graph is empty (i.e. it returns whether the
   *   graph contains no nodes).
   */
  bool isEmpty() {
    return nodes.empty();
  }

  /* [clear] clears the graph so that it contains no vertices or edges.
   */
  void clear() {
    nodes.clear();
  }

  /* [degree] returns the (out-)degree of the node [v] in the graph.
   *     [v]: The node whose degree is to be determined.
   *
   * Requires: [v] is present in the graph.
   */
  int degree(const Node *v) {
    return nodes.at(v).size();
  }

  /* [isAdjacent] returns true if and only if [v] is adjacent to [u] in the graph,
   *   i.e. it returns whether [v] is in the adjacency list of [u].
   *     [u]: The vertex to be tested against for adjacency.
   *     [v]: The vertex source to test.
   *
   * Requires: [u] and [v] are present in the graph.
   */
  bool isAdjacent(const Node *u, const Node *v) {
    return (nodes[u].find(v) != nodes[v].end());
  }

  /* [adj] returns the list of nodes that are adjacent to [v] in the graph, i.e.
   *   it returns the adjacency list of [v].
   *     [v]: The node whose adjacency list is to be determined.
   *
   * Requires: [v] is present in the graph.
   */
  const std::set<const Node*> adj(const Node *v) {
    return nodes.at(v);
  }

  /* [addVertex] adds the node [v] to the graph. It is initially not adjacent
   *   to any other node, i.e. its adjacency list is empty.
   *     [v]: The vertex to add.
   *
   * Requires: [v] is not already present in the graph.
   */
  void addVertex(const Node *v) {
    nodes[v];
  }

  /* [addVertex] deletes the node [v] from the graph.
   *     [v]: The vertex to remove.
   *
   * Requires: [v] is present in the graph.
   */
  void removeVertex(const Node *v) {
    // Loop through and remove all edge connections to this vertex.
    for (const auto &entry : nodes) {
      auto node = entry.first;
      auto adjs = entry.second;
      adjs.erase(v);
    }
    // Finally, remove this vertex itself.
    nodes.erase(v);
  }

  /* [addEdge] adds a directed edge from [u] to [v] in the graph. Thus, [v] is
   *   added to the end of the adjacency list of [u].
   *     [u]: The starting vertex of the directed edge.
   *     [v]: The ending vertex of the directed edge.
   *
   * Requires: [u] and [v] are present in the graph.
   */
  void addEdge(const Node *u, const Node *v) {
    // Cannot have "parallel edges", i.e. multiple directed edges from u->v.
    nodes[u].insert(v);
  }

  /* [removeEdge] deletes the directed edge from [u] to [v] in the graph. Thus,
   *   [v] is removed from the adjacency list of [u].
   *     [u]: The starting vertex of the directed edge to remove.
   *     [v]: The ending vertex of the directed edge to remove.
   *
   * Requires: [v] is in the adjacency list of [u] in the graph.
   */
  void removeEdge(const Node *u, const Node *v) {
    nodes.at(u).erase(v);
  }

  /* [begin] returns the starting position for looping over this graph with a
   *   simple iterator. Useful for range-based for loops in C++14.
   */
  iterator begin() {
    return nodes.begin();
  }

  /* [end] returns the ending position for looping over this graph with a
   *   simple iterator. Useful for range-based for loops in C++14.
   */
  iterator end() {
    return nodes.end();
  }

  /* [begin] returns the starting position for looping over this graph with a
   *   constant iterator. Useful for range-based for loops in C++14.
   */
  const_iterator cbegin() {
    return nodes.cbegin();
  }

  /* [end] returns the ending position for looping over this graph with a
   *   constant iterator. Useful for range-based for loops in C++14.
   */
  const_iterator cend() {
    return nodes.cend();
  }

protected:
  // Actual graph structure, represented as an adjacency list (set) of nodes.
  std::map<const Node*, std::set<const Node*>> nodes;
};

}

#endif