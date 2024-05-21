#include "sim.hpp"
#include "memsys/DRAM.hpp"
#include "omp.h"
#include "tile/Accelerator.hpp"
class Accelerator;


#if EXPERIMENTAL_FILESYSTEM == 1
#include <experimental/filesystem>
using namespace std::experimental::filesystem;
#else
#include <filesystem>
using namespace std::filesystem;
#endif

using namespace std;

bool Barrier::register_barrier(DynamicNode* d) {
  /* make sure you don't have 2 barriers from same core at once */
  if(barrier_map.find(d->core->id) != barrier_map.end()) {
    return false;
  }

  int map_size=barrier_map.size();
  /* If we are the last one */
  if(map_size==num_cores-1) {
    /* free all barriers, remove from map */
    for(auto it=barrier_map.begin(); it!=barrier_map.end();) {
      DynamicNode* bd;
      bd=it->second;
      /* barrier release operation. Note we "add -1" to the latency,
	 beacuse we want to be excecuted during next cycle (barrier
	 instruction latency is 1). */
      bd->extra_lat = -1;
#pragma omp critical (barrier_instruction)      
      bd->c->to_insert = bd;
      /* erase and get next barrier */
      it = barrier_map.erase(it);
    }
    /* no extra latency here, beacuase current cycle + 1 (cost of the
       barrier) is next cycle */
    d->c->insertQ(d);
    return true;
  }
  barrier_map.insert({d->core->id,d});
  return true;
}

bool Simulator::register_partial_barrier(DynamicNode* d, int id, int num_cores) {
  map<int, DynamicNode*> &partial_barrier_map = partial_barrier_maps.at(id); 
  /* make sure you don't have 2 barriers from same core at once */
  if(partial_barrier_map.find(d->core->id) != partial_barrier_map.end()) {
    return false;
  }

  int map_size=partial_barrier_map.size();
  /* If we are the last one */
  if(map_size==num_cores-1) {
    /* free all barriers, remove from map */
    for(auto it=partial_barrier_map.begin(); it!=partial_barrier_map.end();) {
      DynamicNode *bd =it->second;
      /* barrier release operation. Note we "add -1" to the latency,
	 beacuse we want to be excecuted during next cycle (barrier
	 instruction latency is 1). */
      bd->extra_lat = -1;
#pragma omp critical (barrier_instruction)
      bd->c->to_insert = bd;
      /* erase and get next barrier */
      it = partial_barrier_map.erase(it);
    }
    /* no extra latency here, beacuase current cycle  1 (cost of the
       barrier) is next cycle */
    d->c->insertQ(d);
    return true;
  }
  partial_barrier_map.insert({d->core->id,d});
  return true;
}

Simulator::Simulator(string config, string DRAM_system, string DRAM_device ) {
  cfg = new Config();
  global_stat = new Statistics();
}

void Simulator::registerCore(string wlpath, string cfgfile, PP_static_Buff<string> *acc_comm) {
  int id = tileCount;
  string name = "MosaicSim Core " + to_string(id);
  string gName = wlpath + "/graphOutput.txt";
  string DynDataName = wlpath + "/dyn_data.txt";
  string CFName = wlpath + "/ctrl.txt";
  string memName = wlpath + "/mem.txt";
  string accName = wlpath + "/acc.txt";
  string PBName = wlpath + "/PB.txt";

  Core* core = new Core(this, clockspeed, id < nb_cores/2);
  core->local_cfg.read(cfgfile);
  core->name=name;
  
  barrier->num_cores++;
  Reader r;
  r.readGraph(gName, core->g, this->cfg);
  
  if (input_files_type == 2) {
    r.readProfMemory(memName, core->memory);
    r.readProfCF(CFName, core->cf, core->bb_cond_destinations);
    core->finished_dyn_data = true;
  }

  core->initialize(id, this, acc_comm);
  registerTile(core);

  /* create a descq for the first half of nodes.  */
  if(id < (nb_cores+1)/2) {
    core->descq = new DESCQ(cfg);
  } else {
    if (Core* previouse_core = dynamic_cast<Core*>(tiles[id - nb_cores/2])) {
      core->descq = previouse_core->descq;
    } else {
      assert(false);
    }
  }
}

