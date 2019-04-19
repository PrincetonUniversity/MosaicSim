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
#include <cmath>

using namespace std;

Simulator::Simulator() {         
  cache = new Cache(cfg.cache_latency, cfg.cache_size, cfg.cache_assoc, cfg.cache_linesize, cfg.cache_load_ports, cfg.cache_store_ports, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(this, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->sim = this;
  cache->isLLC=true;
  cache->memInterface = memInterface;

  descq = new DESCQ(cfg);
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

void Simulator::initDRAM(int clockspeed) {
  //set the DRAMSim clockspeed
  memInterface->mem->setCPUClockSpeed(clockspeed * 1000000);
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
    
    long send_runahead_sum=0;
    outfile << "NODE_ID CONTEXT_ID DECOUPLING_ID RUNAHEAD_DIST" << endl;
    for(auto it=descq->send_runahead_map.begin(); it!=descq->send_runahead_map.end(); ++it) {
      DynamicNode* send_node = descq->send_map[it->first];
      outfile << send_node->n->id << " " << send_node->c->id << " " << it->first << " " << it->second << endl;
      send_runahead_sum+=it->second;      
      //assert(send_runahead_sum > -1*pow(2, 63));
    }

    long size = descq->send_runahead_map.size();
    long avg_send_runahead=send_runahead_sum/size;
    cout<<"Avg SEND Runahead : " << avg_send_runahead << " cycles \n";
  }

  if(descq->stval_runahead_map.size()>0) {
    long stval_runahead_sum=0;
    for(auto it=descq->stval_runahead_map.begin(); it!=descq->stval_runahead_map.end(); ++it) {
      stval_runahead_sum+=it->second;    
    }
    long avg_stval_runahead=stval_runahead_sum/descq->stval_runahead_map.size();
    
    cout<<"Avg STVAL Runahead : " << avg_stval_runahead << " cycles \n";
  }

  if(descq->recv_delay_map.size()>0) {
    long recv_delay_sum=0;
    for(auto it=descq->recv_delay_map.begin(); it!=descq->recv_delay_map.end(); ++it) {
      recv_delay_sum+=it->second;    
    }
    long avg_recv_delay=recv_delay_sum/descq->recv_delay_map.size();
    
    cout<<"Avg RECV Delay : " << avg_recv_delay << " cycles \n";
  }

  //calculate and print stats on load latencies
  if(load_stats_map.size()>0) {
    ofstream loadfile;
    loadfile.open("loadStats");
    long long totalLatency=0;
    string outstring="";
    for(auto entry:load_stats_map) {
      long long issue_cycle=entry.second.first;
      long long return_cycle=entry.second.second;
      int node_id=entry.first->n->id;
      long long diff=(return_cycle-issue_cycle);
      totalLatency=totalLatency + diff;
      
      //cout << "ret: " << return_cycle << ", issue: "<< issue_cycle << ", diff: " << diff << endl;
      //assert(diff>0);
      outstring+=to_string(entry.first->addr) + " " + to_string(node_id) + " " + to_string(issue_cycle) + " " + to_string(return_cycle) + " " + to_string(diff) + "\n";
    }
    
    loadfile << "Total Load Latency (cycles): " << totalLatency << endl;
    loadfile << "Avg Load Latency (cycles): " << totalLatency/load_stats_map.size() << endl;
    loadfile << "Adress Node_ID Issue_Cycle Return_Cycle Latency" << endl;

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
  return descq->insert(d, NULL);
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
    descq->recv_map.insert({d->desc_id,d});
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
  while(true) {
    if (pq.empty() || pq.top().second >= cycles) {
      break;
    }    
    execution_set.insert(pq.top().first); //insert, sorted by desc_id   
    pq.pop();   
  }

  //go through execution set sorted in desc id
  //this also allows us to send out as many staddr instructions (that become SAB head) back to back
  for(auto it=execution_set.begin(); it!=execution_set.end();) {   
    if(execute(*it)) {     
      it=execution_set.erase(it); //gets next iteration
    }
    else {     
      ++it;
    }
  }
  
  //process terminal loads
  //has already accessed mem hierarchy, which calls callback to remove from TLBuffer and decrement term_ld_count 
  //must stall if no space in commQ or still waiting for mem results
  for(auto it=TLBuffer.begin(); it!=TLBuffer.end();) {   
    DynamicNode* d=it->second;
    
    if(d->mem_status==PENDING) { //still awaiting mem response   
      ++it; 
    }      
    else { //can insert in commQ      
      d->mem_status=NONE;
      it=TLBuffer.erase(it); //get next item
      assert(commQ.find(d->desc_id)==commQ.end());
      commQ[d->desc_id]=d;
      d->c->insertQ(d);     
      term_ld_count--; //free up space in term ld buff
    }
  }
  cycles++;
}

DynamicNode* DESCQ::sab_has_dependency(DynamicNode* d) {
  //note:SAB is sorted by desc_id, which also sorts by program order
  for(auto it=SAB.begin(); it!=SAB.end();++it) {
    DynamicNode* store_d = it->second; 
    if(*d < *store_d) { //everything beyond this will be younger
      return NULL;
    }
    else if(store_d->addr==d->addr) { //older store with matching address
      return store_d;     
    }
  }
  return NULL;
}

bool DESCQ::execute(DynamicNode* d) {
  if(d->n->typeInstr==LD_PROD) {    
    if(d->mem_status==NONE || d->mem_status==DESC_FWD || d->mem_status==FWD_COMPLETE) {     
      d->c->insertQ(d);
      return true;
    }
    else { //pending here
      TLBuffer.insert({d->desc_id,d});    
      return true;
    }
  }
  else if (d->n->typeInstr==SEND) {
    assert(d->mem_status==NONE);
    d->c->insertQ(d);
    return true; 
  }
  else if (d->n->typeInstr==RECV) {
    if(commQ.find(d->desc_id)==commQ.end() || !commQ[d->desc_id]->completed) { //RECV too far ahead
      return false;
    }

    if(commQ[d->desc_id]->mem_status==FWD_COMPLETE || commQ[d->desc_id]->mem_status==NONE) { //data is ready in commQ from a forward or completed terminal load or regular produce

      if(commQ[d->desc_id]->mem_status==FWD_COMPLETE) { //stl forwarding was used        
        assert(STLMap.find(d->desc_id)!=STLMap.end());
        uint64_t stval_desc_id=STLMap[d->desc_id];
        
        assert(SVB.find(stval_desc_id)!=SVB.end());
        SVB[stval_desc_id].erase(d->desc_id); //remove desc_id of ld_prod. eventually, it'll be empty, which allows STVAL to complete
        STLMap.erase(d->desc_id); //save space       
      }
      commQ_count--; //free up commQ entry
      commQ.erase(d->desc_id);
      d->c->insertQ(d);
      return true;     
    }    
  }
  else if (d->n->typeInstr==STVAL) {
    
    d->mem_status=FWD_COMPLETE; //indicate that it's ready to forward
    //can't complete until corresponding staddr is at front of SAB
    
    if(SAB.size()==0) { //STADDR is still behind (rare, but possible)  
      return false;
    }
    
    uint64_t f_desc_id=SAB.begin()->first; //get the desc_id of front of SAB
      
    if(d->desc_id==f_desc_id && (SVB.find(d->desc_id)==SVB.end() || SVB[d->desc_id].size()==0)) { //STADDR is head of SAB and nothing to forward to RECV instr     
      SVB_count--;
      SVB_back++;
      SVB.erase(d->desc_id); //Luwa: just added
      d->c->insertQ(d);
      return true;     
    }
  }
  else if (d->n->typeInstr==STADDR) { //make sure staddrs complete after corresponding send    
    if(stval_map.find(d->desc_id)==stval_map.end()) {
      return false;
    }
    auto sab_front=SAB.begin();
    
    if(d->desc_id==sab_front->first && stval_map[d->desc_id]->mem_status==FWD_COMPLETE) {
     
      //loop through all ld_prod and mark their mem_status as fwd_complete, so recv can get the value
      for(auto it = SVB[d->desc_id].begin(); it != SVB[d->desc_id].end(); ++it ) {
        assert(commQ.find(*it)!=commQ.end());
        commQ[*it]->mem_status=FWD_COMPLETE;       
      }
    }    
    //note: corresponding sval would only have completed when this becomes front of SAB
    if(stval_map.at(d->desc_id)->completed) {
      auto sab_front=SAB.begin();
      assert(d==sab_front->second);
      //assert it's at front
      
      SAB.erase(d->desc_id); //free entry so future term loads can't find it
      SAB_count--;
      SAB_back++; //allow younger instructions to be able to enter SAB
      d->core->access(d); //now that you've gotten the value, access memory 
      d->c->insertQ(d); 
      return true;      
    }
  }
  return false;
}

//checks if there are resource limitations
//if not inserts respective instructions into their buffers
bool DESCQ::insert(DynamicNode* d, DynamicNode* forwarding_staddr) {
  bool canInsert=true;
  
  if(d->n->typeInstr==LD_PROD) {
    
    if(d->mem_status==PENDING) { //must insert in TLBuff
      if(term_ld_count==term_buffer_size || commQ_count==commQ_size) {  //check term ld buff size 
        canInsert=false;        
      }
      else {
        commQ_count++; //corresponding recv will decrement this count, signifying removing/freeing the entry from commQ
        term_ld_count++; //allocate space on TLBuff but don't insert yet until execute() to account for descq latency
      }
    }
    else { // insert in commQ directly        
      if(commQ_count==commQ_size) { //check commQ size
        canInsert=false;
      }
      else {
        if(d->mem_status==DESC_FWD) {
       
          //here, connect the queues          
          assert(STLMap.find(d->desc_id)==STLMap.end());
          STLMap[d->desc_id]=forwarding_staddr->desc_id; //this allows corresponding RECV to find desc id of STVAL to get data from
          
          //next, we set the set desc ids of pending forwards that an STVAL has to wait for before completing
          SVB[forwarding_staddr->desc_id].insert(d->desc_id);
        }
        else { //e.g., LSQ_FWD
          d->mem_status=NONE;
        }
        commQ_count++;
        assert(commQ.find(d->desc_id)==commQ.end());
        commQ[d->desc_id]=d;
      }
    }
  }
  else if(d->n->typeInstr==SEND) { 
    if(commQ_count==commQ_size) { //check commQ size
      canInsert=false;
    }
    else {
      commQ_count++;
      assert(commQ.find(d->desc_id)==commQ.end());
      commQ[d->desc_id]=d;
    }
  }
  else if(d->n->typeInstr==RECV) {    
    //no resource constraints here
  }
  else if(d->n->typeInstr==STADDR) {
    if(d->desc_id!=SAB_issue_pointer || d->desc_id>SAB_back ||  SAB_count==SAB_size) { //check SAB size, check that it's not younger than youngest allowed staddr instruction, based on SAB size, force in order issue/dispatch of staddrs
      canInsert=false;
    }
    else {
      SAB.insert({d->desc_id,d});
      SAB_count++;
      SAB_issue_pointer++;
    }
  }
  else if(d->n->typeInstr==STVAL) {
    if(d->desc_id!=SVB_issue_pointer || d->desc_id>SVB_back ||  SVB_count==SVB_size) { //check SVB size, check that it's not younger than youngest allowed stval instruction, based on SVB size, force in order issue/dispatch of stvals
      canInsert=false;
    }
    else {
      SVB_count++;
      SVB_issue_pointer++;
    }
  }
  if(canInsert) {  
    pq.push(make_pair(d,cycles+latency));
  }
  return canInsert;
}
