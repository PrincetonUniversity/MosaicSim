#include <fstream>
#include <map>
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
namespace apollo {
class Node 
{
public:
  std::string name;
  std::set<Node*> c_adjs;
  std::set<Node*> d_adjs;
  std::set<Node*> m_adjs;
  std::set<Node*> mm_adjs;
  std::set<Node*> p_adjs;
  std::set<Node*> lc_adjs;
  std::set<Node*> lcm_adjs;
  std::map<Node*, int> lc_dist;
  std::map<Node*, int> lcm_dist;
  int id;
  int type;
  Value *v;
  BasicBlock *bb;
  Node(Value *v, int id_in) {
    this->v = v;
    if(Instruction *inst = dyn_cast<Instruction>(v)) {
      type = 0;
      bb = inst->getParent();
    }
    else if(isa<Constant>(v) || isa<Argument>(v))
      type = 1;
    else if(BasicBlock *b = dyn_cast<BasicBlock>(v)) {
      type = 2;
      bb = b;
    }
    else
      assert(false);
    id = id_in;
    name = ""; 
    if(type == 0 || type == 1 || type == 3) {
       raw_string_ostream strm(name);
      v->print(strm);
      name = strm.str();
    }
    else if(type == 2)
      name = v->getName();
  }
};
class Graph
{
public:
  std::set<Value*> vals;
  std::vector<Node*> nodes;
  std::map<Value*, Node*> vmap;
  bool includeConstant;
  Graph() {
    includeConstant = false;
  }
  bool isConstantorArgument(Value *v) {
    if(isa<Constant>(v) || isa<Argument>(v))
      return true;
    else
      return false;
  }
  void insertNode(Value *v) {
    if(isConstantorArgument(v) && !includeConstant)
      return;
    if(vals.find(v) == vals.end()) {
      Node* n = new Node(v, vals.size());
      nodes.push_back(n);
      vmap.insert(std::make_pair(v, n));
      vals.insert(v);
    }
  }
  void addEdge(Value *s, Value *d, int type, int dist=-999) {
    if((isConstantorArgument(s) || isConstantorArgument(d)) && !includeConstant)
      return;
    if((vmap.find(s) == vmap.end()) || (vmap.find(d) == vmap.end())) {
      if(vmap.find(s) == vmap.end())
        errs() << *s << "\n";
      else if(vmap.find(d) == vmap.end())
        errs() << *d << "\n";
      assert(false);
    }
    Node *src = vmap.at(s);
    Node *dst = vmap.at(d);
    std::set<Node*> *adjs;
    if(type == 0)
      adjs = &(src->c_adjs);
    else if (type == 1)
      adjs = &(src->d_adjs);
    else if (type == 2)
      adjs = &(src->m_adjs);
    else if(type == 3)
      adjs = &(src->p_adjs);
    else if(type == 4)
      adjs = &(src->mm_adjs);
    else if(type == 5)
      adjs = &(src->lc_adjs);
    else if(type == 6)
      adjs = &(src->lcm_adjs);
    else
      assert(false);
    if(adjs->find(dst) != adjs->end())
      assert(false);
    adjs->insert(dst);
    if(type == 5)
      src->lc_dist.insert(std::make_pair(dst, dist));
    if(type == 6)
      src->lcm_dist.insert(std::make_pair(dst, dist));
  }
  void visualize() {
    std::ofstream fout;
    std::string gname = "graph";
    const char *file = (gname + ".dot").c_str();
    fout.open(file);
    fout << "digraph G {\n";
    fout << "node [nodesep=0.75, ranksep=0.75];\n";
    fout << "edge [weight=1.2];\n";
    std::vector<Node*> clusters;
    for(int i=0; i<nodes.size(); i++) {
      Node *n = nodes.at(i);
      if(n->type == 2)
        clusters.push_back(n);
    }
    for(int i=0; i<clusters.size(); i++) {
      fout << "subgraph cluster_" << std::to_string(i) << " {\n";
      fout << "color=black;\n";
      Node *bn = clusters.at(i);
      for(int j=0; j<nodes.size(); j++) {
        Node *n = nodes.at(j);
        if(bn->bb != n->bb)
          continue;
        std::string extra;
        if(n->type != 0)
          extra = ",shape=rectangle";
        else
          extra = ",shape=ellipse";
        if(isa<LoadInst>(n->v))
          extra += ",fontcolor=red,pencolor=red";
        else if(isa<StoreInst>(n->v))
          extra += ",fontcolor=red,pencolor=red";
        else if(isa<TerminatorInst>(n->v))
          extra += ",fontcolor=blue,pencolor=blue";
        else if(isa<BasicBlock>(n->v))
          extra += ",fontcolor=blue,pencolor=blue";
        fout << n->id << "[label=\"" << n->name <<"\",fontsize=10"<< extra << "];\n";
      }
      fout << "}\n";
    }
    for(int i=0; i<nodes.size(); i++) {
      Node *n = nodes.at(i);
      std::set<Node*>::iterator it;
      for(it = n->d_adjs.begin(); it!= n->d_adjs.end(); ++it) {
        Node *dst = *it;
        fout << n->id << " -> " << dst->id << "[color=black];\n";
      }
      for(it = n->c_adjs.begin(); it!= n->c_adjs.end(); ++it) {
        Node *dst = *it;
        bool rev = false;
        if(Instruction *inst = dyn_cast<Instruction>(n->v)) {
          if(inst->getParent() == dst->v) {
            rev = true;
          }
        }
        if(!rev)
          fout << n->id << " -> " << dst->id << "[color=blue];\n";
        else
          fout << dst->id << " -> " << n->id << "[color=blue,dir=back];\n";
      }
      for(it = n->m_adjs.begin(); it!= n->m_adjs.end(); ++it) {
        Node *dst = *it;  
        fout << n->id << " -> " << dst->id << "[color=red];\n";
      }
      for(it = n->mm_adjs.begin(); it!= n->mm_adjs.end(); ++it) {
        Node *dst = *it;
        fout << n->id << " -> " << dst->id << "[color=red,style=dotted];\n";
      }
      for(it = n->p_adjs.begin(); it!= n->p_adjs.end(); ++it) {
        Node *dst = *it;
        bool rev = false;
        if(isa<Instruction>(n->v) && isa<Instruction>(dst->v)) {
          Instruction *srcinst = cast<Instruction>(n->v);
          Instruction *dstinst = cast<Instruction>(dst->v);
          if(srcinst->getParent() == dstinst->getParent())
            rev = true;
        }  
        if(!rev)
          fout << n->id << " -> " << dst->id << "[color=navyblue];\n";
        else
          fout << dst->id << " -> " << n->id << "[color=navyblue,dir=back];\n";
      }
      for(it = n->lc_adjs.begin(); it!= n->lc_adjs.end(); ++it) {
        Node *dst = *it;
        int dist = n->lc_dist.at(dst);
        std::string dstr = "";
        if(dist != -999)
          dstr = std::to_string(dist);
        fout << n->id << " -> " << dst->id << "[label=\"" << dstr <<"\",color=orange];\n";
      }
      for(it = n->lcm_adjs.begin(); it!= n->lcm_adjs.end(); ++it) {
        Node *dst = *it;
        int dist = n->lcm_dist.at(dst);
        std::string dstr = "";
        if(dist != -999)
          dstr = std::to_string(dist);
        fout << n->id << " -> " << dst->id << "[label=\"" << dstr <<"\",color=orange,style=dotted];\n";
      }
    }
    fout << "subgraph cluster_help {\ncolor=black;\n";
    fout << "t1[label=\"Red Ellipse = LD/ST\",fontsize=10,shape=ellipse,fontcolor=red,pencolor=red];\n";
    fout << "t2[label=\"Blue Rectangle = BasicBlock\",fontsize=10,shape=rectangle,fontcolor=blue,pencolor=blue];\n";
    fout << "t3[label=\"Blue Ellipse = Terminator Instruction\",fontsize=10,shape=ellipse,fontcolor=blue,pencolor=blue];\n";
    fout << "t4[label=\"Black Edge = Data Dependence\n Red Edge = Memory Dependence (Dotted=Maybe) \n Orange Edge = Loop-carried Memory Dependence (Dotted=Maybe) \n Blue Edge = Control\n Navy Edge = Phi Data Dependence \",fontsize=10,shape=rectangle,fontcolor=black];\n";
    fout << "t1->t2 [style=invis];\n";
    fout << "t2->t3 [style=invis];\n";
    fout << "t3->t4 [style=invis];\n";
    fout << "}\n";
    fout << "}\n";
  }

