#include "sim.h"
#include "memsys/Cache.h"
#include "memsys/DRAM.h"
#include "tile/Core.h"
#include "misc/Reader.h"
#include "graph/GraphOpt.h"
#include "tile/Core.h"
#include <bits/stdc++.h>
#include <numeric>
#include <fstream>
#include <iostream>


using namespace std;


Simulator::Simulator() {         
  cache = new Cache(cfg.cache_latency, cfg.cache_size, cfg.cache_assoc, cfg.cache_linesize, cfg.cache_load_ports, cfg.cache_store_ports, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(this, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->sim = this;
  cache->isLLC=true;
  cache->memInterface = memInterface;

  descq = new DESCQ();
  descq->consume_size=cfg.consume_size;
  descq->supply_size=cfg.supply_size;
  descq->term_buffer_size=cfg.term_buffer_size;
  descq->latency=cfg.desc_latency;  
}

void Simulator::registerCore(string wlpath, string cfgname, int id) {
  string name = "Pythia Core";
  string cfgpath = "../sim/config/" + cfgname+".txt";
  string cname = wlpath + "/output/ctrl.txt";     
  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";   
  
  Core* core = new Core(this, clockspeed);
  core->local_cfg.read(cfgpath);
  core->name=name;
  Reader r;
  r.readGraph(gname, core->g);
  r.readProfMemory(mname , core->memory);
  r.readProfCF(cname, core->cf);
  
  //GraphOpt opt(core->g);
  //opt.inductionOptimization();
  core->sim=this;
  //cout << "register core id " << id << endl;
  core->initialize(id);
  
  registerTile(core, id);  
}

int transactioncount=0;
bool Simulator::InsertTransaction(Transaction* t, uint64_t cycle) {
  assert(tiles.find(t->src_id)!=tiles.end());
  assert(tiles.find(t->dst_id)!=tiles.end());  
  
  int dst_clockspeed=tiles[t->dst_id]->clockspeed;
  int src_clockspeed=tiles[t->src_id]->clockspeed;
  int dst_cycle=(dst_clockspeed*cycle)/src_clockspeed; //should round up, but no +/-1 cycle is nbd
  //if(t->src_id==0)
  //cout << "source cycles, src cs, dst cs, dst cycles: " << cycle << ", " << src_clockspeed << ", " << dst_clockspeed << ", " << dst_cycle <<  endl;
  //assert(false);
  //cout << "dst cycles: " << dst_cycle << endl;
  //assert(transactioncount<10);
 
  uint64_t final_cycle=dst_cycle+transq_latency;
  //if(transq_map[t->dst_id].size()==0) {
  
     //priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare> pq;//=new priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare>;
    //vector<Transaction*> myvec;
    //myvec.push_back(t);
    //pq.push(make_pair(t,final_cycle));
    //transq_map[t->dst_id]=pq;
    //assert(false);
 
    //pq.push({t,final_cycle});
    
  
  
  
  transq_map[t->dst_id].push({t,final_cycle});
  
  
  return true;
}

//tile ids must be non repeating
void Simulator::registerTile(Tile* tile) {
  
  assert(tiles.find(tileCount)==tiles.end());
  tile->id=tileCount;
  tiles[tileCount]=tile;
  tileCount++;
  tile->sim=this;
  clockspeedVec.push_back(tile->clockspeed);
}

void Simulator::registerTile(Tile* tile, int tid) {
  //cout << "register tile id " << tid << endl;
  assert(tiles.find(tid)==tiles.end());
  tile->id=tid;
  tiles[tid]=tile;
  tile->sim=this;
  clockspeedVec.push_back(tile->clockspeed);
}

//LCM 
// Utility function to find
// GCD of 'a' and 'b'
uint64_t gcd(uint64_t a, uint64_t b)
{
  if (b == 0)
    return a;
  return gcd(b, a % b);
}

// Returns LCM of array elements
uint64_t findlcm(vector<uint64_t> numbers, int n)
{
  // Initialize result
  uint64_t ans = numbers[0];

  // ans contains LCM of numbers[0], ..numbers[i]
  // after i'th iteration,
  for (int i = 1; i < n; i++)
    ans = (((numbers[i] * ans)) /
           (gcd(numbers[i], ans)));

  return ans;
}

void Simulator::initDRAM() {
  //set the DRAMSim clockspeed based on Tile0's clockspeed
  memInterface->mem->setCPUClockSpeed((tiles[0]->clockspeed)*1000000);
}

void Simulator::run() {
  
  std::vector<int> processVec(tiles.size(), 0);
  int simulate = 1;
  clockspeed=findlcm(clockspeedVec,clockspeedVec.size());
  cout << "clockspeed : " << clockspeed << endl;
  
  while(simulate > 0) {
    if(simulate==0) {
      break;
    }
    for (auto it=tiles.begin(); it!=tiles.end(); ++it) {
    
      Tile* tile = it->second;
      uint64_t norm_tile_frequency=clockspeed / tile->clockspeed; //normalized cloockspeed. i.e., update every x cycles. note: automatically always an integer because of global clockspeed is lcm of local clockspeeds
     
      if(cycles%norm_tile_frequency==0) {
        
        vector<pair<Transaction*,uint64_t>> rejected_transactions;         
        //assume tiles never go from completed (process() returning false) back to not completed (process() returning true)

        
        processVec.at(it->first)=tile->process();
        
        //process transactions
        priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare>& pq=transq_map[tile->id];
        while(true) {
          if(pq.empty() || pq.top().second >= tile->cycles)
            break;
          Transaction* t=pq.top().first;
          uint64_t cycles=pq.top().second;
          if(!tile->ReceiveTransaction(t)) {
            rejected_transactions.push_back({t,cycles});
          }      
          pq.pop();    
        }
        //push back all the rejected transactions
        for(auto& trans_cycle: rejected_transactions) {
          pq.push(trans_cycle);
        }
        rejected_transactions.clear();
        
        //use same clockspeed as 1st tile (probably a core)
        if(it->first==0) {
          simulate += cache->process();    
          memInterface->process();
          descq->process();
        }
      }


    }
    
    simulate = accumulate(processVec.begin(), processVec.end(), 0);
    
   
    //---printing stats---    
    if(tiles[0]->cycles % 1000000 == 0 && tiles[0]->cycles !=0) {
      
      curr_time = Clock::now();
      uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count();
      //uint64_t tdiff_mins = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count();
      double instr_rate = ((double)(stat.get("total_instructions") - last_instr_count)) / tdiff;
      cout << "Global Simulation Speed: " << instr_rate << " Instructions per ms \n";
      
      uint64_t remaining_instructions = total_instructions - last_instr_count; 
      
      double remaining_time = (double)remaining_instructions/(60000*instr_rate);
      cout << "Remaining Time: " << (int)remaining_time << " mins \n Remaining Instructions: " << remaining_instructions << endl;
      
      last_instr_count = stat.get("total_instructions");
      last_time = curr_time;
    }
    else if(tiles[0]->cycles == 0) {
      last_time = Clock::now();
      last_instr_count = 0;
    }
    
    cycles++;
  
  }
  
  //print stats for each pythia tile
  for (auto it=tiles.begin(); it!=tiles.end(); it++) {
    Tile* tile=it->second;
    if(Core* core=dynamic_cast<Core*>(tile)) {
      stat.set("cycles", core->cycles);
      core->local_stat.set("cycles", core->cycles);
      cout << "----------------" << core->name << " General Stats--------------\n";
      core->local_stat.print();
    }
  }
  
  cout << "----------------GLOBAL STATS--------------\n";
  
  stat.print();
  memInterface->mem->printStats(true);
  curr_time=Clock::now();
  uint64_t tdiff_mins = chrono::duration_cast<std::chrono::minutes>(curr_time - init_time).count();
  uint64_t tdiff_seconds = chrono::duration_cast<std::chrono::seconds>(curr_time - init_time).count();
  uint64_t tdiff_milliseconds = chrono::duration_cast<std::chrono::milliseconds>(curr_time - init_time).count();
  if(tdiff_mins>5) {
    cout << "Total Runtime: " << tdiff_mins << " mins \n";
  }
  else if(tdiff_seconds>0) {
    cout << "Total Runtime: " << tdiff_seconds << " secs \n";
  }
  else
    cout << "Total Runtime: " << tdiff_milliseconds << " ms \n";

  cout << "Average Global Simulation Speed: " << 1000*total_instructions/tdiff_milliseconds << " Instructions per sec \n";
  if(descq->send_runahead_map.size()>0) {
    ofstream outfile;
    outfile.open("decouplingStats");
    
    uint64_t send_runahead_sum=0;
    outfile << "NODE_ID CONTEXT_ID DECOUPLING_ID RUNAHEAD_DIST" << endl;
    for(auto it=descq->send_runahead_map.begin(); it!=descq->send_runahead_map.end(); ++it) {
      DynamicNode* send_node = descq->send_map[it->first];
      outfile << send_node->n->id << " " << send_node->c->id << " " << it->first << " " << it->second << endl;
      send_runahead_sum+=it->second;    
    }
    uint64_t avg_send_runahead=send_runahead_sum/descq->send_runahead_map.size();
    
    cout<<"Avg SEND Runahead : " << avg_send_runahead << " cycles \n";
  }

  if(descq->stval_runahead_map.size()>0) {
    uint64_t stval_runahead_sum=0;
    for(auto it=descq->stval_runahead_map.begin(); it!=descq->stval_runahead_map.end(); ++it) {
      stval_runahead_sum+=it->second;    
    }
    uint64_t avg_stval_runahead=stval_runahead_sum/descq->stval_runahead_map.size();
    
    cout<<"Avg STVAL Runahead : " << avg_stval_runahead << " cycles \n";
  }

  if(descq->recv_delay_map.size()>0) {
    uint64_t recv_delay_sum=0;
    for(auto it=descq->recv_delay_map.begin(); it!=descq->recv_delay_map.end(); ++it) {
      recv_delay_sum+=it->second;    
    }
    uint64_t avg_recv_delay=recv_delay_sum/descq->recv_delay_map.size();
    
    cout<<"Avg RECV Delay : " << avg_recv_delay << " cycles \n";
  }

  //calculate and print stats on load latencies
  if(load_stats_map.size()>0) {
    ofstream loadfile;
    loadfile.open("loadStats");
    uint64_t totalLatency=0;
    string outstring="";
    for(auto entry:load_stats_map) {
      uint64_t issue_cycle=entry.second.first;
      uint64_t return_cycle=entry.second.second;
      int node_id=entry.first->n->id;
      
      totalLatency+=return_cycle-issue_cycle;
      outstring+= to_string(node_id) + " " + to_string(issue_cycle) + " " + to_string(return_cycle) + "\n";
    }
    
    loadfile << "Total Load Latency (cycles): " << totalLatency << endl;
    loadfile << "Node_ID Issue_Cycle Return_Cycle" << endl;
    loadfile << outstring;
  }

  //cout << "DeSC Forward Count: " << desc_fwd_count << endl;
  //cout << "Number of Vector Entries: " << load_count << endl; 
}

bool Simulator::canAccess(Core* core, bool isLoad) {
  Cache* cache = core->cache;
  if(isLoad)
    return cache->free_load_ports > 0 || cache->load_ports==-1;
  else
    return cache->free_store_ports > 0 || cache->store_ports==-1;
}

bool Simulator::communicate(DynamicNode* d) {
  return descq->insert(d);
}

void Simulator::orderDESC(DynamicNode* d) {
  if(d->n->typeInstr == SEND) {     
      descq->send_map.insert(make_pair(descq->last_send_id,d));
      d->desc_id=descq->last_send_id;
      descq->last_send_id++;
  }
  if(d->n->typeInstr == LD_PROD) {     
      descq->send_map.insert(make_pair(descq->last_send_id,d));
      d->desc_id=descq->last_send_id;
      descq->last_send_id++;
  }
  else if(d->n->typeInstr == STVAL) {
      descq->stval_map.insert(make_pair(descq->last_stval_id,d));
      d->desc_id=descq->last_stval_id;
      descq->last_stval_id++;
  }
  else if (d->n->typeInstr == RECV) {
     d->desc_id=descq->last_recv_id;
     descq->last_recv_id++;
  }
  else if (d->n->typeInstr == STADDR) {
    d->desc_id=descq->last_staddr_id;
    descq->last_staddr_id++;
  }
}

void Simulator::accessComplete(MemTransaction *t) {
  if(Core* core=dynamic_cast<Core*>(tiles[t->src_id])) {
    core->accessComplete(t);
  }
}



void DESCQ::process() {
  vector<DynamicNode*> failed_nodes;
  while(true) {
    if (pq.empty() || pq.top().second >= cycles)
      break;   
    if(!execute(pq.top().first)) {
      failed_nodes.push_back(pq.top().first);
    }
    pq.pop();    
  }
  for(auto& d:failed_nodes) {
    pq.push(make_pair(d,cycles));
  }
  
  //drain the terminal load buffer (possibly) up to the max size
  int term_buffer_count=0; 
  while(term_buffer_count<term_buffer_size && term_ld_buffer.size()>0) {
    DynamicNode* dn=term_ld_buffer.front();
    if (insert(dn)) {
      term_ld_buffer.pop_front();
      term_buffer_count++;
    }
    else {
      break;
    }
  }
  cycles++;
}

int desc_fwd_count=0;

bool DESCQ::execute(DynamicNode* d) {
  //int predecessor_send=d->desc_id-1;
  if (d->n->typeInstr==SEND || d->n->typeInstr==LD_PROD) { //sends can commit out of order
    d->c->insertQ(d);
    debug_send_set.insert(d->desc_id);
    supply_count--;
    return true;    
  }
  else if (d->n->typeInstr==STVAL) { //sends can commit out of order
    if(!stvalFwdPending(d)) { //must wait until all fwds are sent
     
      d->c->insertQ(d);
      debug_stval_set.insert(d->desc_id);
      return true;    
    }
    else{
      assert(false);
    }
  }
  else if (d->n->typeInstr==RECV) {
    //make sure recvs complete after corresponding send
    //or they can get a fwd from SVB
    if (recv_delay_map.find(d->desc_id)==recv_delay_map.end()) {
      recv_delay_map[d->desc_id]=cycles;
    }
    if (decrementFwdCounter(d)) {//can you get fwd from SVB
      desc_fwd_count++;
      assert(desc_fwd_count==0);
      d->c->insertQ(d);
      consume_count--;
      recv_delay_map[d->desc_id]=cycles - recv_delay_map[d->desc_id];
      return true;      
    }    
    
    if(send_map.find(d->desc_id)==send_map.end()) {
      return false;
    }
    else if (send_map.at(d->desc_id)->completed) {
      d->c->insertQ(d);
      consume_count--;
      recv_delay_map[d->desc_id]=cycles - recv_delay_map[d->desc_id];
      return true;      
    }
  }
  else if (d->n->typeInstr==STADDR) { //make sure staddrs complete after corresponding send
    if(stval_map.find(d->desc_id)==stval_map.end()) {
      return false;
    }
    else if (stval_map.at(d->desc_id)->completed) {
      d->c->insertQ(d);
      d->print("Access Memory Hierarchy", 1);
      d->core->access(d);
      return true;      
    }
  }
  return false;
}

void DESCQ::updateSVB(DynamicNode* lpd, DynamicNode* staddr_d) {
  stval_svb_map[staddr_d->desc_id]++;
  //want to increment the counter for fwds using staddr_d's desc id (really, for stval's desc id), that way stval knows when it's safe to complete
  recv_map[lpd->desc_id]=staddr_d->desc_id;
  //also want to create a mapping from ld_prod's descid (really for recv) to stval's desc id above. that way recv can know if there's a fwd waiting for it and also decrement the stval's counter
 
}

bool DESCQ::updateSAB(DynamicNode* lpd) { //called by issuedescnode
  
  uint64_t addr=lpd->addr;
  if(staddr_map.find(addr)==staddr_map.end()) { //we've deleted the entry for corresponding staddr
    return false;
  }
  
  set<DynamicNode*, DynamicNodePointerCompare> staddr_set=staddr_map.at(addr);
  bool found_fwd=false;
  auto it = staddr_set.begin();
  while(it!=staddr_set.end()) {    
    if(*lpd < **it) {
      if(it!=staddr_set.begin()) {
        it--;
        found_fwd=true;
      }
      break;
    }
    it++;
  }
  if(found_fwd) {
    
    DynamicNode* staddr_d=*it;
    updateSVB(lpd, staddr_d);
  }
  return found_fwd; 
}

void DESCQ::insert_staddr_map(DynamicNode* d) {
  
  if(staddr_map.find(d->addr)==staddr_map.end()) {      
    set<DynamicNode*, DynamicNodePointerCompare> temp;
    temp.insert(d);
    staddr_map[d->addr]=temp;
  }
  else {
    staddr_map[d->addr].insert(d);
  }  
}

bool DESCQ::decrementFwdCounter(DynamicNode* recv_d) {
  
  if(recv_map.find(recv_d->desc_id)==recv_map.end()) {
    return false;
  }
 
  uint64_t stval_id=recv_map.at(recv_d->desc_id);
  
  stval_svb_map[stval_id]--;
  assert(stval_svb_map[stval_id]>=0); 
  return true;
}

bool DESCQ::stvalFwdPending(DynamicNode* stval_d) {
  if(stval_svb_map.find(stval_d->desc_id)==stval_svb_map.end()) {
    return false;
  }
  return stval_svb_map[stval_d->desc_id]>0;
}

bool DESCQ::insert(DynamicNode* d) {
  bool canInsert=true;
  //luwa: should consider pushing ld_prod insertion request directly into term load buffer first
  if(d->n->typeInstr==LD_PROD) { //should be entry in comm queue   
    if(supply_count==supply_size) {  
      canInsert=false;
      if(d->issued && d!=term_ld_buffer.front()) { //don't push here if just testing if there are resources
        term_ld_buffer.push_back(d); //we're mostly using this so we don't lose the return from memory, only relevant when handlememoryreturn function calls insert
      }
    }
    else {     
      supply_count++;
    }
  }
  if(d->n->typeInstr==SEND) {     
    if(supply_count==supply_size)  
      canInsert=false;
    else
      supply_count++;  
  }
  if(d->n->typeInstr==RECV) {    
    if(consume_count==consume_size) {     
      canInsert=false;
    }
    else {     
      consume_count++;      
    }
  }
  if(d->n->typeInstr==STADDR) {
    //insert_staddr_map(d); doing it in initialize context
  }
  if(d->n->typeInstr==STVAL) {
    stval_svb_map[d->desc_id]=0;
  }
  
  if(canInsert) {
    pq.push(make_pair(d,cycles+latency));
    if(d->type==LD_PROD && d->issued==true) {
      pq.push(make_pair(d,cycles+(int) 0.6*latency));
    }
  }
  return canInsert;
}
