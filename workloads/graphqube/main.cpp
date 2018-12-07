/*
    main.cpp
    
    TODO:
        The hasher probably has too many collisions
*/

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <queue>

#include <ctime>

using namespace std;

// --
// Typedefs

namespace std {
    template <> struct hash<std::pair<int, int>> {
        inline size_t operator()(const std::pair<int, int> &v) const {
            std::hash<int> int_hasher;
            return int_hasher(v.first) ^ int_hasher(v.second);
        }
    };
    template <> struct hash<std::unordered_map<int, int>> {
        inline size_t operator()(const std::unordered_map<int, int> &v) const {
            std::hash<int> int_hasher;
            long int out = 0;
            for (int i=0; i < v.size(); ++i) {
                out += (i * 2654435761U) ^ v.at(i);
            }
            return out;
        }
    };
}

typedef unordered_set<pair<int,int>> edgeset;
typedef unordered_map<int, int> intint_map;

typedef vector<pair<int, float>> neighbors_type_entry;
typedef unordered_map<int, neighbors_type_entry> neighbors_type;
typedef unordered_map<int, neighbors_type> neighbors;

// --
// Structs

struct Row {
    int src;
    int trg;
    float weight;
    int src_type;
    int trg_type;
    string edge_type;
};

struct Reference {
    neighbors neibs;
    edgeset visited;
    int num_edges;
};

struct Query {
    edgeset edges;
    intint_map types;
    int num_edges;
};

struct Candidate {
    edgeset edges;
    intint_map nodes;
    float weight;
    int num_uncovered_edges;
};

auto cmp = [](Candidate* left, Candidate* right) { return left->weight > right->weight;};
typedef priority_queue<Candidate*, vector<Candidate*>, decltype(cmp)> candidate_heap;

// --
// Helpers

Query load_query(string filename) {
    
    Query query;
    
    int src, trg, src_type, trg_type;
    float weight;
    string edge_type;

    string line;
    ifstream infile(filename, ifstream::in);
    infile.ignore(9999, '\n');
    while(getline(infile, line)) {

        istringstream iss(line);
        iss >> src;
        iss >> trg;
        iss >> weight;
        iss >> src_type;
        iss >> trg_type;
        iss >> edge_type;
        
        query.edges.insert(pair<int,int>(src, trg));
        query.edges.insert(pair<int,int>(trg, src));
        query.types.emplace(src, src_type);
        query.types.emplace(trg, trg_type);
    }
    query.num_edges = query.edges.size() / 2;
    return query;
}

Reference load_edgelist(string filename) {
    Reference reference;
    reference.num_edges = 0;
    
    int src, trg, src_type, trg_type;
    float weight;
    string edge_type;

    string line;
    ifstream infile(filename, ifstream::in);
    infile.ignore(9999, '\n');
    while(getline(infile, line)) {
        reference.num_edges += 1;
        
        istringstream iss(line);
        iss >> src;
        iss >> trg;
        iss >> weight;
        iss >> src_type;
        iss >> trg_type;
        iss >> edge_type;
        
        // Index edges
        if(reference.neibs.find(src) == reference.neibs.end()) {
            neighbors_type_entry tmp_type_entry;
            neighbors_type tmp_type;
            
            tmp_type_entry.push_back(pair<int,float>(trg, weight));
            tmp_type.emplace(trg_type, tmp_type_entry);
            reference.neibs.emplace(src, tmp_type);
        } else {
            if(reference.neibs[src].find(trg_type) == reference.neibs[src].end()) {
                neighbors_type_entry tmp_type_entry;
                tmp_type_entry.push_back(pair<int,float>(trg, weight));
                reference.neibs[src].emplace(trg_type, tmp_type_entry);
            } else {
                reference.neibs[src][trg_type].push_back(pair<int,float>(trg, weight));
            }
        }
        
        // Index reverse edges
        if(reference.neibs.find(trg) == reference.neibs.end()) {
            neighbors_type_entry tmp_type_entry;
            neighbors_type tmp_type;
            
            tmp_type_entry.push_back(pair<int,float>(src, weight));
            tmp_type.emplace(src_type, tmp_type_entry);
            reference.neibs.emplace(trg, tmp_type);
        } else {
            if(reference.neibs[trg].find(src_type) == reference.neibs[trg].end()) {
                neighbors_type_entry tmp_type_entry;
                tmp_type_entry.push_back(pair<int,float>(src, weight));
                reference.neibs[trg].emplace(src_type, tmp_type_entry);
            } else {
                reference.neibs[trg][src_type].push_back(pair<int,float>(src, weight));
            }
        }
        
    }
    
    return reference;
}

