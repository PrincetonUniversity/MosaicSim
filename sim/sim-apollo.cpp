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
std::map<Node*, int> latency_map;
std::map<Node*, int> dep_map;
int cycle_count = 0;

// ------------------------------------------------------------------------------------------

void process_cycle()
{
   std::vector<Node *> next_active_list;

   // print & update cycles
   std::cout << "Cycle : " << cycle_count << "\n";
   cycle_count++;

   // process ALL nodes in <active_list>, ie, those being executed in this cycle
   for ( int i=0; i < active_list.size(); i++ ) {

      Node *n = active_list.at(i);
      std::cout << "Node [" << n->instr_name << "] is processing now\n";
      int lat = latency_map.at(n);
      lat--;
      if ( lat > 0 ) { // node NOT done yet -> in progress 
         latency_map.at(n) = lat;
         next_active_list.push_back(n);  // Node <n> will continue execution in next cycle
      }
      else  { // node done!!
         std::cout << "Node [" << n->instr_name << "] Finished Execution \n";

         latency_map.erase(n);
         dep_map.erase(n);
         
         // traverse ALL dependents
         std::set<Edge>::iterator it;
         for ( it = n->dependents.begin(); it != n->dependents.end(); ++it ) {
            Node *d = it->dst;

            // insert dependent d in a map (if not present yet)
            if ( dep_map.find(d) == dep_map.end() )  
                dep_map.insert( std::make_pair(d, d->parent_count) );
            
            // decrease # of parents of dependent d
            dep_map.at(d)--;

            // if d has no more parents -> push it for "execution" in next cycle
            if ( dep_map.at(d) == 0 ) {
               next_active_list.push_back(d);
               std::cout << "Node [" << d->instr_name << "] will start Execution \n";
               latency_map.insert( std::make_pair(d, d->instr_lat) );
               dep_map.erase(d);
            }
         }
      }
   }
   active_list = next_active_list;
   if ( cycle_count == 20 )
      assert(false);
}

// ------------------------------------------------------------------------------------------

int main(int argc, char const *argv[])
{
   Graph g;
   Node *nodes[10];
   int lats[10];
   int instr_id = 1;

   // initilize the latencies array
   lats[ADD] = 1;
   lats[SUB] = 1;
   lats[LOGICAL] = 1;
   lats[MULT] = 5;
   lats[DIV] = 10;
   lats[LD] = 3;
   lats[ST] = 3;
   lats[BR_COND] = 2;
   lats[BR_UNCOND] = 2;

   // create a BB graph

   // first, create ALL the nodes
   nodes[0] = g.addNode();  // this is the entry point
   nodes[1] = g.addNode(instr_id++, lats[ADD], ADD, "1-add $1,$3,$4");
   nodes[2] = g.addNode(instr_id++, lats[SUB], SUB, "2-sub $1,$3,$4");
   nodes[3] = g.addNode(instr_id++, lats[LOGICAL], LOGICAL, "3-xor $1,$3,$4");
   nodes[4] = g.addNode(instr_id++, lats[DIV], DIV, "4-mult $1,$3,$4");
   nodes[5] = g.addNode(instr_id++, lats[SUB], SUB, "5-sub $1,$3,$4");
   nodes[6] = g.addNode(instr_id++, lats[LOGICAL], LOGICAL, "6-xor $1,$3,$4");

   // add some dependents
   nodes[0]->addDependent(nodes[1], /*type*/ data_dep);
   nodes[0]->addDependent(nodes[6], /*type*/ data_dep);
   nodes[0]->addDependent(nodes[6], /*type*/ data_dep);

   nodes[1]->addDependent(nodes[2], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[3], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[4], /*type*/ data_dep);

   nodes[2]->addDependent(nodes[5], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[6], /*type*/ data_dep);
   
   nodes[6]->addDependent(nodes[4], /*type*/ data_dep);


   cout << g;
   cout << "Accum.latency=" << g.calculate_accum_latency() << endl << endl;

   // Let's make a Topological Sort
   std::stack<Node *> Stack;
   g.make_topological_sort( Stack );
   cout << "Topological Sort=";
   while ( ! Stack.empty() ) {
     cout << Stack.top()->instr_name << " ";
     Stack.pop();
   }
   cout << endl << endl;

   // Let's EXECUTE the Graph
   cycle_count = 0;
   active_list.push_back( nodes[0] );
   latency_map.insert( std::make_pair(nodes[0], nodes[0]->instr_lat) );
   while ( active_list.size() != 0 || dep_map.size() != 0 )
      process_cycle();

   // some sanity checks
   //nodes[2]->eraseDependent(nodes[5], data_dep);
   //nodes[2]->eraseDependent(nodes[6], data_dep);
   //cout << g << endl << g.calculate_accum_latency() ;
   //g.eraseNode(nodes[0]);
   //g.eraseNode(nodes[1]);
   //g.eraseNode(nodes[2]);
   //cout << g.size() << endl;
   
   return 0;
} 
