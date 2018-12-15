#include <iostream>
#include <fstream>
#include <sstream> 
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

  void readGraph(std::string name, Graph &g) {
    cout<< "Entered readgraph \n" ;
    ifstream cfile(name);
    cout << "Opened file: " << name << endl ;
    if (cfile.is_open()) {
      string temp;
      getline(cfile,temp);
      int numBB = stoi(temp);  
      getline(cfile,temp);
      int numNode = stoi(temp);

      getline(cfile,temp);
      
      int numEdge = stoi(temp);
      string line;
      for (int i=0; i<numBB; i++)
        g.addBasicBlock(i);

      
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
        
        vector<string> s = split(line, ',');
        int id = stoi(s.at(0));
        TInstr type = static_cast<TInstr>(stoi(s.at(1)));
        int bbid = stoi(s.at(2));
        string name = "";
        for(unsigned int i=3; i<s.size(); i++)
          name += s.at(i);     
        name = name.substr(2, name.size()-2);
        g.addNode( id, type, bbid, name, cfg.instr_latency[type]);
        if(id+1==numNode) {
          //cout << "ID "<< (id+1) << endl;
          //cout << "Num Nodes " << numNode << endl;
          break;
        }
      }
      //cout << "Done with Nodes " << endl;
      int i=0;
      while (i<numEdge) {
        //cout << "edge iter: " <<i << endl;

        getline(cfile,line);
        //cout << line << endl;
        string stripped_line = line;
        boost::trim_left(stripped_line);
        //some instructions (e.g., exceptions) span multiple lines. "cleanup" is just a label as well, so we can skip to next line
        /*if(stripped_line.find("to label")==0 || stripped_line.find("catch")==0 || stripped_line.find("cleanup")==0) {
          cout << "BAD LINE: " << line << endl;
          continue;
          }*/
        vector<string> s = split(line, ',');

        // if(i==6789 || i==6788) {
        //   cout << i << " At the failing edge \n";
        //   cout << line << endl;
        // }
        int edgeT;

        edgeT = stoi(s.at(2));
        /*
        try {
          edgeT = stoi(s.at(2));
        }
        catch (...) {
          cout << "Reached end because of: " << line << endl;
          break;
          }*/
        
        
        if(edgeT >= 0) {
          TEdge type = static_cast<TEdge>(stoi(s.at(2)));
          g.addDependent(g.getNode(stoi(s.at(0))), g.getNode(stoi(s.at(1))), type);
        }
        else if (edgeT == -1) {
          g.getNode(stoi(s.at(1)))->store_addr_dependents.insert(g.getNode(stoi(s.at(0))));
        }
        i++;
      }
      cout << "Done reading edgelist \n";
      if (getline(cfile,line))
        assert(false);
    }
    else {
      cout << "Error opening Graph file\n";
      assert(false);
    }
    cfile.close();
    cout << "[2] Finished Reading Graph (" << name << ") \n";

    //cout << g << endl; luwa commented out, graph can be large
    
  }
  // Read Dynamic Memory accesses from profiling file.
  // <memory> will be a map of { <instr_id>, <queue of addresses> }
  void readProfMemory(std::string name, std::unordered_map<int, std::queue<uint64_t> > &memory) {
    string line;
    string last_line;
    ifstream cfile(name);
    if (cfile.is_open()) {
      while ( getline(cfile,line) ) {
        vector<string> s = split(line, ',');
        assert(s.size() == 4);
        int id = stoi(s.at(1));
        if (memory.find(id) == memory.end()) 
          memory.insert(make_pair(id, queue<uint64_t>()));
        memory.at(id).push(stoull(s.at(2)));  // insert the <address> into the memory instructions's <queue>
      }
    }
    else {
      cout << "Error opening Memory profiling file\n";
      assert(false);
    }
    cout << "[3] Finished Reading Memory Profile (" << name << ")\n";
    cfile.close();
  }

  // Read Dynamic Control Flow data from profiling file. 
  // Format:   <string_bb_name>,<current_bb_id>,<next_bb_id>
  // vector <cf> will be the sequential list of executed BBs
  void readProfCF(std::string name, std::vector<int> &cf) {
    string line;
    string last_line;
    ifstream cfile(name);
    bool init = false;
    int last_bbid = -1;
    if (cfile.is_open()) {
      while (getline (cfile,line)) {
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
      cout << "Error opening CF profiling file\n";
      assert(false);
    }
    cout <<"[4] Finished Reading CF - Total number of contexts : " << cf.size() << "\n";
    cfile.close();
  }
};