vector<Candidate*> get_initial_candidates(Query& query, Row& row) {    
    
    vector<Candidate*> initial_cands;
    
    for (auto &q_edge : query.edges) {
        if(q_edge.first < q_edge.second) {
            
            int q_src_type = query.types[q_edge.first];
            int q_trg_type = query.types[q_edge.second];

            if((q_src_type == row.src_type) & (q_trg_type == row.trg_type)) {
                Candidate* new_cand = new Candidate;
                
                new_cand->edges.insert(pair<int,int>(q_edge.first, q_edge.second));
                new_cand->edges.insert(pair<int,int>(q_edge.second, q_edge.first));
                                
                new_cand->nodes.insert(pair<int,int>(q_edge.first, row.src));
                new_cand->nodes.insert(pair<int,int>(q_edge.second, row.trg));

                new_cand->num_uncovered_edges = query.num_edges - 1;
                
                new_cand->weight = row.weight;
                
                initial_cands.push_back(new_cand);
            }

            if((q_src_type == row.trg_type) & (q_trg_type == row.src_type)) {
                Candidate* new_cand = new Candidate;
                
                new_cand->edges.insert(pair<int,int>(q_edge.first, q_edge.second));
                new_cand->edges.insert(pair<int,int>(q_edge.second, q_edge.first));
                
                new_cand->nodes.insert(pair<int,int>(q_edge.second, row.src));
                new_cand->nodes.insert(pair<int,int>(q_edge.first, row.trg));
                
                new_cand->num_uncovered_edges = query.num_edges - 1;
                
                new_cand->weight = row.weight;
                
                initial_cands.push_back(new_cand);
            }
       }
    }
    return initial_cands;
}

vector<Candidate*> enter_new_candidate(Candidate* cand, vector<Candidate*> out_cands, int q_src, int q_trg, int r_trg, float new_weight, int new_num_uncovered_edges, vector<Candidate*>* new_cand_ptr, int verbose) {
  Candidate* new_cand = new Candidate;
  
  new_cand->edges = cand->edges;
  new_cand->edges.insert(pair<int,int>(q_src, q_trg));
  new_cand->edges.insert(pair<int,int>(q_trg, q_src));
  
  new_cand->nodes = cand->nodes;
  new_cand->nodes.insert(pair<int,int>(q_trg, r_trg));
  
  new_cand->weight = new_weight;
  new_cand->num_uncovered_edges = cand->num_uncovered_edges - 1;
  
  if(verbose) {
    cout << "new_cand: ";
    for(auto it = new_cand->nodes.cbegin(); it != new_cand->nodes.cend(); ++it) {
      cout << "(" << it->first << ": " << it->second << ") ";
    }
    cout << " " << new_cand->num_uncovered_edges << endl;
  }
  
  if(new_num_uncovered_edges == 0) {
    out_cands.push_back(new_cand);
  } else {
    new_cand_ptr->push_back(new_cand);
  }
  
  return out_cands;
} 

 //find the 1st edge that's not in candidate but whose src node is in candidate's nodes
int* get_query_ref_instance (Query& query, Candidate* cand) {
  int* query_array=new int[4]();
  for(auto &q_edge : query.edges) {
    if(cand->edges.find(q_edge) == cand->edges.end()) {
      if(cand->nodes.find(q_edge.first) != cand->nodes.end()) {
        int r_src = cand->nodes[q_edge.first];
        int q_src = q_edge.first;
        int q_trg = q_edge.second;
        int q_trg_type = query.types[q_trg];
        query_array[0] = r_src;  query_array[1]=q_src; query_array[2]=q_trg; query_array[3]=q_trg_type;
        
        break;
      }
    }
  }
  return query_array;
}

vector<Candidate*> expand_candidate(Query& query, Reference& reference, \
        vector<Candidate*>& initial_cands, const float max_weight, candidate_heap& top_k) {
    
    vector<Candidate*> new_cands;
    vector<Candidate*> out_cands;
    
    int verbose = 0;
    
    vector<Candidate*>* curr_cand_ptr;
    vector<Candidate*>* new_cand_ptr;
    vector<Candidate*>* tmp_cand_ptr;
    
    curr_cand_ptr = &initial_cands;
    new_cand_ptr = &new_cands;
    
    while(true) {
        for(auto &cand : *curr_cand_ptr) {

            if(verbose) {
                cout << "entry: ";
                for(auto it = cand->nodes.cbegin(); it != cand->nodes.cend(); ++it) {
                    cout << "(" << it->first << ": " << it->second << ") ";
                }
                cout << " " << cand->num_uncovered_edges << endl;
            }
                  
            int r_src, q_src, q_trg, q_trg_type;
            
            int* query_array=get_query_ref_instance(query, cand);
            
            r_src=query_array[0]; q_src=query_array[1]; q_trg=query_array[2]; q_trg_type=query_array[3];
            int flag = 0;
                                   
            int q_trg_current, q_trg_new;
            if(reference.neibs[r_src].find(q_trg_type) != reference.neibs[r_src].end()) {
                
                if(cand->nodes.find(q_trg) != cand->nodes.end()) {
                    q_trg_current = cand->nodes[q_trg];
                    q_trg_new = 0;
                } else {
                    q_trg_current = -1;
                    q_trg_new = 1;
                }
                
                unordered_set<int> cand_nodes_values;
                for(auto &kv : cand->nodes) {
                    cand_nodes_values.insert(kv.second);
                }
                
                for(auto &r_trg_weight : reference.neibs[r_src][q_trg_type]) {
                    int r_trg = r_trg_weight.first;
                    float r_edge_weight = r_trg_weight.second;
                    
                    bool node_fits = (q_trg_new && (cand_nodes_values.find(r_trg) == cand_nodes_values.end())) || (q_trg_current == r_trg);
                    if(node_fits) {
                        
                        bool not_visited = reference.visited.find(pair<int,int>(r_src, r_trg)) == reference.visited.end();
                        if(not_visited) {

                            int new_num_uncovered_edges = cand->num_uncovered_edges - 1;
                            float new_weight = cand->weight + r_edge_weight;
                            float upper_bound = new_weight + max_weight * new_num_uncovered_edges;
                            
                            if(upper_bound > top_k.top()->weight) {
                              out_cands=enter_new_candidate(cand, out_cands, q_src, q_trg, r_trg, new_weight, new_num_uncovered_edges, new_cand_ptr, verbose);
                            }
                        }
                    }
                }
            }
        }
        
        if(new_cand_ptr->size() == 0) {
            if(verbose) { cout << "out_cands.size() " << out_cands.size() << endl; }
            return out_cands;
        } else {
            if(verbose) { cout << "new_cands.size() " << new_cand_ptr->size() << endl; }
            curr_cand_ptr->clear();
            tmp_cand_ptr = curr_cand_ptr;
            curr_cand_ptr = new_cand_ptr;
            new_cand_ptr = tmp_cand_ptr;
        }
    }
}

