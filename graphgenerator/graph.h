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
  std::set<Node*> p_adjs;
  std::set<Node*> bb_s_adjs;
  std::set<Node*> bb_e_adjs;
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
  void addEdge(Value *s, Value *d, int type) {
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
      adjs = &(src->bb_s_adjs);
    else if(type == 5)
      adjs = &(src->bb_e_adjs);
    else
      assert(false);
    if(adjs->find(dst) != adjs->end())
      assert(false);
    adjs->insert(dst);
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
        if(Instruction *inst = dyn_cast<Instruction>(n->v))
          if(inst->getParent() == dst->v) {
            rev = true;
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
      for(it = n->p_adjs.begin(); it!= n->p_adjs.end(); ++it) {
        Node *dst = *it;
        fout << n->id << " -> " << dst->id << "[color=navy];\n";
      }
      for(it = n->bb_s_adjs.begin(); it!= n->bb_s_adjs.end(); ++it) {
        Node *dst = *it;
        fout << n->id << " -> " << dst->id << "[color=black,style=invis];\n";
      }
      for(it = n->bb_e_adjs.begin(); it!= n->bb_e_adjs.end(); ++it) {
        Node *dst = *it;
        fout << n->id << " -> " << dst->id << "[color=black,style=invis];\n";
      }
    }
    fout << "subgraph cluster_help {\ncolor=black;\n";
    fout << "t1[label=\"Red Ellipse = LD/ST\",fontsize=10,shape=ellipse,fontcolor=red,pencolor=red];\n";
    fout << "t2[label=\"Blue Rectangle = BasicBlock\",fontsize=10,shape=rectangle,fontcolor=blue,pencolor=blue];\n";
    fout << "t3[label=\"Blue Ellipse = Terminator Instruction\",fontsize=10,shape=ellipse,fontcolor=blue,pencolor=blue];\n";
    fout << "t4[label=\"Black Edge = Data Dependence\n Red Edge = Memory Dependence\n Blue Edge = Control\n Navy Edge = Phi Data Dependence\",fontsize=10,shape=rectangle,fontcolor=black];\n";
    fout << "t1->t2 [style=invis];\n";
    fout << "t2->t3 [style=invis];\n";
    fout << "t3->t4 [style=invis];\n";
    fout << "}\n";
    fout << "}\n";
  }

};
}