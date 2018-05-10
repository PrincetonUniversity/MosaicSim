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
   Node *node1, *node2, *node3;

   // create some nodes
   node1 = g.addNode(add, 1, "add $1,$3,$4");
   node2 = g.addNode(add, 1, "sub $1,$3,$4");
   node3 = g.addNode(add, 1, "xor $1,$3,$4");

   // add some dependants
   node1->addDependent(node2, always_mem_dep);
   node1->addDependent(node3, always_mem_dep);

   node2->addDependent(node1, always_mem_dep);
   node2->addDependent(node3, always_mem_dep);

   node3->addDependent(node1, always_mem_dep);
   node3->addDependent(node2, always_mem_dep);

   cout << g;

   node1->eraseDependent(node2, always_mem_dep);
   node1->eraseDependent(node3, always_mem_dep);

   cout << g;

   g.eraseNode(node1);
   g.eraseNode(node2);
   g.eraseNode(node3);
   cout << g;
   return 0;
}