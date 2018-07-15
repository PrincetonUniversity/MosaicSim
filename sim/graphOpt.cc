#include "base.h"
class GraphOpt {
public:
  Graph &g;
  GraphOpt(Graph &g) : g(g) {}
  bool checkType(Node *n) {
    if(n->typeInstr == I_ADDSUB || n->typeInstr == FP_ADDSUB || n->typeInstr == LOGICAL || n->typeInstr == CAST)
      return true;
    else
      return false;
  }
  bool findOptimizableTerminator(Node *init, set<Node*> phis) {
    std::set<Node*> frontier;
    std::set<Node*> next_frontier;
    std::set<Node*> visited;
    frontier.insert(init);
    visited.insert(init);
    while(frontier.size() > 0) {
      for(auto fit = frontier.begin(); fit != frontier.end(); ++fit) {
        Node *n = *fit;
        for(auto it = n->parents.begin(); it!=n->parents.end(); ++it) {
          Node *sn = *it;
          if(sn->typeInstr == PHI && (phis.find(sn) != phis.end()))
            continue;
          if(!checkType(sn))
            return false;
          else if(visited.find(sn) == visited.end()) {
            visited.insert(sn);
            next_frontier.insert(sn);
          }
        }
      }
      frontier.clear();
      frontier = next_frontier;
      next_frontier.clear();
    }
    return true;
  }
  bool findOptimizablePhi(Node *init) {
    assert(init->typeInstr == PHI);
    std::set<Node*> frontier;
    std::set<Node*> next_frontier;
    std::set<Node*> visited;
    for(auto pit = init->phi_parents.begin(); pit!= init->phi_parents.end(); ++pit) {
      Node *sn = *pit;
      if(sn->bbid == init->bbid) {
        if(sn->typeInstr == PHI)
          assert(false); 
        if(!checkType(sn)) {
          return false;
        }
        visited.insert(sn);
        frontier.insert(sn);
      }
    }
    while(frontier.size() > 0) {
      for(auto fit = frontier.begin(); fit != frontier.end(); ++fit) {
        Node *n = *fit;
        for(auto it = n->parents.begin(); it!=n->parents.end(); ++it) {
          Node *sn = *it;
          if(sn->typeInstr == PHI && sn!= init)
            assert(false);
          if(sn->typeInstr == PHI && sn == init)
            continue;
          if(!checkType(sn)) {
            return false;
          }
          else if(visited.find(sn) == visited.end()) {
            visited.insert(sn);
            next_frontier.insert(sn);
          }
        }
      }
      frontier.clear();
      frontier = next_frontier;
      next_frontier.clear();
    }
    return true;
  }
  void inductionOptimization() {
    std::set<Node*> phis;
    std::vector<Node*> terms;
    for(auto it = g.nodes.begin(); it!= g.nodes.end(); ++it) {
      Node *n = it->second;
      if(n->typeInstr == PHI) {
        if(findOptimizablePhi(n)) {
          cout << "Optimized Phi : " << *n << "\n";
          phis.insert(n);
        }
      }
    }
    for(auto it = phis.begin(); it!= phis.end(); ++it) {
      Node *n = *it;
      for(auto pit = n->phi_parents.begin(); pit!= n->phi_parents.end(); ++pit) {
        Node *sn = *pit;
        if(sn->bbid == n->bbid)
          sn->eraseDependent(n, PHI_DEP);
      }
    }
    for(auto it = g.nodes.begin(); it!= g.nodes.end(); ++it) {
      Node *n = it->second;
      if(n->typeInstr == TERMINATOR) {
        if(findOptimizableTerminator(n, phis)) {
          for(auto iit = n->parents.begin(); iit != n->parents.end(); ++iit) {
            Node *sn = *iit;
            sn->eraseDependent(n, DATA_DEP);
          }
          cout << "Optimized Terminator : " << *n << "\n";
        }
      }
    }
  }
};