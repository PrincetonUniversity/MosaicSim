#include "Graph.h"

/** \brief   */
class GraphOpt {
public:
  /** \brief   */
  Graph &g;
  /** \brief   */
 GraphOpt(Graph &g) : g(g) {}
  /** \brief  checks... something..... 
      
      used only in this file. 
   */
  bool checkType(Node *n) {
    if(n->typeInstr == I_ADDSUB || n->typeInstr == FP_ADDSUB || n->typeInstr == LOGICAL || n->typeInstr == CAST)
      return true;
    else
      return false;
  }

  /** \brief Checks if starting from the #init Node, in all of his
      parents, grand parents and grand-grand parents etc.. all the 
      Node are of checkType() Tinstr

      Skips all the Nodes in the #phis set and nodes that are from Tinstr PHI. 
   */
  bool fiandOptimizableTerminator(Node *init, set<Node*> phis) {
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
	  // this means that a non-PHI instruction could be in the set #phis. 
          if(sn->typeInstr == PHI && (phis.find(sn) != phis.end()))
            continue;
          else if(!checkType(sn))
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
  
  /** \brief Starting from the init Node from Instr type PHI, checks that in its parents, 
      grand parents etc, no Node from the INstr checkType() exists.  

      Also if starting from the grandparents a node with at least 2 PHI parents are not 
      accepted. In the case that there is one but not from the same BasicBlock, only a 
      wargning is issued.
  */
  bool findOptimizablePhi(Node *init) {
    assert(init->typeInstr == PHI);
    std::set<Node*> frontier;
    std::set<Node*> next_frontier;
    std::set<Node*> visited;

    // Checks that all the Node paranets are from Tinstr checkType() 
    for(auto pit = init->phi_parents.begin(); pit!= init->phi_parents.end(); ++pit) {
      Node *sn = *pit;

      // TODO this test should be taken out
      if(sn->typeInstr == PHI && sn->bbid == init->bbid)
      {
      }
      else if(!checkType(sn)) {
        return false;
      }
      visited.insert(sn);
      frontier.insert(sn);

    }

    while(frontier.size() > 0) {
      for(auto fit = frontier.begin(); fit != frontier.end(); ++fit) {
        Node *n = *fit;
        
        if(n->phi_parents.size() >= 2) {
          cout << "Has Phi Parents : " << n->phi_parents.size() << "\n";
          return false;
        }
        if(n->phi_parents.size() == 1 && (*(n->phi_parents.begin()))->bbid != init->bbid) {
          cout << "Has Non-trivial Phi Parent : "<< *(n->phi_parents.begin()) << "\n";
        }

        for(auto it = n->external_parents.begin(); it!=n->external_parents.end(); ++it) {
          Node *sn = *it;
	  // TODO this test should be taken out
          if(sn->typeInstr == PHI && sn->bbid == init->bbid) {
          } 
          else if(!checkType(sn)) {
            return false;
          }
          else if(visited.find(sn) == visited.end()) {
            visited.insert(sn);
            next_frontier.insert(sn);
          }
        }

	for(auto it = n->parents.begin(); it!=n->parents.end(); ++it) {
          Node *sn = *it;

	  // TODO this test should be taken out
	  if(sn->typeInstr == PHI && sn->bbid == init->bbid)
          {
          }
          else if(!checkType(sn)) {
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

  /** \brief   */
  void inductionOptimization() {
    std::set<Node*> phis;
    std::vector<Node*> terms;
    // Goes throught the graph, and tries to find optimizable PHIs
    for(auto it = g.nodes.begin(); it!= g.nodes.end(); ++it) {
      Node *n = it->second;
      if(n->typeInstr == PHI) {
        if(findOptimizablePhi(n)) {
          cout << "Optimized Phi : " << *n << "\n";
          phis.insert(n);
        }
      }
    }

    // for the optimizable Node s erase its upwards dependecies (its parents) 
    for(auto it = phis.begin(); it!= phis.end(); ++it) {
      Node *n = *it;
      for(auto pit = n->phi_parents.begin(); pit!= n->phi_parents.end(); ++pit) {
        Node *sn = *pit;
	cout << "Erasing Dependent : " << *n << " - " << *sn << "\n";
	sn->eraseDependent(n, PHI_DEP);
      }
    }
    //todo understand this .... puts together the whole file
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
