#include <iostream>
#include <fstream>
#include <sstream> 

//#define NDEBUG
#include "assert.h"
#include <regex>
#include <boost/algorithm/string.hpp>

using namespace std;

class Reader {
public:
  // helper function: split a string [with delimiter] into a vector
  vector<string> split(const string &s, char delim) {
     stringstream ss(s);
     string item;
     vector<string> tokens;
     while (getline(ss, item, delim)) {
        tokens.push_back(item);
     }
     return tokens;
  }
  uint64_t fileSizeKB(std::string name) {
    uint64_t filesize = 0;
    ifstream cfile(name);
    if (cfile.is_open()) {
      cfile.seekg(0, ios::end);
      filesize = cfile.tellg(); 
      cfile.close();
    }
    return filesize/1024;
  }
  bool isNumber(string s)
  {
    for (unsigned int i = 0; i < s.length(); i++)
      if (isdigit(s[i]) == false)
        return false;
    return true;
  }
  void readGraph(std::string name, Graph &g) {
    ifstream cfile(name);
    if (cfile.is_open()) {
      cout << "[SIM] Start reading the Data Dependency Graph (" << name << ")...\n";
      string line;
      string temp;
      getline(cfile,temp);
      int numBBs = stoi(temp);  
      getline(cfile,temp);
      int numNodes = stoi(temp);
      getline(cfile,temp);  
      int numEdges = stoi(temp);

      // add the list of Basic Blocks to the Graph
      for (int i=0; i<numBBs; i++)
        g.addBasicBlock(i);

      // Now start adding ALL the nodes


      while (true) {
        
        getline(cfile,line);
        vector<string> s = split(line, ',');
        //some instructions span multiple lines, we want to make sure it's the start of a new instruction
        if(!isNumber(s[0])) {
          continue;
        }
        
        //sime
        // string stripped_line = line;
        // boost::trim_left(stripped_line);
        // //some instructions (e.g., exceptions) span multiple lines. "cleanup" is just a label as well, so we can skip to next line
        // if(stripped_line.find("to label")==0 || stripped_line.find("catch")==0 || stripped_line.find("cleanup")==0) {
        //   //cout << "BAD LINE: " << line << endl;
        //   continue;
        // }
        // //can't parse switch statement cuz it's multiple lines
        // //switch statements span multiple lines and are of the form e.g.,
        // /* 
        //    7,13,2,  switch i31 %trunc, label %sw.default [
        //    i31 1, label %sw.bb
        //    i31 2, label %sw.bb6
        //    i31 3, label %sw.bb16
        //    ]          
        // */
        // if(line.find("switch")!=string::npos) { 
        //   string line2=line;
        //   while(line2.find("]")==string::npos) { 
        //     getline(cfile,line2);
        //   }
        // }
        
        int id = stoi(s.at(0));
        TInstr type = static_cast<TInstr>(stoi(s.at(1)));   
        int bbid = stoi(s.at(2));
        string name = "";
        assert((int) s.size() >3 );
        for(unsigned int i=3; i<s.size(); i++)
          name += s.at(i);     
        name = name.substr(2, name.size()-2);
        int vec_width=1;
        if(name.find("dec_bs_vector_inc") != std::string::npos) {
          vec_width = stoi(s.at(s.size()-1));      
        }

        g.addNode( id, type, bbid, name, cfg.instr_latency[type], vec_width);

        if(id+1==numNodes) {
          //cout << "ID "<< (id+1) << endl;
          //cout << "Num Nodes " << numNodes << endl;
          break;
        }
      }
      //cout << "Done with Nodes " << endl;

      // Now start adding ALL the edges
      int i=0;
      while (i<numEdges && getline(cfile,line)) {
        //cout << "edge iter: " <<i << endl;

        //getline(cfile,line); luwa restore, just testing
        //cout << line << endl;
        string stripped_line = line;
        boost::trim_left(stripped_line);
        vector<string> s = split(line, ',');
        int edgeT;
        //        cout << "not offending line " << line << endl;
         
        if((int) s.size() < 3) { //should have index for 2
          
          cout << "offending line " << line << endl;
          assert(false);
        }
        edgeT = stoi(s.at(2));
        
        if(edgeT >= 0) {
          TEdge type = static_cast<TEdge>(stoi(s.at(2)));
          g.addDependent(g.getNode(stoi(s.at(0))), g.getNode(stoi(s.at(1))), type);
        }
        else if (edgeT == -1) {
          g.getNode(stoi(s.at(1)))->store_addr_dependents.insert(g.getNode(stoi(s.at(0))));
        }
        i++;
      }
//      cout << "Done reading edgelist \n";
      if (getline(cfile,line))
        assert(false);
    }
    else {
      cout << "[ERROR] Cannot open the Data Dependency Graph file!\n";
      assert(false);
    }
    cfile.close();
    cout << "[SIM] ...Finished reading the Data Dependency Graph!\n\n";
    
  }