bool Simulator::InsertTransaction(Transaction* t, uint64_t cycle) {
  assert(tiles.find(t->src_id)!=tiles.end());
  assert(tiles.find(t->dst_id)!=tiles.end());
  
  int dst_clockspeed=tiles[t->dst_id]->clockspeed;
  int src_clockspeed=tiles[t->src_id]->clockspeed;
  uint64_t dst_cycle=(dst_clockspeed*cycle)/src_clockspeed; //should round up, but no +/-1 cycle is nbd
  uint64_t final_cycle = dst_cycle + transq_latency;
  
  transq_map[t->dst_id].push({t,final_cycle});
  
  return true;
}

void Simulator::fastForward(int src_tid, uint64_t inc) {
  assert(tiles.find(src_tid)!=tiles.end());
  int dst_clockspeed = clockspeed;
  int src_clockspeed = tiles[src_tid]->clockspeed;
  uint64_t dst_inc=(dst_clockspeed*inc)/src_clockspeed;
  
  cycles+=dst_inc;

  /* recursively fast forward all the tiles */
  for (auto entry:tiles) {
    Tile* tile=entry.second;
    dst_clockspeed=tile->clockspeed;
 
    dst_inc=(dst_clockspeed*inc)/src_clockspeed;
    tile->fastForward(dst_inc);
    
    /* increment L2 and DRAM..assumed to be on the same clockspeed as
       core 0 */
    if(tile->id==0) {
      cache->cycles+=dst_inc;
      memInterface->fastForward(dst_inc);
    }
  }
}

