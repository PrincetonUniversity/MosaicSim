#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.hpp"

#if EXPERIMENTAL_FILESYSTEM == 1
#include <experimental/filesystem>
#else
#include <filesystem>
#endif 

#include "parms.hpp"
#include "memsys/DRAM.hpp"
#include "tile/Accelerator.hpp"

using namespace std;

class DRAMSimInterface;

#if EXPERIMENTAL_FILESYSTEM == 1
using namespace std::experimental::filesystem;
#else
using namespace std::filesystem;
#endif

int
exe(string name, string args, bool async) {
  /* string command = name + args; */
  string command = async ? name + args + "&" : name + args;
  int ret;

  cout << "Exeuting: " << command << endl;
  ret = system(command.c_str());
  
  /* need to do this because DEC return 256, which in char value is 0, aka success */
  return  ret = ret > 255 ? 255 : ret;
}

void
mutual_parms::usage() {
  cout << "  --nb_cores | -n #\tNumber of cores. Accepts positive values. In decoupled and DeSC mode, the number of producer/consumer pairs. Default 1" << endl; 
  cout << "  --nb_files | -f\tNumber of files/pipes to use for the communication with the native run." << endl;
  cout << "  --decoupled | -d\tRun in decoupled Supply/Compute mode" << endl;
  cout << "  --DeSC | -S d\tRun in decoupled DeSC Supply/Compute mode mode" << endl;
  cout << "  --debug | -dbg\tRun in debug mode; collect load and decoupling stats" << endl;
  cout << "  --help | -h\t\tPrint this message" << endl;
}

bool
mutual_parms::parseArgs(vector<string>& args) {
  for( auto s = args.begin(); s < args.end(); ) {
    if (*s == "--nb_cores" || *s == "-n") { 
      args.erase(s);
      nb_cores = stoi(*s);
      args.erase(s);
    } if (*s == "--nb_files" || *s == "-f") { 
      args.erase(s);
      nb_files = stoi(*s);
      args.erase(s);
    } else if (*s == "--decoupled" || *s == "-d") {
      args.erase(s);
      decoupled = true;
    } else if (*s == "--DeSC" || *s == "-S") {
      args.erase(s);
      DeSC = true;
    } else if (*s == "--debug" || *s == "-db") {
      args.erase(s);
      debug = true;
    } else {
      s++;
    }
  }
  return valid();
}  

bool
mutual_parms::valid()
{
  bool valid = true;
  
  if ( decoupled && DeSC ) {
    cerr << "Decoupled and DeSC mode activated in samae time  " << endl;
    valid = false;
  }
  if ( nb_cores <= 0 ) {
    cerr << "Invalid number of cores (" << nb_cores << ")" << endl;
    valid = false;
  }
  if (nb_files < 1 || nb_cores < 1 )
    valid = false;
  
  return valid;
}

ostream& operator<<(ostream& os, const mutual_parms& parms) {
  os << "Number of cores :\t" << parms.nb_cores << endl;
  os << "Number of files :\t" << parms.nb_files << endl;
  os << "Debug mode :\t\t" << parms.debug << endl;
  os << "Decoupled mode :\t" << parms.decoupled << endl;
  os << "DeSC mode :\t" << parms.DeSC << endl;
  return os;
}  

sim_parms::sim_parms(){
  path config_dir(MOSAIC_CONFIG_DIR);
  vector<string> ret;

  for(const auto& entry : directory_iterator(config_dir)) {
    string file_name  = entry.path().filename();
    if (!file_name.compare(0, 4, "sim_")) {
      size_t pos = file_name.find(".txt");
      simConfigs.push_back(file_name.substr(4, file_name.size() - 8));
    }
    if (!file_name.compare(0, 5, "core_")) {
      size_t pos = file_name.find(".txt");
      coreConfigs.push_back(file_name.substr(5,file_name.size() - 9));
    }
    if (!file_name.compare(0, 3, "DDR")) {
      size_t pos = file_name.find(".txt");
      DDRConfigs.push_back(file_name.substr(3,file_name.size() - 7));
    }
  }
}

