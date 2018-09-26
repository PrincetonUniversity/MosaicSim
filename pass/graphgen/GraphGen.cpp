#include <cassert>
#include <fstream>
#include "Graph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#define KERNEL_STR "_kernel_"
using namespace apollo;
class GraphGen : public FunctionPass {
public:
  static char ID;
  Graph depGraph;
  std::map<Value*,Node*> nodeMap;
  std::map<Instruction*, Instruction*> store_to_addr;
  LoopInfo *LI;
  ScalarEvolution *SE;

  GraphGen(): FunctionPass(ID) { }
  virtual bool runOnFunction(Function &func) override;
  virtual StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &au) const override;
  bool isKernelFunction(Function &func);
  void analyzeLoop();
  void constructGraph(Function &func);
  void addDataEdges(Function &func);
  void addPhiEdges(Function &func);
  void addControlEdges(Function &func);
  void detectComm(Function &func);
  void exportGraph();
  void visualize();
};

char GraphGen::ID = 0;
static RegisterPass<GraphGen> rp("graphgen", "Graph Generator", false, true);
StringRef GraphGen::getPassName() const {
  return "Graph Generator";
}

bool GraphGen::isKernelFunction(Function &func) {
  return (func.getName().str().find(KERNEL_STR) != std::string::npos);
}

void GraphGen::getAnalysisUsage(AnalysisUsage &au) const {
  au.addRequired<ScalarEvolutionWrapperPass>();
  au.addRequired<LoopInfoWrapperPass>();
  au.setPreservesAll();
}
bool GraphGen::runOnFunction(Function &func) {
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();

  if (isKernelFunction(func)) {
    constructGraph(func);
    addDataEdges(func);
    addControlEdges(func);
    addPhiEdges(func);
    detectComm(func);
    analyzeLoop();
    visualize();
    exportGraph();
  }
  return false;
}

void GraphGen::detectComm(Function &func) {
  int id = 0;
  for (auto &bb : func) {
    for (auto &inst : bb) {
      Instruction *i = &(inst);
      Node *n = nodeMap[i];
      if (CallInst *cinst = dyn_cast<CallInst>(&inst)) {
        for (Use &use : inst.operands()) {
          Value *v = use.get();
          if(Function *f = dyn_cast<Function>(v)) {
            errs() << "Call Instruction : "<< f->getName() << "\n";
            if(f->getName() == "_Z16desc_produce_i32Pi") {
              errs() << "[SEND]"<< *i << "\n";
              n->itype = SEND;
            }
            else if(f->getName() == " _Z16desc_consume_i32v") {
              errs() << "[RECV]"<< *i << "\n";
              n->itype = RECV;
            }
            if(f->getName() == "_Z19desc_store_addr_i32Pi") {
              errs() << "[STADDR]"<< *i << "\n";
              n->itype = STADDR;
            }
            else if(f->getName() == "_Z18desc_store_val_i32i") {
              errs() << "[STVAL]"<< *i << "\n";
              n->itype = STVAL;
            }
          }
        }
      }
    }
  }
}
void GraphGen::analyzeLoop() {
  for(Loop *L : *LI) {
    PHINode *p = L->getCanonicalInductionVariable();
    if(p!=NULL) {
      Instruction *inst = dyn_cast<Instruction>(p);
      Node *n = nodeMap[inst];

      errs() << "(Canonical) Induction Variable : "<< *inst << "\n";
      std::vector<Instruction*> frontier;
      std::vector<Instruction*> next_frontier;
      std::set<Instruction*> processed;
      frontier.push_back(inst);
      processed.insert(inst);
      while(frontier.size() > 0) {
        for(int i=0; i<frontier.size(); i++) {
          Instruction *curr = frontier.at(i);
          for (User *U : curr->users()) {
            if (Instruction *child = dyn_cast<Instruction>(U)) {
              depGraph.removeEdge(nodeMap[curr], nodeMap[child], Edge_Data);
            }
            if(processed.find(curr) == processed.end()) {
              next_frontier.push_back(curr);
              processed.insert(curr);
            }
          }
        }
        frontier.clear();
        frontier = next_frontier;
        next_frontier.clear();
      }
      for (int i = 0; i < p->getNumIncomingValues(); i++) {
        Value *v = p->getIncomingValue(i);
        if(isa<Instruction>(v)) {
          auto phisrc =  nodeMap.at(v);
          depGraph.removeEdge(phisrc, n, Edge_Phi);
        }
      }
    }
    
  }
}
void GraphGen::constructGraph(Function &func) {
  int uid =0;
  for (auto &bb : func) {
    BasicBlock *b = &(bb);
    Node* n = new Node(Node_BasicBlock, b, uid, -1, uid);
    depGraph.addNode(n);
    nodeMap[b] = n;
    uid++;
  }
  int id = 0;
  for (auto &bb : func) {
    for (auto &inst : bb) {
      Instruction *i = &(inst);
      if (StoreInst *stinst = dyn_cast<StoreInst>(&inst)) {
        Value *pv = stinst->getPointerOperand();
        if(Instruction *ipv = dyn_cast<Instruction>(pv)) {
          store_to_addr.insert(std::make_pair(stinst, ipv));
        }
        else
          assert(false);
      }
      Node *n = new Node(Node_Instruction, i, uid, id, nodeMap[i->getParent()]->bb_id);
      depGraph.addNode(n);
      nodeMap[i] = n;
      id++;
      uid++;
    }
  }
}
void GraphGen::addDataEdges(Function &func) {
  for (auto &bb : func) {
    for (auto &inst : bb) {
      Node *dst = nodeMap.at(&inst);
      if (!isa<PHINode>(inst)) {
        for (Use &use : inst.operands()) {
          Value *v = use.get();
          if(isa<Instruction>(v)) {
            Node *src = nodeMap.at(v);
            depGraph.addEdge(src, dst, Edge_Data);
          }
        }
      }
    }
  }
}

