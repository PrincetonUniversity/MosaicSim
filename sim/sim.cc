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
#include <sstream>
#include <cmath>

using namespace std;

Simulator::Simulator(string home) {
  pythia_home=home;
  clockspeed=cfg.chip_freq;
  cache = new Cache(cfg);
  memInterface = new DRAMSimInterface(this, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->sim = this;
  cache->isLLC=true;
  cache->memInterface = memInterface;
  
  epoch_stats_out.open(outputDir+"epochStats");
  //descq = new DESCQ(cfg);
}

DESCQ* Simulator::get_descq(DynamicNode* d) {
  int tile_id =  d->core->id;
  return descq_vec.at(tile_id/2);
}

DESCQ* Simulator::get_descq(Tile* tile) {
  int tile_id =  tile->id;
  return descq_vec.at(tile_id/2);
}

bool Barrier::register_barrier(DynamicNode* d) {
  //make sure you don't have 2 barriers from same core at once
 
  if(barrier_map.find(d->core->id)!=barrier_map.end()) {
    return false;
  }

  int map_size=barrier_map.size();
  if(map_size==num_threads-1) {
 
    DynamicNode* bd;
     //free all barriers, remove from map
    for(auto it=barrier_map.begin(); it!=barrier_map.end();) {

      bd=it->second;
      bd->c->insertQ(bd); //complete instruction
      it=barrier_map.erase(it); //erase and get next barrier
    }
    d->c->insertQ(d);
    return true;
  }
  barrier_map.insert({d->core->id,d});
  return true;
}

void Simulator::registerCore(string wlpath, string cfgpath, string cfgname, int id) {
  string name = "Pythia Core "+ to_string(id);
  string gName = wlpath + "/graphOutput.txt";
  string cfName = wlpath + "/ctrl.txt";     
  string memName = wlpath + "/mem.txt";   
  string accName = wlpath + "/acc.txt";
    
  Core* core = new Core(this, clockspeed);
  core->local_cfg.read(cfgpath+cfgname);
  core->name=name;
  
  //  if(!decoupling_mode || id % 2 ==0)   
  barrier->num_threads++;
  
  // Read the Program's Static Data Dependency Graph + dynamic traces for control flow and memory
  Reader r;

  r.readGraph(gName, core->g);
  r.readProfMemory(memName, core->memory);
  r.readProfCF(cfName, core->cf);
  r.readAccTrace(accName, core->acc_map);
  
  //GraphOpt opt(core->g);
  //opt.inductionOptimization();
  core->sim=this;

  //cout << "register core id " << id << endl;
  core->initialize(id);

  //registerTile(core, id);
  registerTile(core);
  //create a descq for every 2nd core, starting with core 0
  if(id % 2 == 0) {
    descq_vec.push_back(new DESCQ(cfg));
  }

}

int transactioncount=0;
bool Simulator::InsertTransaction(Transaction* t, uint64_t cycle) {
  assert(tiles.find(t->src_id)!=tiles.end());
  assert(tiles.find(t->dst_id)!=tiles.end());  
  
  int dst_clockspeed=tiles[t->dst_id]->clockspeed;
  int src_clockspeed=tiles[t->src_id]->clockspeed;
  uint64_t dst_cycle=(dst_clockspeed*cycle)/src_clockspeed; //should round up, but no +/-1 cycle is nbd
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

void Simulator::fastForward(int src_tid, uint64_t inc) {

  assert(tiles.find(src_tid)!=tiles.end());  
  int dst_clockspeed;//=tiles[tid]->clockspeed;
  int src_clockspeed=tiles[src_tid]->clockspeed;
  dst_clockspeed=clockspeed;
  uint64_t dst_inc=(dst_clockspeed*inc)/src_clockspeed;
  
  cycles+=dst_inc;

  //recursively fast forward all the tiles
  for (auto entry:tiles) {
    Tile* tile=entry.second;
    dst_clockspeed=tile->clockspeed;
 
    dst_inc=(dst_clockspeed*inc)/src_clockspeed;
    tile->fastForward(dst_inc);
    
    //increment L2 and DRAM..assumed to be on the same clockspeed as core 0
    if(tile->id==0) {
      cache->cycles+=dst_inc;
      memInterface->fastForward(dst_inc);
    }
  }
}

bool Simulator::isLocked(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;
  return lockedLineMap.find(cacheline)!=lockedLineMap.end();
}

bool Simulator::hasLock(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;
  return lockedLineMap.find(cacheline)!=lockedLineMap.end() && lockedLineMap[cacheline]==d;
}

/*
void Simulator::releaseLock(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;
  lockedLineMap.erase(cacheline);  
}

bool Simulator::lockCacheline(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;
  bool hadLock=hasLock(d);
  if(isLocked(d) && !hadLock) { //if someone else holds lock
    if(!d->requestedLock) {//havent requested before
      lockedLineQ[cacheline].push(d); //enqueue
    }
    d->requestedLock=true;
    return false;
  }
  
  if(!isLocked(d)) { //it's not locked (could maybe add && !hadLock) 
    if(lockedLineQ.find(cacheline)!=lockedLineQ.end() && !lockedLineQ[cacheline].empty() && lockedLineQ[cacheline].front()!=d) {  //not next in line for lock
      if(!d->requestedLock) {//havent requested before
        lockedLineQ[cacheline].push(d); //enqueue
        d->requestedLock=true;
      }      
      return false;
    }
    else if (lockedLineQ.find(cacheline)!=lockedLineQ.end() && !lockedLineQ[cacheline].empty() && lockedLineQ[cacheline].front()==d) { //you're next in line
      lockedLineQ[cacheline].pop(); //dequeue
      if(lockedLineQ[cacheline].empty()) {
        lockedLineQ.erase(cacheline);
      }
      
    }    
  }
  evictAllCaches(d->addr); //upon assignment of lock, must evict all cachelines  
  lockedLineMap[cacheline]=d; //get the lock, idempotent if you already have it
  return true;
}
*/

void Simulator::releaseLock(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;

  //assign lock to next in queue

  if(lockedLineQ.find(cacheline)!=lockedLineQ.end() && !lockedLineQ[cacheline].empty()) {
    lockedLineMap[cacheline]=lockedLineQ[cacheline].front();
    lockedLineQ[cacheline].pop();
    evictAllCaches(d->addr); //upon assignment of lock, must evict all cachelines
  }
  else {
    lockedLineMap.erase(cacheline);
    lockedLineQ.erase(cacheline);
  }


}

bool Simulator::lockCacheline(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;
  if(isLocked(d) && !hasLock(d)) { //if someone else holds lock
    if(!d->requestedLock) {//havent requested before
      lockedLineQ[cacheline].push(d); //enqueue
    }
    d->requestedLock=true;
    return false;
  }
  d->requestedLock=true;
  //if no one holds the lock, must evict cachelines
  //if you hold the lock, that was done right when you were given the lock (i.e., in the code line above or when the last owner released the lock)
  if(!isLocked(d)) {
    evictAllCaches(d->addr); //pass in address, not cacheline
  }

  lockedLineMap[cacheline]=d; //get the lock, idempotent if you already have it
  return true;
}

void Simulator::evictAllCaches(uint64_t addr) {

  for(auto id_tile: tiles) {
    if(Core* core=dynamic_cast<Core*>(id_tile.second)) {
      core->cache->evict(addr);      
    }
  }
  
  /* test that eviction worked
  for(auto id_tile: tiles) {
    if(Core* core=dynamic_cast<Core*>(id_tile.second)) {
      assert(!core->cache->fc->access(addr/core->cache->size_of_cacheline, true));
      //should not be in cacheline anymore!
    }
  }
  */
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
  memInterface->initialize(clockspeed);
}

//return tile id of an accelerator tile
//future version will allow you to specify which kind of accelerator you're looking for
int Simulator::getAccelerator(){
  for(auto it=tiles.begin(); it!=tiles.end(); ++it) {
    if(it->second->name=="accelerator")
      return it->second->id;
  }
  return -1;
}

void Simulator::run() {

  std::vector<int> processVec(tiles.size(), 0);
  int simulate = 1;
  clockspeed=findlcm(clockspeedVec,clockspeedVec.size());

  cout << "[SIM] ------- Starting Simulation!!! ------------------------" << endl;
  
  last_time = Clock::now();  
  while(simulate > 0) {

    if(simulate==0) {
      break;
    }
    for (auto it=tiles.begin(); it!=tiles.end(); ++it) {
    
      Tile* tile = it->second;
      uint64_t norm_tile_frequency=clockspeed / tile->clockspeed; //normalized cloockspeed. i.e., update every x cycles. note: automatically always an integer because of global clockspeed is lcm of local clockspeeds

      //time for tile to be called
      if(cycles%norm_tile_frequency==0) {
        
        vector<pair<Transaction*,uint64_t>> rejected_transactions;         
        //assume tiles never go from completed (process() returning false) back to not completed (process() returning true)

        //only core tiles can affect whether to continue processing, accelerator tiles return false

        //register activity of tile
        processVec.at(it->first)=tile->process();
        
        //process transactions
        priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare>& pq=transq_map[tile->id];
        while(true) {
          if(pq.empty() || pq.top().second >= tile->cycles) {
           
            break;
          }
          Transaction* t=pq.top().first;
          
          uint64_t tcycles=pq.top().second;
         
          if(!tile->ReceiveTransaction(t)) {
            rejected_transactions.push_back({t,tcycles});
          }      
          pq.pop();    
        }
        //push back all the rejected transactions
        for(auto& trans_cycle: rejected_transactions) {
          pq.push(trans_cycle);
        }
        rejected_transactions.clear();
        
        //use same clockspeed as 1st tile (probably a core)
        if(tile->id==0) {
          cache->process();          
          memInterface->process();                  
        }                
      }
    }
    simulate = accumulate(processVec.begin(), processVec.end(), 0);
    
    // Print GLOBAL stats every "stat.printInterval" cycles
    if(tiles[0]->cycles % stat.printInterval == 0 && tiles[0]->cycles !=0) {
      
      curr_time = Clock::now();
      uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count();
      //uint64_t tdiff_mins = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count();
      double instr_rate = ((double)(stat.get("total_instructions") - last_instr_count)) / tdiff;
      cout << "\nGlobal Simulation Speed: " << instr_rate << " Instructions per ms \n";
      
      uint64_t remaining_instructions = total_instructions - last_instr_count; 
      
      double remaining_time = (double)remaining_instructions/(60000*instr_rate);
      cout << "Remaining Time: " << (int)remaining_time << " mins \nRemaining Instructions: " << remaining_instructions << endl;
      
      last_instr_count = stat.get("total_instructions");
      last_time = curr_time;
      
      //stat.print_epoch(cout);
    }
    else if(tiles[0]->cycles == 0) {
      last_time = Clock::now();
      last_instr_count = 0;
    }

    if(tiles[0]->cycles % 10000 == 0 && tiles[0]->cycles !=0) {
      stat.print_epoch(epoch_stats_out);
    }
    cycles++;
  }
  cout << "[SIM] ------- End of Simulation!!! ------------------------" << endl << endl;

  // print stats for each pythia tile
  for (auto it=tiles.begin(); it!=tiles.end(); it++) {
    Tile* tile=it->second;
    if(Core* core=dynamic_cast<Core*>(tile)) {
      // update cycle stats
      stat.set("cycles", core->cycles);
      core->local_stat.set("cycles", core->cycles);
      // print stats
      cout << "------------- Final " << core->name << " Stats --------------\n";
      core->local_stat.print(cout);
      // calculate energy & print
      core->calculateEnergyPower();
      cout << "total_energy : " << core->total_energy << " Joules\n";
      cout << "avg_power : " << core->avg_power << " Watts\n";
    }
  }
  
  cout << "\n----------------GLOBAL STATS--------------\n";
  stat.print(cout);
  calculateGlobalEnergyPower();
  cout << "global_energy : " << stat.global_energy << " Joules\n";
  cout << "global_avg_power : " << stat.global_avg_power << " Watts\n";
  memInterface->mem->printStats(true);
  curr_time=Clock::now();
  uint64_t tdiff_mins = chrono::duration_cast<std::chrono::minutes>(curr_time - init_time).count();
  uint64_t tdiff_seconds = chrono::duration_cast<std::chrono::seconds>(curr_time - init_time).count();
  uint64_t tdiff_milliseconds = chrono::duration_cast<std::chrono::milliseconds>(curr_time - init_time).count();
  if(tdiff_mins>5) {
    cout << "Total Simulation Time: " << tdiff_mins << " mins \n";
  }
  else if(tdiff_seconds>0) {
    cout << "Total Simulation Time: " << tdiff_seconds << " secs \n";
  }
  else
    cout << "Total Simulation Time: " << tdiff_milliseconds << " ms \n";
  cout << "Average Global Simulation Speed: " << 1000*total_instructions/tdiff_milliseconds << " Instructions per sec \n";

  
  
  
  // DESCQ* descq=descq_vec.at(0);

  if(runaheadVec.size()>0) {
    ofstream outfile;
    outfile.open(outputDir+"decouplingStats");
    long long send_runahead_sum=0;
    string outstring="";
    outfile << "Total Recv Latency (cycles): " + to_string(total_recv_latency) + "\n";
    outfile << "Avg Recv Latency (cycles): " + to_string((long long)total_recv_latency/runaheadVec.size()) + "\n";

    for(auto entry:runaheadVec) {
      
      outstring+=to_string(entry.nodeId) + " " + to_string(entry.coreId) + " " + to_string(entry.runahead) + "\n";
      send_runahead_sum+=entry.runahead;      
    }

    outfile<<"Total Runahead Distance (cycles): " << send_runahead_sum << "\n";
    outfile << "Number of Receive_Instructions: " << runaheadVec.size() << "\n";
    outfile << "Average Runahead Distance(cycles): " << send_runahead_sum/(long long)runaheadVec.size() << endl; 
    outfile << "NODE_ID CORE_ID RUNAHEAD_DIST" << endl;
    outfile << outstring;
  }
  /*
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
*/
  //calculate mlp stats
  string median_mlp_prefix="";
  median_mlp_prefix+= "Median # DRAM Accesses Per " + to_string(mlp_epoch) + "-cycle Epoch: ";
  string max_mlp_prefix="";
  max_mlp_prefix+= "Max # DRAM Accesses Per " + to_string(mlp_epoch) + "-cycle Epoch: ";

  string mean_mlp_prefix="";
  mean_mlp_prefix+= "Mean # DRAM Accesses Per " + to_string(mlp_epoch) + "-cycle Epoch: ";
  
  sort(accesses_per_epoch.begin(), accesses_per_epoch.end());
  //  for(auto epoch:accesses_per_epoch) {
  //  cout << epoch << endl;
  //}
  
  
  //calculate and print stats on load latencies
  if(load_stats_vector.size()>0) {
    
    long long totalLatency=0;
    string outstring="";
    for(auto load_stat:load_stats_vector) {

      long long issue_cycle=load_stat.issueCycle;
      long long return_cycle=load_stat.completeCycle;
      
      string isHit="Miss";
      if (load_stat.hit) {
        isHit="Hit";
      }
      string MEMOP="";
      if (load_stat.type==LD) {
        MEMOP="LD";
      }
      else if (load_stat.type==LD_PROD) {
        MEMOP="LD_PROD";
      }
      else if (load_stat.type==ST) {
        MEMOP="ST";
      }
      else if (load_stat.type==STADDR) {
        MEMOP="ST_ADDR";
      }
      else if (load_stat.type==ATOMIC_ADD) {
        MEMOP="ATOMIC_ADD";
      }
      else if (load_stat.type==ATOMIC_FADD) {
        MEMOP="ATOMIC_FADD";
      }
      else if (load_stat.type==ATOMIC_MIN) {
        MEMOP="ATOMIC_MIN";
      }
      else if (load_stat.type==ATOMIC_CAS) {
        MEMOP="ATOMIC_CAS";
      }
      else if (load_stat.type==TRM_ATOMIC_FADD) {
        MEMOP="TRM_ATOMIC_FADD";
      }
      else if (load_stat.type==TRM_ATOMIC_MIN) {
        MEMOP="TRM_ATOMIC_MIN";
      }
      else if (load_stat.type==TRM_ATOMIC_CAS) {
        MEMOP="TRM_ATOMIC_CAS";
      }
      else {
        assert(false);
      }
      int node_id=load_stat.nodeId;
      long long diff=(return_cycle-issue_cycle);
      totalLatency=totalLatency + diff;
      
      //cout << "ret: " << return_cycle << ", issue: "<< issue_cycle << ", diff: " << diff << endl;
      //assert(diff>0);
      outstring+=MEMOP+" "+to_string(load_stat.addr) + " " + to_string(node_id) + " " + to_string(issue_cycle) + " " + to_string(return_cycle) + " " + to_string(diff) + " " + isHit + "\n";
    }
        
    ofstream loadfile;
    loadfile.open(outputDir+"memStats");
    
    if(accesses_per_epoch.size()==0) {
      loadfile << mean_mlp_prefix << 0 << endl;
      loadfile << median_mlp_prefix << 0 << endl;
      loadfile << max_mlp_prefix << 0 << endl;
    }
    else {
      loadfile << mean_mlp_prefix << accumulate(accesses_per_epoch.begin(), accesses_per_epoch.end(),0)/accesses_per_epoch.size() << endl;
      loadfile << median_mlp_prefix << accesses_per_epoch[accesses_per_epoch.size()/2] << endl;
      loadfile << max_mlp_prefix << accesses_per_epoch[accesses_per_epoch.size()-1] << endl;
    }
    
    
    loadfile << "Total Mem Access Latency (cycles): " << totalLatency << endl;
    loadfile << "Avg Mem Access Latency (cycles): " << totalLatency/load_stats_vector.size() << endl;
    
    

    loadfile << "Memop Adress Node_ID Issue_Cycle Return_Cycle Latency L1_Hit/Miss" << endl;
    loadfile << outstring;
  }
  //cout << "DeSC Forward Count: " << desc_fwd_count << endl;
  //cout << "Number of Vector Entries: " << load_count << endl; 
} 

void Simulator::calculateGlobalEnergyPower() {
  stat.global_energy = 0.0;
  int n_cores = 0;
  for (auto it=tiles.begin(); it!=tiles.end(); it++) {
    Tile* tile=it->second;
    if(Core* core=dynamic_cast<Core*>(tile)) {
      // aggregate all of the per-core energy
      n_cores++;
      if(core->total_energy==0.0)
        core->calculateEnergyPower();
      stat.global_energy += core->total_energy;
    }
  }
  double e = stat.global_energy;

  // Add the L2 cache energy (we assume it is shared - NOTE this can change in a future)
  double L2_energy = (stat.get("l2_hits") + stat.get("l2_misses") ) * cfg.energy_per_L2_access.at(cfg.technology_node);
  stat.global_energy += L2_energy;      

  // Add the DRAM energy
  //Note: dram_accesses is on a cache line granularity
  double DRAM_energy = stat.get("dram_accesses") * cfg.energy_per_DRAM_access.at(cfg.technology_node);
  stat.global_energy += DRAM_energy;

  // For Luwa: 
  // Add accelerators energy 
  double Acc_energy = stat.acc_energy;
  stat.global_energy += Acc_energy;      

  // Finally, calculate Avg Power (in Watts)
  stat.global_avg_power = stat.global_energy * clockspeed*1e+6 / cycles;   // clockspeed is defined in MHz

  // some debug stuff
    uint64_t total_flops=stat.get("FP_ADDSUB")+stat.get("FP_MULT")+stat.get("FP_REM");
  uint64_t total_gflops=total_flops/(1e9);

  cout << "Total GFLOPs : " << total_gflops << endl;
  
  cout << "-------All (" << n_cores << ") cores energy (J) : " << e << endl;
  cout << "-------L2_energy (J) : " << L2_energy << endl;
  cout << "-------DRAM_energy (J) : " << DRAM_energy << endl;
  cout << "-------Acc_energy (J) : " << Acc_energy << endl;
}

bool Simulator::canAccess(Core* core, bool isLoad) {
  Cache* cache = core->cache;
  if(isLoad)
    return cache->free_load_ports > 0 || cache->load_ports==-1;
  else
    return cache->free_store_ports > 0 || cache->store_ports==-1;
}

bool Simulator::communicate(DynamicNode* d) {
  DESCQ* descq=get_descq(d);
  return descq->insert(d, NULL);
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
  //should not be able to execute atomic instruction if can't get lock
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

void Simulator::orderDESC(DynamicNode* d) {
  DESCQ* descq=get_descq(d);
  if(d->n->typeInstr == SEND) {
    //descq->send_map.insert(make_pair(descq->last_send_id,d));
    d->desc_id=descq->last_send_id;
    descq->last_send_id++;
  }
  if(d->n->typeInstr == LD_PROD || d->atomic) {
    //descq->send_map.insert(make_pair(descq->last_send_id,d));
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
    //descq->recv_map.insert({d->desc_id,d});
  }
  else if (d->n->typeInstr == STADDR) {
    d->desc_id=descq->last_staddr_id;
    descq->last_staddr_id++;
  }
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
  //if you're atomic, I wanna do an if
  if(d->atomic) {    
    
    d->core->access(d);
    d->mem_status=PENDING;
    TLBuffer.insert({d->desc_id,d});    
    return true;
    
  }
  else if(d->n->typeInstr==LD_PROD) {    
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

    //if(d->desc_id==sab_front->first && stval_map[d->desc_id]->mem_status==FWD_COMPLETE) {
    if(SVB.find(d->desc_id)!=SVB.end()) {
      //loop through all ld_prod and mark their mem_status as fwd_complete, so recv can get the value
      for(auto it = SVB[d->desc_id].begin(); it != SVB[d->desc_id].end(); ++it ) {
        assert(commQ.find(*it)!=commQ.end());
        
        commQ[*it]->mem_status=FWD_COMPLETE;       
      }
    }
    
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
    //auto sab_front=SAB.begin();
        
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
  
  if(d->n->typeInstr==LD_PROD || d->atomic) {
    
    if(d->mem_status==PENDING || d->atomic) { //must insert in TLBuff
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