void
sim_parms::usage(bool print_first_line, bool print_mutual) {
  int size = 0;
  if (print_first_line) {
    cout << "usage: sim_driver [ arguments ] input_dir" << endl;
    cout << "Arguments:" << endl;
  }
  cout << "  --sim-config | -sc config \tConfig mode for simulator, shared L2, and DRAM. Default default. Accepts a config file or any of the predifned options:" << endl;
  cout << "\t\t\t ";
  for(auto i: simConfigs) { 
    if (size + i.size() >  250) {
      cout << endl << "\t\t\t ";
      size = 0;
    }
    cout <<  i << "; ";
    size += i.size();
  }  
  cout << endl;
  cout << "  --core-config | -cc config \tConfig mode for cores and their private L1s. Default inorder. Accepts a config file or any of the predifned options: " << endl;
  cout << "\t\t\t ";
  for(auto i: coreConfigs) { 
    if (size + i.size() >  250) {
      cout << endl << "\t\t\t ";
      size = 0;
    }
    cout <<  i << "; ";
    size += i.size();
  }  
  cout << endl;
  cout << "  --DRAM-config | -dc system device \tConfig mode for DRAM. Takes two arguments: system and device. Default DDRsys and" << endl;
  cout << "\t\t\t DDR3_micron_16M_8B_x8_sg3. Accepts a config file or any of the predifned options:" << endl;
  cout << "\t\t\t ";
  for(auto i: DDRConfigs) { 
    if (size + i.size() >  250) {
      cout << endl << "\t\t\t ";
      size = 0;
    }
    cout <<  i << "; ";
    size += i.size();
  }  
  cout << endl;
  cout << "  --extra_simConfig | -esc parm val \t Change a specific parameter parm for the simulator configuration" << endl; 
  cout<<  "  --extra_coreConfig | -ecc parm val \t Change a specific parameter parm for the core configuration" << endl;
  cout << "  --optmode\t\tRun in optimized mode. No debugging, not stats collection" << endl;
  cout << "  --output-dir | -o dir\tOutput directory. default ./" << endl;
  cout << "  --verbosity | -v lvl\tLevel of verbosity. Accepted values[-1,7], Default -1" << endl; 
  cout << "  --seq_exe | -q lvl\tActivate sequential exection. Default deactivated" << endl; 
  if (print_mutual) mutual_parms::usage();
}

bool
sim_parms::check_inDir() {
  string workingDir = inDir +
    ((decoupled || DeSC) ? "/decades_decoupled_implicit/" : "/decades_base/");
  bool no_errors = true;
  int _nb_cores = 0;

  if (is_directory(workingDir)) {
    for(auto& entry: directory_iterator(workingDir)) {
      if (is_directory(entry.status()))  {
	string name  = entry.path().filename();
	if( name.compare(0, 6, "output_"))
	  _nb_cores++;
      }
    }
    
    if ( (decoupled || DeSC) && _nb_cores % 2 != 0) {
      cerr << "Found odd number of cores in decoupled mode. (" << _nb_cores << ")" << endl;
      no_errors = false;
    }  
  } else {
    cerr << "Input Directory " << workingDir << " does not exists." << endl;
    no_errors = false;
  }
  
  _nb_cores = (decoupled || DeSC) ? _nb_cores / 2 : _nb_cores;
  if (nb_cores != _nb_cores) {
    cerr << "The given number of cores (" << nb_cores << ") and the number of cores found (" << _nb_cores << ") do not match" << endl;
    no_errors = false;
  }
  
  return no_errors;
}