void GraphGen::addControlEdges(Function &func) {
  for (auto &bb : func) {
    TerminatorInst* term = bb.getTerminator();
    auto src = nodeMap.at(&bb);
    for (auto &phiNode : bb.phis()) {
      auto phidst = nodeMap.at(&phiNode);
      depGraph.addEdge(src, phidst, Edge_Control);
    }
    src = nodeMap.at(term);
    for (int i = 0; i < term->getNumSuccessors(); i++) {
      auto dst = nodeMap.at(term->getSuccessor(i));
      depGraph.addEdge(src, dst, Edge_Control);
    }
  }
}

void GraphGen::addPhiEdges(Function &func) {
  for (auto &bb : func) {
    for (auto &phiNode : bb.phis()) {
      auto phidst = nodeMap.at(&phiNode);
      for (int i = 0; i < phiNode.getNumIncomingValues(); i++) {
        Value *v = phiNode.getIncomingValue(i);
        if(isa<Instruction>(v)) {
          auto phisrc =  nodeMap.at(v);
          depGraph.addEdge(phisrc, phidst, Edge_Phi);
        }
      }
    }
  }
}

void GraphGen::visualize() {
  std::ofstream fout;
  errs() << "[GraphGen] Start Visuzliation \n";
  std::string gname = "int/graphDiagram";
  const char *file = (gname + ".dot").c_str();
  fout.open(file);
  fout << "digraph G {\n";
  fout << "node [nodesep=0.75, ranksep=0.75];\n";
  fout << "edge [weight=1.2];\n";
  std::vector<Node*> clusters;
  for(int i=0; i<depGraph.bb_nodes.size(); i++) {
    Node *n = depGraph.bb_nodes.at(i);
    clusters.push_back(n);
  }
  for(int i=0; i<clusters.size(); i++) {
    fout << "subgraph cluster_" << std::to_string(i) << " {\n";
    fout << "color=black;\n";
    Node *bn = clusters.at(i);
    for(int j=0; j<depGraph.nodes.size(); j++) {
      Node *n = depGraph.nodes.at(j);
      if(bn->bb_id != n->bb_id)
        continue;
      std::string extra;
      if(n->type == Node_BasicBlock)
        extra = ",shape=rectangle";
      else if(n->type == Node_Instruction)
        extra = ",shape=ellipse";
      if(isa<LoadInst>(n->val))
        extra += ",fontcolor=red,pencolor=red";
      else if(isa<StoreInst>(n->val))
        extra += ",fontcolor=red,pencolor=red";
      else if(isa<TerminatorInst>(n->val))
        extra += ",fontcolor=blue,pencolor=blue";
      else if(isa<BasicBlock>(n->val))
        extra += ",fontcolor=blue,pencolor=blue";
      fout << n->uid << "[label=\"" << n->name <<"\",fontsize=10"<< extra << "];\n";
    }
    fout << "}\n";
  }
  for(int i=0; i<depGraph.nodes.size(); i++) {
    Node *n = depGraph.nodes.at(i);
    std::set<Node*>::iterator it;
    if(depGraph.dataEdges.find(n) != depGraph.dataEdges.end()) {
      std::set<Node*> &m = depGraph.dataEdges.at(n);
      for(it = m.begin(); it!= m.end(); ++it) {
        Node *dst = *it;
        fout << n->uid << " -> " << dst->uid << "[color=black];\n";
      }
    }
    if(depGraph.controlEdges.find(n) != depGraph.controlEdges.end()) {
      std::set<Node*> &m = depGraph.controlEdges.at(n);
      for(it = m.begin(); it!= m.end(); ++it) {
        Node *dst = *it;
        bool rev = false;
        if(Instruction *inst = dyn_cast<Instruction>(n->val)) {
          if(inst->getParent() == dst->val) {
            rev = true;
          }
        }
        if(!rev)
          fout << n->uid << " -> " << dst->uid << "[color=blue];\n";
        else
          fout << dst->uid << " -> " << n->uid << "[color=blue,dir=back];\n";
      }
    }
    if(depGraph.memoryEdges.find(n) != depGraph.memoryEdges.end()) {
      std::set<Node*> &m = depGraph.memoryEdges.at(n);
      for(it = m.begin(); it!= m.end(); ++it) {
        Node *dst = *it;
        fout << n->uid << " -> " << dst->uid << "[color=red];\n";
      }
    }
    if(depGraph.phiEdges.find(n) != depGraph.phiEdges.end()) {
      std::set<Node*> &m = depGraph.phiEdges.at(n);
      for(it = m.begin(); it!= m.end(); ++it) {
        Node *dst = *it;
        bool rev = false;
        if(isa<Instruction>(n->val) && isa<Instruction>(dst->val)) {
          Instruction *srcinst = cast<Instruction>(n->val);
          Instruction *dstinst = cast<Instruction>(dst->val);
          if(srcinst->getParent() == dstinst->getParent())
            rev = true;
        }  
        if(!rev)
          fout << n->uid << " -> " << dst->uid << "[color=navyblue];\n";
        else
          fout << dst->uid << " -> " << n->uid << "[color=navyblue,dir=back];\n";
      }
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
  errs() << "[GraphGen] Finished Visuzliation \n";
}
void GraphGen::exportGraph() {
  std::ofstream cfile ("output/graphOutput.txt");
  if (cfile.is_open()) {
    int numEdge = depGraph.num_export_edges  + store_to_addr.size();
    cfile << depGraph.bb_nodes.size() << "\n";
    cfile << depGraph.i_nodes.size() << "\n";
    cfile << numEdge << "\n";
    for(int i=0; i<depGraph.i_nodes.size(); i++) {
      Node *n =depGraph.i_nodes.at(i);
      cfile << n->id << "," << static_cast<int>(n->itype) << "," << n->bb_id << "," << n->name << "\n";
    }
    int ect = 0;
    for(auto &src : depGraph.i_nodes) {
      if(depGraph.dataEdges.find(src) != depGraph.dataEdges.end()) {
        for(auto &dst: depGraph.dataEdges.at(src)) {
          cfile << src->id << "," << dst->id << ",0\n";
          ect++;
        }
      }
      if(depGraph.phiEdges.find(src) != depGraph.phiEdges.end()) {
        for(auto &dst: depGraph.phiEdges.at(src)) {
          cfile << src->id << "," << dst->id << ",1\n";
          ect++;
        }
      }
    }
    /* Store to Addr Edges */
    for(std::map<Instruction*, Instruction*>::iterator it = store_to_addr.begin(); it != store_to_addr.end(); ++it) {
      Node *src = nodeMap.at(it->first);
      Node *dst = nodeMap.at(it->second);
      cfile << src->id << "," << dst->id <<",-1\n";
      ect++;
    }
    if(ect != numEdge) {
      errs() << "Ect : " << ect << " / " << numEdge <<"\n";
      assert(false);
    }
  }
}