  // Read Dynamic Memory accesses from profiling file.
  // <memory> will be a map of { <instr_id>, <queue of addresses> }
  void readProfMemory(std::string name, std::unordered_map<int, std::queue<uint64_t> > &memory) {
    string line;
    string last_line;
    uint64_t filesizeKB = fileSizeKB(name);
    ifstream memfile(name);
    if (memfile.is_open()) {
      cout << "[SIM] Start reading the Memory trace (" << name << ") | size = " << filesizeKB << " KBytes\n";
      if (filesizeKB > 100000)  // > 100 MB
        cout << "[SIM] ...big file (>100MB), please, expect some minutes to read it.\n";
      while ( getline(memfile,line) ) {
        vector<string> s = split(line, ',');
        assert(s.size() == 4);
        int id = stoi(s.at(1));
        if (memory.find(id) == memory.end()) 
          memory.insert(make_pair(id, queue<uint64_t>()));
        memory.at(id).push(stoull(s.at(2)));  // insert the <address> into the memory instructions's <queue>
      }
    }
    else {
      cout << "[ERROR] Cannot open the Memory trace file!\n";
      assert(false);
    }
    cout << "[SIM] ...Finished reading the Memory trace!\n\n";
    memfile.close();
  }

  // Read Dynamic Memory accesses from profiling file in chunks where <memory> will be a map of { <instr_id>, <queue of addresses> }
  bool readProfMemoryChunk(Core* core) {
    std::ifstream& memfile=core->memfile;
    std::unordered_map<int, std::queue<uint64_t> > &memory=core->memory;
    long long chunk_size=core->sim->mem_chunk_size;
    
    string line;
    int numLines = 0;
    bool progressed=false;
    if (memfile.is_open()) {
      while ((chunk_size < 0 || numLines <= chunk_size) && getline(memfile,line) ) {
        progressed=true;
        vector<string> s = split(line, ',');
        int id = stoi(s.at(1));
        uint64_t address = stoull(s.at(2));
        if(memory.find(id) == memory.end()) 
          memory.insert(make_pair(id, queue<uint64_t>()));
        memory.at(id).push(address);  // insert the <address> into the memory instructions's <queue>
        if (s.size() >= 6) {
          int graphNodeId = stoi(s.at(4));
          int graphNodeDeg = stoi(s.at(5));
          if(core->sim->graphNodeIdMap.find(address) == core->sim->graphNodeIdMap.end()) {
            core->sim->graphNodeIdMap[address] = graphNodeId;

            //assert(core->sim->graphNodeDegMap.find(graphNodeId) == core->sim->graphNodeDegMap.end());
            core->sim->graphNodeDegMap[graphNodeId] = graphNodeDeg;
          } else { // address already in map
            if (core->sim->graphNodeIdMap[address] != graphNodeId) {
              cout << address << " " << core->sim->graphNodeIdMap[address] << " " << graphNodeId << endl;
              assert(false);
            }
            if (core->sim->graphNodeDegMap[graphNodeId] != graphNodeDeg) {
              assert(false);
            }
          }
        }
        numLines++;
      }
      if ( memfile.eof() ) {
        cout << "[SIM] ...Finished reading the Memory trace!\n\n";
        memfile.close();
      }

      return progressed;
    }
    else {
      cout << "[ERROR] Cannot open the Memory trace file!\n";
      return false;
    }
  }

  //read accelerator file
  //eg 1,decadesTF_matmul,3,3,3,3
  //node id, acc_kernel_name, sizes ...
  void readAccTrace(std::string name, std::unordered_map<int, std::queue<string> > &acc_map) {
    string line;
    string last_line;
    uint64_t filesizeKB = fileSizeKB(name);
    ifstream accfile(name);
    if (accfile.is_open()) {
      cout << "[SIM] Start reading the Accelerator Invokation trace (" << name << ") | size = " << filesizeKB << " KBytes\n";
      if (filesizeKB > 100000)  // > 100 MB
        cout << "[SIM] ...big file (>100MB), please, expect some minutes to read it.\n";
      while ( getline(accfile,line) ) {
        
        vector<string> s = split(line, ',');
        
        int id = stoi(s.at(0));
        if (acc_map.find(id) == acc_map.end()) 
          acc_map.insert(make_pair(id, queue<string>()));
        //cout << id << " TEST PRINTING IN ACC TRACE \n";
        acc_map.at(id).push(line);  //insert the acc name and all args needed to run it for that instance
      }
    }
    else {
      cout << "[ERROR] Cannot open the Accelerator trace file!\n";
      assert(false);
    }
    cout << "[SIM] ...Finished reading the Accelerator trace!\n\n";
    accfile.close();
  }
  