bool
sim_parms::parseArgs(vector<string>& args, bool extract_input) {
  bool found_input = false;

  mutual_parms::parseArgs(args);
  for( auto s = args.begin(); s < args.end(); ) {
    if (*s == "--output-dir" || *s == "-o") {
      args.erase(s);
      outDir = *s;
      args.erase(s);
    } else if (*s == "--verbosity" || *s == "-v") {
      args.erase(s);
      verbosity = stoi(*s);
      args.erase(s);
    } else if ( *s == "--sim_config" || *s == "-sc") {
      args.erase(s);
      simConfig = *s;
      args.erase(s);
    } else if (*s == "--core_config" || *s == "-cc") {
      args.erase(s);
      coreConfig = *s;
      args.erase(s);
    } else if (*s == "--DRAM_config" || *s == "-dc") {
      args.erase(s);
      DRAM_system = *s;
      args.erase(s);
      DRAM_device = *s;
      args.erase(s);
    } else if ( *s == "--optmode" || *s == "-opt") {
      args.erase(s);
      optmode = true;
    } else if ( *s == "--extra_simConfig" || *s == "-esc") {
      args.erase(s);
      string parm = *s; args.erase(s);
      int val = stoi(*s); args.erase(s);
      extra_simConfigs.push_back(make_pair(parm, val));
    } else if ( *s == "--extra_coreConfig" || *s == "-ecc") {
      args.erase(s);
      string parm = *s; args.erase(s);
      int val = stoi(*s); args.erase(s);
      extra_coreConfigs.push_back(make_pair(parm, val));
    } else if (*s == "--help" || *s == "-h") {
      sim_parms::usage();
      exit(1);
    } else if ( *s == "--seq_exe" || *s == "-q") {
      args.erase(s);
      seq_exe = true;
    } else if (extract_input){
      if(!found_input) {
	inDir = *s;
	args.erase(s);
	found_input = true;
      } else {
	cerr<<"Input Directory given twice! first ("<<inDir<<") and now ("<<*s<<")" << endl;
	exit(1);
      }
    } else {
      s++;
    }
  }
  
  if (extract_input) sim_parms::check_inDir();
  return valid();
}

static bool
contains(vector<string> vect, string elem)
{
  bool found = false;
  for(auto item: vect) {
    if (item == elem) {
      found =true;
      break;
    }
  }
  return found;
}

bool
sim_parms::valid() {
  bool no_errors = true;
  bool found = false;
  for (auto config :simConfigs)
    if(config == simConfig){
      found = true; break;
    }
    
  if( !contains(simConfigs, simConfig)  &&  !exists(simConfig) ) {
    cerr << "Unkown simConfig argument (" << simConfig << ")" << endl; 
    no_errors = false;
  }
  if( !contains(coreConfigs, coreConfig) && !exists(coreConfig)) {
    cerr << "Unkown coreConfig argument (" << coreConfig << ")" << endl; 
    no_errors = false;
  }
  if(!contains(DDRConfigs, DRAM_system) && !exists(DRAM_system)) {
    cerr << "Unkown DRAM system  argument (" << DRAM_system << ")" << endl; 
    no_errors = false;
  }
  if( !contains(DDRConfigs, DRAM_device) && !exists(DRAM_device)) {
    cerr << "Unkown DRAM device  argument (" << DRAM_device << ")" << endl; 
    no_errors = false;
  }
  if ( !is_directory(inDir)) {
    cerr << "Input Directory does not exists (" << inDir << ")" << endl;
    no_errors = false;
  }
  if ( !is_directory(outDir)) {
    cerr << "Output Directory does not exists (" << outDir << ")" << endl;
    no_errors = false;
  }
  
  return no_errors;
}

void
sim_parms::update_config(Config &sim_cfg) {
  for( auto config_entry: extra_simConfigs) 
    sim_cfg.set_config(config_entry.first, config_entry.second);
}

