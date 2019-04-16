
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
  if(n->typeInstr == LD || n->typeInstr == LD_PROD)
    ld_count++;
  else if(n->typeInstr == ST || n->typeInstr == STADDR)
    st_count++;
}

void Graph::addBasicBlock(int id) {
  bbs.insert( make_pair(id, new BasicBlock(id)) );
}


void Graph::addNode(int id, TInstr type, int bbid, string name, int lat, int vecWidth) {
  Node *n = new Node(id, type, bbid, name, lat);
  n->width=vecWidth;
  nodes.insert(make_pair(n->id, n));
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
  for ( map<int, Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it )
    eraseNode(it->second);
}

void Graph::addDependent(Node *src, Node *dest, TEdge type) {
  src->addDependent(dest, type);
}

void Graph::eraseDependent(Node *src, Node *dest, TEdge type) {
  src->eraseDependent(dest, type);
}

// Print Node
ostream& operator<<(ostream &os, Node &n) {
  os << "[" << n.name << "]";
  return os;
}
// Print Graph
ostream& operator<<(ostream &os, Graph &g) {
  for (map<int, Node *>::iterator it = g.nodes.begin(); it != g.nodes.end(); ++it)
    cout << it->first << ":" << *it->second << "\n";
  cout << "";
  return os;
}