/* tile ids must be non repeating */
void Simulator::registerTile(Tile* tile) {
  assert(tiles.find(tileCount)==tiles.end());
  
  tile->id=tileCount;
  tiles[tileCount]=tile;
  tileCount++;
  tile->sim=this;
  clockspeedVec.push_back(tile->clockspeed);
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

static bool
check_all(bool *cond, int size)
{
  for(int i = 0; i < size; i++)
    if (!cond[i]) return false;
  return true;
}

void
Simulator::read_dyn(int dyn_file) {
  int buf[1024];
  int bytes, size = 1024 * sizeof(int);

  bytes = read(dyn_file, buf, size);
  if(bytes < 1)
    return;
  assert(bytes == size);

  int id = buf[0];
  
  if(Core* core=dynamic_cast<Core*>(tiles[id])) {
    core->read_dyn_data(buf);
  } else {
    assert(false);
  }
}

uint64_t findlcm(vector<uint64_t> numbers)
{
  // Initialize result
  uint64_t ans = numbers[0];

  // ans contains LCM of numbers[0], ..numbers[i]
  // after i'th iteration,
  for (int i = 1; i < numbers.size(); i++)
    ans = (((numbers[i] * ans)) /
           (gcd(numbers[i], ans)));

  return ans;
}

void Simulator::run() {
  int nb_tiles = tiles.size();
  vector<int> processVec(nb_tiles+1, 0);
  int simulate = 1;
  init_time=chrono::high_resolution_clock::now();
  clockspeed = findlcm(clockspeedVec);

  // Stats files
  //TODO: REINTRODUCE_STATS
  ofstream decouplingStats;
  long long send_runahead_sum = 0;
  string decouplingOutstring;
  /* int runaheadVec_size = 0; */

  ofstream memStats;
  long long totalLatency = 0;
  string memOutstring;
  int load_stats_vector_size = 0;
  /* epoch_stats_out.open(outputDir+"epochStats"); */

  cout << "[SIM] ------- Starting Simulation!!! ------------------------" << endl;
  last_time = Clock::now();

  // extra threads for the LLC and DRAM
  int nb_threads = nb_files + 1;
  // vector<vector<uint64_t>> exe_time;
  // for(int i = 0; i < nb_threads; i++)
  //   exe_time.push_back(vector<uint64_t>());

  int maxLocalCores = ceil((nb_tiles-1)/nb_files);
#pragma omp parallel num_threads(nb_threads) firstprivate(nb_tiles) default(shared)
  {
    int thread = omp_get_thread_num();
    int first = thread * maxLocalCores;
    int last  = (thread+1) * maxLocalCores;
    vector<int> coreDist;
    last = last > nb_cores ? nb_cores : last;
    
    for(int i = first; i < last; i++)
      coreDist.push_back(i);
    
    while(simulate > 0 ) {
      /* auto start = chrono::steady_clock::now(); */
      // random_shuffle(coreDist.begin(), coreDist.end());
      for(int &id: coreDist) {
	if (id >= nb_cores)
	  break;
	int file;
	Tile *tile = tiles.at(id);
	if (input_files_type == 0)
	  file = dyn_files.at(thread);
	else if (input_files_type = 1)
	  file = dyn_files.at(id);
	
	if (Core* core=dynamic_cast<Core*>(tile)) 
	  while((core->cf.size() + core->deleted_cf) < core->total_created_contexts+2 && !core->finished_dyn_data)
	    read_dyn(file);
      }
      
      if( thread == nb_threads-1 )  {
	uint64_t norm_tile_frequency = clockspeed / tiles.at(0)->clockspeed;
	if(cycles % norm_tile_frequency == 0) {
	  memInterface->process();
	  processVec.at(nb_tiles) = cache->process();
	  /** accelerator tile */
	  processVec.at(nb_tiles-1)  = tiles.at(nb_cores)->process();
	}
      } else {
	for(int &id: coreDist) {
	  Tile* tile = tiles.at(id);
	  uint64_t norm_tile_frequency = clockspeed / tile->clockspeed;
	  /* time for tile to be called */
	  if(cycles % norm_tile_frequency == 0) 
	    processVec.at(id)  = tile->process();
	}
      }
      /* auto end = chrono::steady_clock::now(); */
      /* exe_time.at(thread).push_back(chrono::duration_cast<chrono::microseconds>(end - start).count()); */
#pragma omp barrier
#pragma omp single
      {
	/* simulation will continue while there is work to do on
	   tiles or LLC  */
	simulate = accumulate(processVec.begin(), processVec.end(), 0);
	cycles++;
      }
    }
  }
  
    // Print GLOBAL stats every "stat.printInterval" cycles
    /* if(tiles[0]->cycles % global_stat->printInterval == 0 && tiles[0]->cycles !=0) { */
      
    /*   curr_time = Clock::now(); */
    /*   uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count(); */
    /*   //uint64_t tdiff_mins = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count(); */
    /*   double instr_rate = ((double)(global_stat->get("total_instructions") - last_instr_count)) / tdiff; */
    /*   cout << "\nGlobal Simulation Speed: " << instr_rate << " Instructions per ms \n"; */

    /*   last_instr_count = global_stat->get("total_instructions"); */
    /*   last_time = curr_time; */
      
    /*   // decouplingStats chunking */
    /*   if(runaheadVec.size() > 0) {  */
    /*     ifstream decouplingStatsIn(outputDir+"decouplingStats"); */
    /*     if (!decouplingStatsIn) { */
    /*       decouplingStats.open(outputDir+"decouplingStats"); */
    /*       decouplingStats << "NODE_ID CORE_ID RUNAHEAD_DIST" << endl; */
    /*     } */

    /*     decouplingOutstring = ""; */
    /*     for(auto it = runaheadVec.begin(); it != runaheadVec.end(); it++) { */
    /*       auto entry = *it; */
    /*       decouplingOutstring += to_string(entry.nodeId) + " " + to_string(entry.coreId) + " " + to_string(entry.runahead) + "\n"; */
    /*       send_runahead_sum += entry.runahead;       */
    /*     } */
          
    /*     runaheadVec_size += runaheadVec.size(); */
    /*     runaheadVec.clear(); */
    /*     decouplingStats << decouplingOutstring; */
    /*   } */

    /*   // memStats chunking */
    /*   if(load_stats_vector.size() > 0) { */
    /*     ifstream memStatsIn(outputDir+"memStats"); */
    /*     if (!memStatsIn) { */
    /*       memStats.open(outputDir+"memStats");       */
    /*       memStats << "Memop Address Node_ID Issue_Cycle Return_Cycle Latency L1_Hit/Miss" << endl; */
    /*     } */

    /*     long long issue_cycle, return_cycle, diff; */
    /*     int node_id; */
    /*     memOutstring = ""; */
    /*     for(auto load_stat:load_stats_vector) { */
    /*       issue_cycle=load_stat.issueCycle; */
    /*       return_cycle=load_stat.completeCycle; */
      
    /*       string isHit="N/A"; */
    /*       //string isHit="Miss"; */
    /*       if (load_stat.hit == 0) { */
    /*         isHit="Miss"; */
    /*       } else if (load_stat.hit == 1) { */
    /*         isHit="Hit"; */
    /*       } */
    /*       string MEMOP=""; */
    /*       if (load_stat.type==LD) { */
    /*         MEMOP="LD"; */
    /*       } else if (load_stat.type==LD_PROD) { */
    /*         MEMOP="LD_PROD"; */
    /*       } else if (load_stat.type==ST) { */
    /*         MEMOP="ST"; */
    /*       } else if (load_stat.type==STADDR) { */
    /*         MEMOP="ST_ADDR"; */
    /*       } else if (load_stat.type==ATOMIC_ADD) { */
    /*         MEMOP="ATOMIC_ADD"; */
    /*       } else if (load_stat.type==ATOMIC_FADD) { */
    /*         MEMOP="ATOMIC_FADD"; */
    /*       } else if (load_stat.type==ATOMIC_MIN) { */
    /*         MEMOP="ATOMIC_MIN"; */
    /*       } else if (load_stat.type==ATOMIC_CAS) { */
    /*         MEMOP="ATOMIC_CAS"; */
    /*       } else if (load_stat.type==TRM_ATOMIC_FADD) { */
    /*         MEMOP="TRM_ATOMIC_FADD"; */
    /*       } else if (load_stat.type==TRM_ATOMIC_MIN) { */
    /*         MEMOP="TRM_ATOMIC_MIN"; */
    /*       } else if (load_stat.type==TRM_ATOMIC_CAS) { */
    /*         MEMOP="TRM_ATOMIC_CAS"; */
    /*       } else { */
    /*         assert(false); */
    /*       } */

    /*       node_id=load_stat.nodeId; */
    /*       diff=(return_cycle-issue_cycle); */
    /*       totalLatency=totalLatency + diff; */
      
    /*       memOutstring+=MEMOP+" "+to_string(load_stat.addr)+" "+to_string(node_id)+" "+to_string(issue_cycle)+" "+to_string(return_cycle)+" "+to_string(diff)+" "+isHit+"\n"; */
    /*     } */
    
    /*     load_stats_vector_size += load_stats_vector.size(); */
    /*     load_stats_vector.clear(); */
    /*     memStats << memOutstring; */
    /*   } */
    /* } else if(tiles[0]->cycles == 0) { */
    /*   last_time = Clock::now(); */
    /*   last_instr_count = 0; */
    /* } */

    /* if(tiles[0]->cycles % 10000 == 0 && tiles[0]->cycles !=0) { */
    /*   global_stat->print_epoch(epoch_stats_out); */
    /* } */
    
  /* } */

  /* ofstream timing("timing.txt"); */
      /* for (uint64_t i = 0; i < cycles; i++) {  */
      /* 	for (int j = 0; j < nb_threads; j++)  */
      /* 	  timing << exe_time.at(j).at(i) << "\t"; */
      /* 	timing << endl; */
      /* } */
      

  curr_time=Clock::now();
  printStats();

  //TODO: REINTRODUCE_STATS
  /* // decouplingStats chunking */
  /* if(runaheadVec.size() > 0) {  */
  /*   ifstream decouplingStatsIn(outputDir+"decouplingStats"); */
  /*   if (!decouplingStatsIn) { */
  /*     decouplingStats.open(outputDir+"decouplingStats"); */
  /*     decouplingStats << "NODE_ID CORE_ID RUNAHEAD_DIST" << endl; */
  /*   } */

  /*   decouplingOutstring = ""; */
  /*   for(auto it = runaheadVec.begin(); it != runaheadVec.end(); it++) { */
  /*     auto entry = *it; */
  /*     decouplingOutstring += to_string(entry.nodeId) + " " + to_string(entry.coreId) + " " + to_string(entry.runahead) + "\n"; */
  /*     send_runahead_sum += entry.runahead;       */
  /*   } */
          
  /*   runaheadVec_size += runaheadVec.size(); */
  /*   runaheadVec.clear(); */
  /*   decouplingStats << decouplingOutstring; */
  /* } */

  /* // memStats chunking */
  /* if(load_stats_vector.size() > 0) { */
  /*   ifstream memStatsIn(outputDir+"memStats"); */
  /*   if (!memStatsIn) { */
  /*     memStats.open(outputDir+"memStats");       */
  /*     memStats << "Memop Adress Node_ID Issue_Cycle Return_Cycle Latency L1_Hit/Miss" << endl; */
  /*   } */

  /*   long long issue_cycle, return_cycle, diff; */
  /*   int node_id; */
  /*   memOutstring = ""; */
  /*   for(auto load_stat:load_stats_vector) { */
  /*     issue_cycle=load_stat.issueCycle; */
  /*     return_cycle=load_stat.completeCycle; */
      
  /*     string isHit="N/A"; */
  /*     //string isHit="Miss"; */
  /*     if (load_stat.hit == 0) { */
  /*       isHit="Miss"; */
  /*     } else if (load_stat.hit == 1) { */
  /*       isHit="Hit"; */
  /*     } */
  /*     string MEMOP=""; */
  /*     if (load_stat.type==LD) { */
  /*       MEMOP="LD"; */
  /*     } else if (load_stat.type==LD_PROD) { */
  /*       MEMOP="LD_PROD"; */
  /*     } else if (load_stat.type==ST) { */
  /*       MEMOP="ST"; */
  /*     } else if (load_stat.type==STADDR) { */
  /*       MEMOP="ST_ADDR"; */
  /*     } else if (load_stat.type==ATOMIC_ADD) { */
  /*       MEMOP="ATOMIC_ADD"; */
  /*     } else if (load_stat.type==ATOMIC_FADD) { */
  /*       MEMOP="ATOMIC_FADD"; */
  /*     } else if (load_stat.type==ATOMIC_MIN) { */
  /*       MEMOP="ATOMIC_MIN"; */
  /*     } else if (load_stat.type==ATOMIC_CAS) { */
  /*       MEMOP="ATOMIC_CAS"; */
  /*     } else if (load_stat.type==TRM_ATOMIC_FADD) { */
  /*       MEMOP="TRM_ATOMIC_FADD"; */
  /*     } else if (load_stat.type==TRM_ATOMIC_MIN) { */
  /*       MEMOP="TRM_ATOMIC_MIN"; */
  /*     } else if (load_stat.type==TRM_ATOMIC_CAS) { */
  /*       MEMOP="TRM_ATOMIC_CAS"; */
  /*     } else { */
  /*       assert(false); */
  /*     } */

  /*     node_id=load_stat.nodeId; */
  /*     diff=(return_cycle-issue_cycle); */
  /*     totalLatency=totalLatency + diff; */
      
  /*     memOutstring+=MEMOP+" "+to_string(load_stat.addr)+" "+to_string(node_id)+" "+to_string(issue_cycle)+" "+to_string(return_cycle)+" "+to_string(diff)+" "+isHit+"\n"; */
  /*   } */
    
  /*   load_stats_vector_size += load_stats_vector.size(); */
  /*   load_stats_vector.clear(); */
  /*   memStats << memOutstring; */
  /* } */

  //TODO: REINTRODUCE_STATS
  /* // print summary stats on decoupling */
  /* ifstream decouplingStatsIn(outputDir+"decouplingStats"); */
  /* if (decouplingStatsIn) { */
  /*   decouplingStats << "\n"; */
  /*   decouplingStats << "SUMMARY\n"; */
  /*   decouplingStats << "Total Recv Latency (cycles): " + to_string(total_recv_latency) + "\n"; */
  /*   if(runaheadVec_size) */
  /*     decouplingStats << "Avg Recv Latency (cycles): " + to_string((long long)total_recv_latency/runaheadVec_size) + "\n"; */
  /*   else  */
  /*     decouplingStats << "Avg Recv Latency (cycles): 0.000\n"; */
  /*   decouplingStats << "Total Runahead Distance (cycles): " << send_runahead_sum << "\n"; */
  /*   decouplingStats << "Number of Receive_Instructions: " << runaheadVec_size << "\n"; */
  /*   if(runaheadVec_size) */
  /*     decouplingStats << "Average Runahead Distance(cycles): " << send_runahead_sum/(long long)runaheadVec_size << endl;  */
  /*   else  */
  /*     decouplingStats << "Average Runahead Distance(cycles): 0.000\n"; */
  /*   decouplingStats.close(); */
  /* } */

  /* IS THIS STILL NEEDED? 
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

  //TODO: REINTRODUCE_STATS
  /* //calculate and print summary stats on load latencies */
  /* ifstream memStatsIn(outputDir+"memStats"); */
  /* if (memStatsIn) { */
  /*   memStats << "\n"; */
  /*   memStats << "SUMMARY\n"; */

  /*   //calculate mlp stats */
  /*   string median_mlp_prefix=""; */
  /*   median_mlp_prefix+= "Median # DRAM Accesses Per " + to_string(mlp_epoch) + "-cycle Epoch: "; */
  /*   string max_mlp_prefix=""; */
  /*   max_mlp_prefix+= "Max # DRAM Accesses Per " + to_string(mlp_epoch) + "-cycle Epoch: "; */
  /*   string mean_mlp_prefix=""; */
  /*   mean_mlp_prefix+= "Mean # DRAM Accesses Per " + to_string(mlp_epoch) + "-cycle Epoch: "; */
  /*   sort(accesses_per_epoch.begin(), accesses_per_epoch.end()); */
  
  /*   if(accesses_per_epoch.size()==0) { */
  /*     memStats << mean_mlp_prefix << 0 << endl; */
  /*     memStats << median_mlp_prefix << 0 << endl; */
  /*     memStats << max_mlp_prefix << 0 << endl; */
  /*   } else { */
  /*     memStats << mean_mlp_prefix << ((float) accumulate(accesses_per_epoch.begin(), accesses_per_epoch.end(),0))/accesses_per_epoch.size() << endl; */
  /*     memStats << median_mlp_prefix << accesses_per_epoch[accesses_per_epoch.size()/2] << endl; */
  /*     memStats << max_mlp_prefix << accesses_per_epoch[accesses_per_epoch.size()-1] << endl; */
      
  /*     ofstream mlpStats; */
  /*     mlpStats.open(outputDir + "mlpStats"); */
  /*     for(auto it = accesses_per_epoch.begin(); it != accesses_per_epoch.end(); it++) {  */
  /*       auto access = *it; */
  /*       mlpStats << access << endl; */
  /*     } */
  /*     mlpStats.close(); */
  /*   } */
    
  /*   memStats << "Total Mem Access Latency (cycles): " << totalLatency << endl; */
  /*   memStats << "Avg Mem Access Latency (cycles): " << totalLatency/load_stats_vector_size << endl; */
  /*   memStats.close(); */
  /* } */

  //TODO: REINTRODUCE_STATS
  /* // queue sizes */
  /* if(commQSizes.size() > 0) { */
  /*   ofstream queueStats; */
  /*   queueStats.open(outputDir+"queueStats"); */
  /*   queueStats << "Mean CommQ Size: " << ((float) accumulate(commQSizes.begin(), commQSizes.end(), 0))/commQSizes.size() << endl; */
  /*   queueStats << "Max CommQ Size: " << commQMax << endl; */
  /*   if (SABSizes.size() > 0) { */
  /*     queueStats << "Mean SAB Size: " << ((float) accumulate(SABSizes.begin(), SABSizes.end(), 0))/SABSizes.size() << endl; */
  /*     queueStats << "Max SAB Size: " << SABMax << endl; */
  /*   } */
  /*   if (SVBSizes.size() > 0) { */
  /*     queueStats << "Mean SVB Size: " << ((float) accumulate(SVBSizes.begin(), SVBSizes.end(), 0))/SVBSizes.size() << endl; */
  /*     queueStats << "Max SVB Size: " << SVBMax << endl; */
  /*   } */
  /*   if (termBuffSizes.size() > 0) { */
  /*     queueStats << "Mean TermBuff Size: " << ((float) accumulate(termBuffSizes.begin(), termBuffSizes.end(), 0))/termBuffSizes.size() << endl; */
  /*     queueStats << "Max TermBuff Size: " << termBuffMax << endl; */
  /*   } */
  /*   queueStats.close(); */
  /* } */
}

void Simulator::printStats(){
  uint64_t tdiff_mins = chrono::duration_cast<std::chrono::minutes>(curr_time - init_time).count();
  uint64_t tdiff_seconds = chrono::duration_cast<std::chrono::seconds>(curr_time - init_time).count();
  uint64_t tdiff_milliseconds = chrono::duration_cast<std::chrono::milliseconds>(curr_time - init_time).count();

  cout << "[SIM] ------- End of Simulation!!! ------------------------" << endl << endl;
  // print stats for each mosaic tile
  {
    for (auto it=tiles.begin(); it!=tiles.end(); ++it) 
      if (Core* core=dynamic_cast<Core*>(it->second)) {
    	*global_stat += *core->stats;
      } else if( Accelerator* acc=dynamic_cast<Accelerator*>(it->second)) {
    	global_stat->acc_energy += acc->energy;
      }
    
    int i = 0;
    for (auto it=tiles.begin(); it!=tiles.end(); it++) {
      Tile* tile=it->second;
      if(Core* core=dynamic_cast<Core*>(tile)) {
  	// update cycle stats
  	ofstream core_output;
  	core_output.open(outputDir + string("core_")+ to_string(i++));
  	global_stat->set("cycles", core->cycles);
  	core->stats->set("cycles", core->cycles);
  	// print stats
  	cout << "------------- Final " << core->name << " Stats --------------\n";
  	core->stats->print(cout);
  	core->stats->print_raw(core_output);
  	// calculate energy & print
  	core->calculateEnergyPower();
  	cout << "total_energy : " << core->total_energy << " Joules\n";
  	cout << "avg_power : " << core->avg_power << " Watts\n";
      } else if (Accelerator *acc = dynamic_cast<Accelerator *>(tile)) {
  	ofstream acc_output(outputDir + "Acc_sim");
  	for(auto dt: acc->acc_bytes)
  	  acc_output << "BYTES: " <<dt.first.first << "," << dt.first.second << "=" << dt.second << endl;
  	cout << endl;
  	for(auto dt: acc->acc_cycles)
  	  acc_output  << "CYCLES: "<< dt.first.first << "," << dt.first.second << "=" << dt.second << endl;
  	cout << endl;
  	for(auto dt: acc->acc_energy)
  	  acc_output  << "ENERGY: "<< dt.first.first << "," << dt.first.second << "=" << dt.second << endl;
      }
    }
  }
  
  ofstream sim_output(outputDir + "Global_sim");
  cout << "\n----------------GLOBAL STATS--------------\n";
  calculateGlobalEnergyPower();
  global_stat->print(cout);
  cout << "global_energy : " << global_stat->global_energy << " Joules\n";
  cout << "global_avg_power : " << global_stat->global_avg_power << " Watts\n";
  memInterface->mem->printStats(true);
  if(tdiff_mins>5) 
    cout << "Total Simulation Time: " << tdiff_mins << " mins \n";
  else if(tdiff_seconds>0) 
    cout << "Total Simulation Time: " << tdiff_seconds << " secs \n";
  else
    cout << "Total Simulation Time: " << tdiff_milliseconds << " ms \n";

  global_stat->print_raw(sim_output);
  uint64_t total_flops=global_stat->get("FP_ADDSUB") + global_stat->get("FP_MULT") + global_stat->get("FP_REM");
  double total_gflops=total_flops/(1e9);
  uint64_t instruction_count = global_stat->get("total_instructions");

  cout << "Average Global Simulation Speed: " << 1000*instruction_count/tdiff_milliseconds << " Instructions per sec \n";
  cout << "Cycles: " << cycles << endl;

  cout << "Total GFLOPs : " << total_gflops << endl;
  
  cout << "Global energy : " << global_stat->global_energy << endl;
  cout << "-------All (" << nb_cores << ") cores energy (J) : " << global_stat->cores_energy << endl;
  cout << "-------LLC_energy (J) : " << global_stat->LLC_energy << endl;
  cout << "-------DRAM_energy (J) : " << global_stat->DRAM_energy << endl;
  cout << "-------Acc_energy (J) : " << global_stat->acc_energy << endl;
  
  sim_output.close();
}
  

void Simulator::calculateGlobalEnergyPower() {
  global_stat->cores_energy = 0.0;
  global_stat->global_energy = 0.0;
  int n_cores = 0;
  for (auto it=tiles.begin(); it!=tiles.end(); it++) {
    Tile* tile=it->second;
    if(Core* core=dynamic_cast<Core*>(tile)) {
      // aggregate all of the per-core energy
      n_cores++;
      core->calculateEnergyPower();
      global_stat->cores_energy += core->total_energy;
    }
  }
  global_stat->global_energy += global_stat->cores_energy;      

  // Add the L3 cache energy (we assume it is shared - NOTE this can change in a future)
  global_stat->LLC_energy = (global_stat->get("l3_hits") + global_stat->get("l3_misses") ) * cfg->energy_per_L3_access.at(cfg->technology_node);
  global_stat->global_energy += global_stat->LLC_energy;      

  // Add the DRAM energy
  //Note: dram_accesses is on a cache line granularity
  global_stat->DRAM_energy = global_stat->get("dram_accesses") * cfg->energy_per_DRAM_access.at(cfg->technology_node);
  global_stat->global_energy += global_stat->DRAM_energy;

  // Add accelerators energy 
  global_stat->global_energy += global_stat->acc_energy;      

  // Finally, calculate Avg Power (in Watts)
  global_stat->global_avg_power = (global_stat->global_energy * clockspeed*1e+6) / cycles;   // clockspeed is defined in MHz

  // some debug stuff
}
