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

  int fileSizeKB(std::string name) {
    int filesize = 0;
    ifstream cfile(name);
    if (cfile.is_open()) {
      cfile.seekg(0, ios::end);
      filesize = cfile.tellg(); 
      cfile.close();
    }
    return filesize/1024;
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
        //sime
        string stripped_line = line;
        boost::trim_left(stripped_line);
        //some instructions (e.g., exceptions) span multiple lines. "cleanup" is just a label as well, so we can skip to next line
        if(stripped_line.find("to label")==0 || stripped_line.find("catch")==0 || stripped_line.find("cleanup")==0) {
          //cout << "BAD LINE: " << line << endl;
          continue;
        }
         //can't parse switch statement cuz it's multiple lines
        //switch statements span multiple lines and are of the form e.g.,
        /* 
           7,13,2,  switch i31 %trunc, label %sw.default [
           i31 1, label %sw.bb
           i31 2, label %sw.bb6
           i31 3, label %sw.bb16
           ]          
        */
        if(line.find("switch")!=string::npos) { 
          string line2=line;
          while(line2.find("]")==string::npos) { 
            getline(cfile,line2);
          }
        }
              
        vector<string> s = split(line, ',');
        
        int id = stoi(s.at(0));
        TInstr type = static_cast<TInstr>(stoi(s.at(1)));
        int bbid = stoi(s.at(2));
        string name = "";
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
      while (i<numEdges) {
        //cout << "edge iter: " <<i << endl;

        getline(cfile,line);
        //cout << line << endl;
        string stripped_line = line;
        boost::trim_left(stripped_line);
        vector<string> s = split(line, ',');
        int edgeT;

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
      cout << "[ERROR] Cannot open Graph file!\n";
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
    int filesizeKB = fileSizeKB(name);
    ifstream memfile(name);
    if (memfile.is_open()) {
      cout << "[SIM] Start reading the Memory trace (" << name << ") | size = " << filesizeKB << " KBytes\n";
      if (filesizeKB > 100000)  // > 100 MB
        cout << "[SIM] ...big file (+100MB), expect some minutes to read it.\n";
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
      cout << "[ERROR] Cannot open Memory profiling file!\n";
      assert(false);
    }
    cout << "[SIM] ...Finished reading the Memory trace!\n\n";
    memfile.close();
  }

  // Read Dynamic Control Flow data from the profiling file. 
  // Format:   <string_bb_name>,<current_bb_id>,<next_bb_id>
  // vector <cf> will be the sequential list of executed BBs
  void readProfCF(std::string name, std::vector<int> &cf) {
    string line;
    string last_line;
    int filesizeKB = fileSizeKB(name);
    ifstream cfile(name);
    bool init = false;
    int last_bbid = -1;
    if (cfile.is_open()) {
      cout <<"[SIM] Start reading the Control-Flow trace (" << name << ") | size = " << filesizeKB << " KBytes\n";
      if (filesizeKB > 100000)  // > 100 MB
        cout << "[SIM] ...big file (+100MB), expect some minutes to read it.\n";
      while (getline (cfile,line)) { //could be cuz kernel gets called multiple times in a loop for example
        vector<string> s = split(line, ',');
        assert(s.size() == 3);
        if (stoi(s.at(1)) != last_bbid && last_bbid != -1) {
          cout << last_bbid << " / " << s.at(1) << "\n";
          cout << last_line << " / " << line << "\n";
          assert(false); 
        }
        if (!init) {
          cf.push_back(stoi(s.at(1)));
          init = true;
        }
        last_bbid = stoi(s.at(2));
        cf.push_back(last_bbid);
        last_line = line;
      }
    }
    else {
      cout << "[ERROR] Cannot open CF profiling file!\n";
      assert(false);
    }
    cout <<"[SIM] ...Finished reading the Control-Flow trace! - Total contexts: " << cf.size() << "\n\n";
    cfile.close();
  }
};
