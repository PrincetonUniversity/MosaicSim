#pragma once

#include <iostream>
#include <fstream>
#include <sstream> 

//#define NDEBUG
#include "assert.h"
#include <regex>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "config.hpp"

using namespace std;

const string WHITESPACES = " \n\r\t\f\v";

/** \brief Triming a string. */ 
static string trim(string s)
{
  size_t index = s.find_first_not_of(WHITESPACES);
  // all white spaces
  s = (index != string::npos) ?  s.substr(index) : "";
  index  = s.find_last_not_of(WHITESPACES);
  s = (index != string::npos) ?  s.substr(0, index+1) : "";

  return s;
}

class Reader {
public:
  /** \bref helper function: split #s with #delimiter into a vector of String   */
  vector<string> split(const string &s, char delim) {
     stringstream ss(s);
     string item;
     vector<string> tokens;
     while (getline(ss, item, delim)) {
        tokens.push_back(item);
     }
     return tokens;
  }

  /** \bref returns the file size in KB */
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

  /** \bref Checks if #s is compose only of digits  */
  bool isNumber(string s)
  {
    for (unsigned int i = 0; i < s.length(); i++)
      if (isdigit(s[i]) == false)
        return false;
    return true;
  }
  
  /** \bref   */
  void readGraph(std::string name, Graph &g, Config *cfg) {
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

        g.addNode( id, type, bbid, name, cfg->instr_latency[type], vec_width);

        if(id+1==numNodes) break;
      }

      // Now start adding ALL the edges
      int i=0;
      while (i<numEdges && getline(cfile,line)) {
        //cout << "edge iter: " <<i << endl;

        //getline(cfile,line); luwa restore, just testing
        //cout << line << endl;
        string stripped_line = line;
        stripped_line = trim(stripped_line);
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

  /** \bref Read Dynamic Memory accesses from profiling file. <memory>
      will be a map of { <instr_id>, <queue of addresses> } */
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

  /**  Read Dynamic Memory accesses from profiling file in chunks
       where <memory> will be a map of { <instr_id>, <queue of
       addresses> } */
  bool readProfMemoryChunk(Core* core) {
//     int file = core->memfile;
//     std::unordered_map<int, std::queue<uint64_t> > &memory=core->memory;
//     long long chunk_size=core->sim->mem_chunk_size;
    
//     string line;
//     int numLines = 0;
//     bool progressed=false;
//     int i;

//     if (core->read_mem)
//       return progressed;

//     if (file > 0) {
//       for(int i = 0; i < chunk_size; i++) {
// 	int input[5];
// 	size_t mess_size = 5 * sizeof(int);
// 	int bytes = 0;

// 	bytes = read(file, input, mess_size);
// 	if (bytes < 0 || !bytes) {
// 	  // char mess[256];
// 	  // sprintf(mess, "Reading memory with core %d", core->id);
// 	  // perror(mess);
// 	  return progressed;
// 	}
// 	if (bytes != mess_size) {
// 	  cout << "READ  " << bytes << " from memory pipe instead of " << mess_size << endl;
// 	  assert(false);
// 	}
// 	if (*input == -1) {
// 	  cout << "[SIM] ...Finished reading the Memory trace! and read " << core->nb_mem << " entrie." << endl;
// 	  core->read_mem = true;
// 	  int ret = close(file);
// 	  if (ret < 0) {
// 	    char mess[256];
// 	    sprintf(mess, "Closing memory file with core %d ", core->id);
// 	    perror(mess);
// 	  }
// 	  core->memfile = 0;
// 	  progressed=true;
// 	  break;
// 	}

// 	progressed=true;
// 	core->nb_mem++;
// 	int id =  input[1];
// 	uint64_t adress = (uint64_t) input[2];
//         if(memory.find(id) == memory.end()) 
//           memory.insert(make_pair(id, queue<uint64_t>()));
//         memory.at(id).push(adress);  // insert the <address> into the memory instructions's <queue>
//       }
//       return progressed;
//     } else {
//       cout << "[ERROR] Cannot open the Memory trace file!\n";
      return false;
//     }
  }

  /** \bref  read accelerator file. 
   
   eg 1,decadesTF_matmul,3,3,3,3
   node id, acc_kernel_name, sizes ...
  */
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
        acc_map.at(id).push(line);  //insert the acc name and all args needed to run it for that instance
      }
    }
    else {
      cout << "[Sim] Empty Accelerator trace file!\n";
      return;
    }
    cout << "[SIM] ...Finished reading the Accelerator trace!\n\n";
    accfile.close();
  }
  
  /** \bref Read Dynamic Control Flow data from the profiling file.  

   Format:   <string_bb_name>,<current_bb_id>,<next_bb_id>
   vector <cf> will be the sequential list of executed BBs
  */

 void readProfCF(string name, vector<int> &cf, map<int, pair<bool,set<int>>> &bb_cond_destinations) {
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
        int new_bbid = stoi(s.at(1)); 
        if(new_bbid != last_bbid && last_bbid != -1) {
          cout << "[WARNING] non-continuous control flow path \n";
          cout << last_bbid << " / " << s.at(1) << "\n";
          cout << last_line << " / " << line << "\n";
        }
        cf.push_back(new_bbid);
        set<int> destinations;
        bb_cond_destinations.insert( make_pair(new_bbid, make_pair(false, destinations)) );
        if(s.at(0)=="B") {
          bb_cond_destinations.at(new_bbid).first=true;   // mark it is a conditional branch
        }
        last_bbid = stoi(s.at(2));
      }
      // push the last BB from the last line of the file 
      cf.push_back(last_bbid);
      set<int> dest;
      bb_cond_destinations.insert( make_pair(last_bbid, make_pair(false, dest)) ); // last branch (RET) is unconditional 
    }
    else {
      cout << "[ERROR] Cannot open the CF Control-Flow trace file!\n";
      assert(false);
    }
    cout <<"[SIM] ...Finished reading the Control-Flow trace! - Total contexts: " << cf.size() << "\n\n";
    cfile.close();

    // for each conditional branch, annotate its destinations: {dest1,dest2,...}
    for(uint64_t i=0; i<cf.size(); i++) {
      if(bb_cond_destinations.at(cf[i]).first==true) {  // check if it is a cond branch
        bb_cond_destinations.at(cf[i]).second.insert(cf[i+1]); // insert its "next_bbid" as a destination
      }
    }
 }

  void ReadPartialBarrier(std::string name, std::deque<pair<int, int>> &partialBarrier) {
    string line;
    ifstream PBfile(name);
    
    if (PBfile.is_open()) {
      cout << "[SIM] Start reading the partial barrier sizes (" << name << ")" << endl;
      while ( getline(PBfile,line) )  {
        vector<string> s = split(line, '\t');
	partialBarrier.push_back(make_pair(stoi(s[0]),stoi(s[1])));
      }
    } else {
      cout << "[Sim] Empty partial barrier file!\n";
      return;
    }
    cout << "[SIM] ...Finished reading the partial barrier sizes!\n\n";
    PBfile.close();
  }
};