  void exportToFile(Function &f) {
   std::ofstream cfile ("input/graph.txt");
   if (cfile.is_open()) {
     // Initial basic block
     cfile << "0\n";
     // Number of basic blocks
     std::vector<BasicBlock*> bbids;
     std::vector<Instruction*> instids;
     std::map<Instruction*,int> instToBB;
     int bbIdx = 0;
     int instIdx = 0;
     for (auto &bb : f) {
       bbids.push_back(&bb);
       for (auto &inst : bb) {
         instids.push_back(&inst);
         instToBB[&inst] = bbIdx;
         instIdx++;
       }
       bbIdx++;
     }
     cfile << bbIdx << "\n";
     // Number of nodes
     int numNodes = nodes.size();
     cfile << numNodes << "\n";
     // Number of edges
     int numEdges = 0;
     std::map<Node*,int> nodeMap;
     int counter = 0;
     for (auto &node : nodes) {
       nodeMap[node] = counter;
       numEdges += node->c_adjs.size();
       numEdges += node->d_adjs.size();
       numEdges += node->m_adjs.size();
       numEdges += node->mm_adjs.size();
       numEdges += node->p_adjs.size();
       numEdges += node->lc_adjs.size();
       numEdges += node->lcm_adjs.size();
       counter++;
     }
     cfile << numEdges << "\n";
     for (int id = 0; id < numNodes; id++) { 
       int instType = -1;
       Instruction* inst = instids[id];
       if (isa<AddInst>(inst)) {
         instType = 1;
       } else if (isa<SubInst>(inst)) {
         instType = 2;
       } else if (isa<LogicalInst>(inst)) {
         instType = 3;
       } else if (isa<MultInst>(inst)) {
         instType = 4;
       } else if (isa<DivInst>(inst)) {
         instType = 5;
       } else if (isa<LoadInst>(inst)) {
         instType = 6;
       } else if (isa<StoreInst>(inst)) {
         instType = 7;
       } else if (isa<TerminatorInst>(inst)) {
         instType = 8;
       } else if (isa<PHIInst>(inst)) {
         instType = 9;
       } else {
         instType = 0;
       }
       cfile << id << "," << instType << "," << instToBB[inst] << "," << "placeholder" << "\n";
     }
     for (auto &src : nodes) {
       for (auto &dest : src->c_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "0" << "\n";
       }
       for (auto &dest : src->d_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "1" << "\n";
       }
       for (auto &dest : src->m_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "2" << "\n";
       }
       for (auto &dest : src->mm_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "3" << "\n";
       }
       for (auto &dest : src->p_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "4" << "\n";
       }
       for (auto &dest : src->lc_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "5" << "\n";
       }
       for (auto &dest : src->lcm_adjs) {
         cfile << nodeMap[src] << "," << nodeMap[dest] << "6" << "\n";
       }
     }
   } else {
     cfile << "Unable to open file\n";
   }
  }

};
}