void
sim_parms::config_simulator(Simulator *sim) {
  string dyn_file_name;
  string acc_file_name;
  string in_dir;
  ofstream nb_files_file;
  string nb_files_file_name;
  PP_static_Buff<string> *acc_comm;  
  PP_static_Buff<tuple<uint64_t, uint64_t, double, uint64_t>> *acc_to_DRAM;  
  PP_static_Buff<uint64_t> *DRAM_to_acc;  

  acc_to_DRAM = new PP_static_Buff<tuple<uint64_t, uint64_t, double, uint64_t>>(10);
  DRAM_to_acc = new PP_static_Buff<uint64_t>(2);

  /* Configure the Simulator */
  sim->cfg->read(get_simConfig());
  update_config(*sim->cfg);
  sim->cfg->verbLevel = verbosity;

  sim->clockspeed = sim->cfg->chip_freq;

  sim->cache = new LLCache(*sim->cfg, sim->global_stat, nb_cores, nb_cores);

  sim->memInterface = new DRAMSimInterface(sim, sim->cache, sim->cfg->SimpleDRAM, sim->cfg->ideal_cache, sim->cfg->mem_read_ports,
					   sim->cfg->mem_write_ports, get_DDR_system(), get_DDR_device(),
					   acc_to_DRAM, DRAM_to_acc);
  sim->mem_chunk_size = sim->cfg->mem_chunk_size; 
  sim->cache->memInterface = sim->memInterface;
  sim->decoupling = decoupled || DeSC;
  sim->nb_cores = sim->decoupling ? 2*nb_cores : nb_cores;

  acc_comm = new PP_static_Buff<string>(sim->nb_cores);

  /* register all the core to the Simulator */
  if(decoupled || DeSC) {
    dyn_file_name = inDir + "/decades_decoupled_implicit" + "/dyn_data.bin";
    acc_file_name = inDir + "/decades_decoupled_implicit" + "/acc_data.bin";
    nb_files_file_name = inDir + "/decades_decoupled_implicit" + "/nb_files.txt";
    for(int i = 0; i < nb_cores; i++) {
      in_dir = inDir + "/decades_decoupled_implicit/output_compute_" + to_string(i);
      sim->registerCore(in_dir, sim_parms::get_coreConfig(), acc_comm);
    }
    for(int n = nb_cores; n < 2*nb_cores; n++) {
      in_dir = inDir + "/decades_decoupled_implicit/output_supply_" + to_string(n);
      sim->registerCore(in_dir, sim_parms::get_coreConfig(), acc_comm);
    }
  } else {
    dyn_file_name = inDir + "/decades_base" + "/dyn_data.bin";
    acc_file_name = inDir + "/decades_base" + "/acc_data.bin";
    nb_files_file_name = inDir + "/decades_base" + "/nb_files.txt";
    for(int i = 0; i < nb_cores; i++) {
      in_dir = inDir + "/decades_base/output_compute_" + to_string(i);
      sim->registerCore(in_dir, sim_parms::get_coreConfig(), acc_comm);
    }
  }

  sim->nb_files = nb_files;

  /* Handles the files for the dynamic data. Option 2 is habdled in the regiserCore method */
  if (sim->input_files_type == 0) {  
    nb_files_file.open(nb_files_file_name);
    nb_files_file << nb_files << endl;
    nb_files_file.close();

    for(int i = 0; i < nb_files; i++) {
      string name = dyn_file_name + to_string(i);
      string acc_name = acc_file_name + to_string(i);
      remove(name);
      remove(acc_name);
      mkfifo(name.c_str(), 0666);
      mkfifo(acc_name.c_str(), 0666);
      sim->dyn_files.push_back(open(name.c_str(), O_RDONLY | O_NONBLOCK));
      sim->acc_files.push_back(open(acc_name.c_str(), O_RDONLY | O_NONBLOCK));
    }
  } else if (sim->input_files_type == 1) {
    for(int i = 0; i < nb_cores; i++) {
      sim->dyn_files.push_back(open((dyn_file_name+to_string(i)).c_str(), O_RDONLY));
      sim->acc_files.push_back(open((acc_file_name+to_string(i)).c_str(), O_RDONLY));
    }
    if(decoupled || DeSC) 
      for(int i = nb_cores; i < 2*nb_cores; i++) 
	sim->dyn_files.push_back(open((dyn_file_name+to_string(i)).c_str(), O_RDONLY));
  }
  
  /* Configure the cores */
  for(auto tile: sim->tiles) { 
    Core *core = static_cast<Core *>(tile.second);
    for( auto config_entry: extra_coreConfigs) {
      core->local_cfg.set_config(config_entry.first, config_entry.second);
    }
  }

  sim->outputDir = outDir + "/";
  sim->debug_mode  = debug;
  sim->mem_stats_mode = !optmode;
  sim->seq_exe = seq_exe;
  sim->cache->tiles = &sim->tiles;
  sim->cache->nb_cores = sim->nb_cores;
  
  /* set the DRAMSim clockspeed based on Tile0's clockspeed, assume tile 0 is a core */
  sim->memInterface->initialize(sim->tiles[0]->clockspeed, sim->cache->size_of_cacheline); 
  Tile* tile = new Accelerator(sim, sim->cfg->chip_freq, acc_comm, acc_to_DRAM, DRAM_to_acc);

  sim->registerTile(tile);
}

