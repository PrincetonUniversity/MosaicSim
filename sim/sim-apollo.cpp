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
   /* code */
   Graph g;
   Node *nodes[10];

   // create some nodes
   nodes[0] = g.addNode(1, 4, add, "add $1,$3,$4");
   nodes[1] = g.addNode(2, 4, sub, "sub $1,$3,$4");
   nodes[2] = g.addNode(3, 4, logical, "xor $1,$3,$4");
   nodes[3] = g.addNode();

   // add some dependants
   nodes[0]->addDependent(nodes[1], always_mem_dep);
   nodes[0]->addDependent(nodes[2], always_mem_dep);

   nodes[1]->addDependent(nodes[0], always_mem_dep);
   nodes[1]->addDependent(nodes[2], always_mem_dep);

   nodes[2]->addDependent(nodes[0], always_mem_dep);
   nodes[2]->addDependent(nodes[1], always_mem_dep);

   cout << g;

   nodes[0]->eraseDependent(nodes[1], always_mem_dep);
   nodes[0]->eraseDependent(nodes[2], always_mem_dep);

   cout << g;

   g.eraseNode(nodes[0]);
   g.eraseNode(nodes[1]);
   g.eraseNode(nodes[2]);
   cout << g;
   return 0;
}