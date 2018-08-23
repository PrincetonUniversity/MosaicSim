
#include "Graph.h"
using namespace std;

void Node::addDependent(Node *dest, TEdge type) {
  if(type == DATA_DEP) {
    if(dest->bbid == this->bbid) {
      dependents.insert(dest);
      dest->parents.insert(this);
    }
    else {
      if(dest->typeInstr == PHI)
        assert(false);
      external_dependents.insert(dest);
      dest->external_parents.insert(this);
    }
  }
  else if(type == PHI_DEP) {
    phi_dependents.insert(dest);
    dest->phi_parents.insert(this);
  }
}

void Node::eraseDependent(Node *dest, TEdge type) {
  int count = 0;
  if(type == DATA_DEP) {
    if(dest->bbid == this->bbid) {
      count += dependents.erase(dest);
      dest->parents.erase(this);
    }
    else {
      count += external_dependents.erase(dest);
      dest->external_parents.erase(this);
    }
  }
  else if(type == PHI_DEP) {
    count += phi_dependents.erase(dest);
    dest->phi_parents.erase(this);
  }
  assert(count == 1);
}

void BasicBlock::addInst(Node* n) {
  inst.push_back(n);
  inst_count++;
  if(n->typeInstr == LD)
    ld_count++;
  else if(n->typeInstr == ST)
    st_count++;
}

void Graph::addBasicBlock(int id) {
  bbs.insert( std::make_pair(id, new BasicBlock(id)) );
}

void Graph::addNode(int id, TInstr type, int bbid, std::string name, int lat) {
  Node *n = new Node(id, type, bbid, name, lat);
  nodes.insert(std::make_pair(n->id, n));
  assert( bbs.find(bbid) != bbs.end() );
  bbs.at(bbid)->addInst(n);
}

Node* Graph::getNode(int id) {
  if ( nodes.find(id) != nodes.end() )
    return nodes.at(id);
  else
    return NULL;
}

void Graph::eraseNode(Node *n) { 
  if (n) {
    nodes.erase(n->id); 
    delete n;
  }
}

void Graph::eraseAllNodes() { 
  for ( std::map<int, Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
    eraseNode(it->second);
}

void Graph::addDependent(Node *src, Node *dest, TEdge type) {
  src->addDependent(dest, type);
}

void Graph::eraseDependent(Node *src, Node *dest, TEdge type) {
  src->eraseDependent(dest, type);
}

// Print Node
std::ostream& operator<<(std::ostream &os, Node &n) {
  os << "[" << n.name << "]";
  return os;
}
// Print Graph
std::ostream& operator<<(std::ostream &os, Graph &g) {
  for (std::map<int, Node *>::iterator it = g.nodes.begin(); it != g.nodes.end(); ++it)
    std::cout << it->first << ":" << *it->second << "\n";
  std::cout << "";
  return os;
}