string sim_parms::get_simConfig() {
  if( contains(simConfigs, simConfig))
    return MOSAIC_CONFIG_DIR + "/sim_" +  simConfig + ".txt";
  else if (exists(simConfig))
    return simConfig;
  else {
    cerr << "Simulator's  configuration file is invalid" << endl;
    exit(1);
  }
}

string sim_parms::get_coreConfig() {
  if( contains(coreConfigs, coreConfig))
    return MOSAIC_CONFIG_DIR + "/core_" +  coreConfig + ".txt";
  else if (exists(coreConfig))
    return coreConfig;
  else {
    cerr << "Core's  configuration file is invalid" << endl;
    exit(1);
  }
}

string sim_parms::get_DDR_system() {
  if (contains(DDRConfigs, DRAM_system))
    return MOSAIC_CONFIG_DIR + "/DDR" + DRAM_system + ".txt";
  else if (exists(DRAM_system))
    return DRAM_system;
  else {
    cerr << "DDR system file is invalid" << endl;
    exit(1);
  }
}

string sim_parms::get_DDR_device() {
  if (contains(DDRConfigs, DRAM_device))
    return MOSAIC_CONFIG_DIR + "/DDR" + DRAM_device + ".txt";
  else if (exists(DRAM_device))
    return DRAM_device;
  else {
    cerr << "DDR device file is invalid" << endl;
    exit(1);
  }
} 

ostream& operator<<(ostream& os, const sim_parms& parms) {
  os << "Input Dir: \t\t" << parms.inDir << endl;
  os << "Output Dir: \t\t" << parms.outDir << endl;
  os << "Simulator config: \t" << parms.simConfig << endl;
  os << "Core config: \t\t" << parms.coreConfig << endl;
  os << "Optimized mode :\t" << parms.optmode << endl;
  os << "Verbosity: \t\t" << parms.verbosity << endl;
  os << static_cast<mutual_parms>(parms);
  return os;
}

bool
sim_driver_parms::parseArgs(vector<string>& args) {
  for( auto s = args.begin(); s < args.end(); ) {
    if ( *s == "--ASCI_dyn_data" || *s == "-asci") {
      args.erase(s);
      ASCI_input = true;
    } else if ( *s == "--help" || *s == "-h") {
      sim_parms::usage();
      cout << "  --ASCI_dyn_data | -asci      \tRead the old style ASCI input files (for backward compatibility)" << endl;
      exit(1);
    } else  {
      s++;
    }
  }
  return sim_parms::parseArgs(args);
}

void
sim_driver_parms::config_simulator(Simulator *sim) {
  if(ASCI_input)
    sim->input_files_type = 2;
  else
    sim->input_files_type = 1;
  sim_parms::config_simulator(sim);
}

bool
compiler_parms::parseArgs(vector<string>& args, bool extract_input)  {
  int i, nb_args;
  bool found_input = false;

  mutual_parms::parseArgs(args);
  for( auto s = args.begin(); s < args.end(); ) {
    if ( *s == "--exe_args" || *s == "-e") {
      args.erase(s);
      exe_args += *s;
      args.erase(s);
    } else if (*s == "--DEC_args" || *s == "-D") {
      args.erase(s);
      DEC_args += *s;
      args.erase(s);
    } else if (*s == "--Accelerators" || *s == "-A") {
      args.erase(s);
      use_accelerators = true;
    }else if (*s == "--xterm_debug" || *s == "-X") {
      args.erase(s);
      xterm_debug = true;
    } else if ( *s == "--help" || *s == "-h") {
      usage();
      exit(1);
    } else if (extract_input){
      input_files.push_back(*s);
      args.erase(s);
    } else {
      s++;
    }
  }

  return valid();
}

bool
compiler_parms::valid() {
  for( auto file : input_files)
    if (!exists(file))
      return false;
  return  true;
}
 