template<typename T> void print_queue(T& q) {
    for(int i = 0; i < 50; ++i) {cout << "--";}; cout << endl;
    float total_weight = 0.0;
    while(!q.empty()) {
        total_weight += q.top()->weight;
        
        Candidate* top = q.top();
        for ( auto it = top->nodes.begin(); it != top->nodes.end(); ++it ) {
            cout << it->first << ":" << it->second << "\t";
        }
        cout << "weight:" << top->weight << endl;
        q.pop();
    }
    for(int i = 0; i < 50; ++i) {cout << "--";};
    cout << endl << "total_weight: " << total_weight << endl;
}


void load_rows(vector<Row>* row_vector, string edge_path, int num_edges) {
  ifstream infile(edge_path, ifstream::in);
  string line;
  for (int i=0; i < num_edges; i++) {
    Row row={};
    getline(infile, line);
    istringstream iss(line);
    iss >> row.src;
    iss >> row.trg;
    iss >> row.weight;
    iss >> row.src_type;
    iss >> row.trg_type;
    iss >> row.edge_type;
    row_vector->push_back(row);
  }
  cout << "size of vector is " << row_vector->size() << endl;  
}

void _kernel_(vector<Row>* row_vector, Query& query, Reference& reference, candidate_heap& top_k, int& K, int& counter) {
      vector<Row>::iterator row_it=row_vector->begin();
      while(true) {        
        Row row=*row_it;
             
        if(row.weight * query.num_edges < top_k.top()->weight) {
            break;
        }
        
        vector<Candidate*> initial_candidates, out_cands; 
        initial_candidates = get_initial_candidates(query, row);
        out_cands = expand_candidate(query, reference, initial_candidates, row.weight, top_k);
        reference.visited.insert(pair<int,int>(row.src, row.trg));
        reference.visited.insert(pair<int,int>(row.trg, row.src));
                       
        for(auto &out_cand : out_cands) {
            if(top_k.size() < K) {
                top_k.push(out_cand);
            } else if (out_cand->weight > top_k.top()->weight) {
                top_k.pop();
                top_k.push(out_cand);
            }
        }
        
        counter++;
        if(counter == reference.num_edges) {
            break;
        }
        ++row_it;
    }
}


int main(int argc, char **argv) {

    if(argc != 4) {
        cerr << "usage: main <query-path> <edge-path> <k>" << endl;
        cerr << "  query-path   path to query" << endl;
        cerr << "  edge-path    path to edgelist" << endl;
        cerr << "  k            number of hits" << endl;
        return 1;
    }
    
    string query_path(argv[1]);
    string edge_path(argv[2]);
    int K = stoi(argv[3]);

    // -- 
    // IO
    
    // Load query  
    Query query = load_query(query_path);
    cerr << "num_query_edges=" << query.edges.size() << endl;

    // Load edges
    Reference reference = load_edgelist(edge_path);
    cerr << "num_reference_edges=" << reference.num_edges << endl;
        
    candidate_heap top_k(cmp);
    for(int i=0; i < K; ++i) {
        Candidate* dummy_cand = new Candidate;
        dummy_cand->weight = -1.0;
        top_k.push(dummy_cand);
    }
    unordered_set<unordered_map<int,int>> top_k_members;
    
    // --
    // Run
    
    int counter = 0;
    clock_t start = clock();

    
    vector<Row>* row_vector = new vector<Row>();
    load_rows(row_vector, edge_path, reference.num_edges);
    
    
    _kernel_(row_vector, query, reference, top_k, K, counter);
    
    print_queue(top_k);
    double duration = (clock() - start) / (double)(CLOCKS_PER_SEC);
    cout << "counter: " << counter << endl;
    cout << "runtime: " << duration << endl;
}