  // Read Dynamic Control Flow data from the profiling file. 
  // Format:   <string_bb_name>,<current_bb_id>,<next_bb_id>
  // vector <cf> will be the sequential list of executed BBs
  void readProfCF(std::string name, std::vector<int> &cf, std::vector<bool> &cf_cond) {
    string line;
    string last_line;
    uint64_t filesizeKB = fileSizeKB(name);
    ifstream cfile(name);
    int last_bbid = -1;
    if (cfile.is_open()) {
      cout <<"[SIM] Start reading the Control-Flow trace (" << name << ") | size = " << filesizeKB << " KBytes\n";
      if (filesizeKB > 100000)  // > 100 MB
        cout << "[SIM] ...big file (>100MB), please, expect some minutes to read it.\n";
      while (getline (cfile,line)) { //could be cuz kernel gets called multiple times in a loop for example
        vector<string> s = split(line, ',');
        assert(s.size() == 3);
        if (stoi(s.at(1)) != last_bbid && last_bbid != -1) {
          cout << "[WARNING] non-continuous control flow path \n";
          cout << last_bbid << " / " << s.at(1) << "\n";
          cout << last_line << " / " << line << "\n";
        }
        cf.push_back(stoi(s.at(1)));
        cf_cond.push_back(s.at(0)=="B");   // in the trace "B" is for conditional branches while the "U" is for unconditional
        last_bbid = stoi(s.at(2));
      }
      // push the last BB from the last line of the file 
      cf.push_back(last_bbid);
      cf_cond.push_back(false); // the very last BB of the program (RET) is unconditional
    }
    else {
      cout << "[ERROR] Cannot open the CF Control-Flow trace file!\n";
      assert(false);
    }
    cout <<"[SIM] ...Finished reading the Control-Flow trace! - Total contexts: " << cf.size() << "\n\n";
    cfile.close();
    assert(cf.size()==cf_cond.size());


    // JLA: make a sanity check over the CF trace
    if (false) {
      cout << "\n**----Sanity check of the CF**----------\n";    
      // 1st: build a map of BB ids which are conditional branches and annotate their destinations: {dest1,dest2,...}
      map<int, set<int> > cond_bb_destinations;   // stores for each conditional bbid a set of destinations
      set<int> destinations;
      int total_cond_branches = 0;
      for (uint64_t i=0;i<cf.size();i++) {
        if (cf_cond[i]) {  // verify is a cond branch
          total_cond_branches++;
          cond_bb_destinations.insert( make_pair(cf[i], destinations ));  // insert a new bbid in the map
          cond_bb_destinations.at(cf[i]).insert(cf[i+1]); // insert its "next_bbid" as a destination
        }
      }
      // 2nd: iterate over the "map" of conditional branches
      int cond_bb_whose_both_destinations_are_non_consecutive = 0;
      int dynamic_occurences_alwaysT_bb = 0;
      for (auto it=cond_bb_destinations.begin(); it!=cond_bb_destinations.end(); ++it) {
        int bbid = it->first;
        set<int> destinations = it->second;
        assert( destinations.size()<=2 );  // make sure a conditional BB has no more than 2 destinations
        cout << "bbid: " << bbid << ";  # of dest: " << destinations.size() << "; ";
     
        // check if at least one of the destinations is consecutive wrt the base bbid
        bool consecutive_bb_found = false;
        for (auto it2=destinations.begin(); it2!=destinations.end(); ++it2) {
          int dest_bbid = *it2;
          cout << " dest: " << dest_bbid;
          if (dest_bbid == bbid+1)
            consecutive_bb_found = true;
        }
        cout << endl;
        if (!consecutive_bb_found) {
          cond_bb_whose_both_destinations_are_non_consecutive++;
         
          // count dynamic occurrences of this bbid in the CF
          // JLA: these branches are currently assumed as ALWAYS-TAKEN
          for (uint64_t i=0; i<cf.size(); i++) {
            if (cf[i]==bbid) 
              dynamic_occurences_alwaysT_bb++;
          }
        }
      } 
      cout << "cond_BBs_whose_both_destinations_are_non_consecutive: " << cond_bb_whose_both_destinations_are_non_consecutive << endl;
      cout << "dynamic_occurences_alwaysT_BBs: " << dynamic_occurences_alwaysT_bb << endl;
      cout << "total_cond_branches: " << total_cond_branches << endl;
      cout << "**----END Sanity check of the CF**----------\n\n";
    }
  }
};