void
compiler_parms::usage(bool print_first_line, bool print_mutual) {
  if (print_first_line) { 
    cout << "usage: compiler_driver [ arguments ] input_files" << endl;
    cout << "Arguments:" << endl;
  }

  cout << "  --Accelerators | -A  \t Activate the usage of the accelerators." << endl;
  cout << "  --exe_args | -e arg \t Argument for the compiled executable. If multiple args are passed, they should be surounded by double quotes (\")." <<
    endl << "                   Default empty\t" << endl;
  cout << "  --DEC_args | -D arg \t Additional DEC++ arguments. If multiple args are passed, they should be surounded by double quotes (\"). " << endl <<
    "                           Note, decoupled, debug and verbosity are already updated and should not be added here. Default empty" << endl;
  cout << "  --xterm_debug | -X\t Activate running the native run in a xterm within GDB. " << endl;
  if (print_mutual) mutual_parms::usage();
}

string
compiler_parms::get_DEC_args() {
  string ret;
  if (use_files)
    ret = DEC_args + " --target simulator --simulator_preprocessor_script " + PREPROC_FILES;
  else 
    ret = DEC_args + " --target simulator --simulator_preprocessor_script " + PREPROC_PIPES;
  ret += "  --num_threads ";
  if (decoupled || DeSC)
    ret += to_string(2 * nb_cores);
  else
    ret += to_string(nb_cores);
  if(decoupled)
    ret += " --comp_model di ";
  else if (DeSC) 
    ret += " --comp_model ds ";
  else
    ret += " --comp_model db ";
  if (debug || xterm_debug)
    ret += " --debug ";
  for( auto file : input_files) 
    ret += " " + file;
  if(use_accelerators)
    ret += " -a 1";
  
  return ret;
}

string
compiler_parms::get_exe_args() {
  return exe_args;
}

string
compiler_parms::get_exe_name() {
  string type = (decoupled || DeSC) ? "decades_decoupled_implicit" : "decades_base";
  if (xterm_debug)
    return  "xterm -e gdb  --args " + type + "/" + type;
  else 
    return type + "/" + type;
}

void
compiler_parms::config_compiler() {
  string nb_files_file_name;
  if(decoupled || DeSC)
    nb_files_file_name = string("./decades_decoupled_implicit/") + string("/nb_files.txt");
  else 
    nb_files_file_name = string("./decades_base") + string("/nb_files.txt");    
  ofstream nb_files_file(nb_files_file_name);
  nb_files_file << nb_files << endl;
}

ostream& operator<<(ostream& os, const compiler_parms& parms) {
  os << "USe accelerators: \t\t" << parms.use_accelerators << endl;
  os << "DEC_args: \t\t" << parms.DEC_args << endl;
  os << "exe args: \t\t" << parms.exe_args << endl;
  os << "Input files: ";
  for( auto file: parms.input_files)
    os << file << " ";
  os << endl;
  os << static_cast<mutual_parms>(parms);
  return os;
}

void
mosaic_parms::usage() {
  cout << "usage: mosaic [ arguments ] input_files" << endl;
  cout << "Compiler arguments: " << endl;
  compiler_parms::usage(false, false);
  cout << "Simulator arguments: " << endl;
  sim_parms::usage(false, false);
  cout << "Mutual arguments: " << endl;
  mutual_parms::usage();
}

bool
mosaic_parms::parseArgs(vector<string>& args) {
  for( auto s = args.begin(); s < args.end(); s++) {
    if ( *s == "--help" || *s == "-h") {
      usage();
      exit(1);
    }
  }
  sim_parms::parseArgs(args, false);
  compiler_parms::parseArgs(args, false);
  for(auto arg: args) 
    input_files.push_back(arg);

  return valid();
}

bool
mosaic_parms::valid() {
  bool ret = true;

  ret = ret && mutual_parms::valid();
  ret = ret && sim_parms::valid();
  ret = ret && compiler_parms::valid();
  return ret;
}  
  
ostream&
operator<<(ostream& os, const mosaic_parms& parms) {
  os << static_cast<sim_parms>(parms);
  os << static_cast<compiler_parms>(parms);
  return os;
}
