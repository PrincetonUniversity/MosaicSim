//=======================================================================
// Copyright 2018 University of Princeton.
//
// Project Apollo - simulator
// Authors: 
//=======================================================================

#include "sim-apollo.hpp"
using namespace apollo;

#include <iostream>                         
using namespace std;


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
   nodes[1] = g.addNode(instr_id++, lats[add], add, "add $1,$3,$4");
   nodes[2] = g.addNode(instr_id++, lats[sub], sub, "sub $1,$3,$4");
   nodes[3] = g.addNode(instr_id++, lats[logical], logical, "xor $1,$3,$4");
   nodes[4] = g.addNode(instr_id++, lats[add], add, "add $1,$3,$4");
   nodes[5] = g.addNode(instr_id++, lats[sub], sub, "sub $1,$3,$4");
   nodes[6] = g.addNode(instr_id++, lats[logical], logical, "xor $1,$3,$4");

   // add some dependents
   nodes[0]->addDependent(nodes[1], /*type*/ data_dep);

   nodes[1]->addDependent(nodes[2], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[3], /*type*/ data_dep);
   nodes[1]->addDependent(nodes[4], /*type*/ data_dep);

   nodes[2]->addDependent(nodes[5], /*type*/ data_dep);
   nodes[2]->addDependent(nodes[6], /*type*/ data_dep);
   
   cout << g;
   cout << "Accum.latency=" << g.calculate_accum_latency() << endl << endl;

   cout << "Critical Path=";
   cout << g.calculate_critical_path() << endl << endl;


   // some sanity checks
   nodes[2]->eraseDependent(nodes[5], data_dep);
   nodes[2]->eraseDependent(nodes[6], data_dep);
   cout << g << endl;

   g.eraseNode(nodes[0]);
   g.eraseNode(nodes[1]);
   g.eraseNode(nodes[2]);
   cout << g << endl;
   
   return 0;
}