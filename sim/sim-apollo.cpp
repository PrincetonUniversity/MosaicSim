//=======================================================================
// Copyright 2018 University of Princeton.
//
// Project Apollo - simulator
// Authors: 
//=======================================================================

#include "sim-apollo.hpp"
using namespace apollo;

#include <iostream> 
#include <map>
#include "assert.h"
#include <vector>                        
using namespace std;

std::vector<Node*> active_list;
std::map<Node*, int> state_map;
std::map<Node*, int> dep_map;
int cycle_count;

void process_cycle()
{
   std::cout << "Cycle : " << cycle_count << "\n";
   cycle_count++;
   std::vector<Node*> next_active_list;
   for(int i=0; i<active_list.size(); i++) {
      Node *n = active_list.at(i);
      std::cout << "Node " << n->instr_name << " is Processed \n";
      int ct = state_map.at(n);
      ct--;
      if(ct == 0) {
         state_map.erase(n);
         dep_map.erase(n);
      }
      else {
         state_map.at(n) = ct;
         next_active_list.push_back(n);
      }
      if(ct == 0) {
         std::cout << "Node " << n->instr_name << " Finished Execution \n";
         std::set<Edge>::iterator it;
         for(it = n->dependents.begin(); it!= n->dependents.end(); ++it) {
            Node *d = (&(*it))->dst;
            if(dep_map.find(d) == dep_map.end()) {
               std::cout << "Node " << d->instr_name <<" inserted to Dep_Map - " << d->parent_count << " \n";
               dep_map.insert(std::make_pair(d, d->parent_count));
            }
            dep_map.at(d)--;
            if(dep_map.at(d) == 0) {
               next_active_list.push_back(d);
               std::cout << "Node " << d->instr_name << " will start Execution \n";
               state_map.insert(std::make_pair(d, d->instr_lat));
               dep_map.erase(d);
            }
         }
      }
   }
   active_list = next_active_list;
   if(cycle_count == 20)
      assert(false);
}

int main(int argc, char const *argv[])
{
   Graph g;
   Node *nodes[10];
   int lats[10];
   int instr_id = 1;

   // initilize the latencies array
   lats[add] = 1;
   lats[sub] = 1;
   lats[logical] = 1;
   lats[mult] = 5;
   lats[apollo::div] = 10;
   lats[ld] = 3;
   lats[st] = 3;
   lats[branch_cond] = 2;
   lats[branch_uncond] = 2;

   // create a BB graph

   // first, create ALL the nodes
   nodes[0] = g.addNode();  // this is the entry point
   nodes[1] = g.addNode(instr_id++, lats[add], add, "1-add $1,$3,$4");
   nodes[2] = g.addNode(instr_id++, lats[sub], sub, "2-sub $1,$3,$4");
   nodes[3] = g.addNode(instr_id++, lats[logical], logical, "3-xor $1,$3,$4");
   nodes[4] = g.addNode(instr_id++, lats[add], add, "4-add $1,$3,$4");
   nodes[5] = g.addNode(instr_id++, lats[sub], sub, "5-sub $1,$3,$4");
   nodes[6] = g.addNode(instr_id++, lats[logical], logical, "6-xor $1,$3,$4");

   // add some dependents
   nodes[0]->addDependent(nodes[1], /*type*/ data_dep);

   nodes[1]->addDependent(nodes[2], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[3], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[4], /*type*/ data_dep);

   nodes[2]->addDependent(nodes[5], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[6], /*type*/ data_dep);
   
   //cout << g;
   cout << "Test \n";
   //cout << "Accum.latency=" << g.calculate_accum_latency() << endl << endl;

   cout << "Critical Path=";
   cout << g.calculate_critical_path() << endl << endl;

   cycle_count = 0;
   active_list.push_back(g.getNode(0));
   state_map.insert(std::make_pair(g.getNode(0), 1));
   while(active_list.size() != 0 || dep_map.size() != 0)
      process_cycle();

   // some sanity checks
   nodes[2]->eraseDependent(nodes[5], data_dep);
   nodes[2]->eraseDependent(nodes[6], data_dep);
   //cout << g << endl;

   g.eraseNode(nodes[0]);
   g.eraseNode(nodes[1]);
   g.eraseNode(nodes[2]);
   //cout << g << endl;
   
   return 0;
} 
