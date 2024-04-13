#ifndef __PARMS_HPP
#define __PARMS_HPP

#include <stdlib.h>
#include <sys/wait.h>

#include <string>
#include <vector>
#include <iostream>

#include "sim.hpp"
/** \file parms.hpp
    \brief Contains the Parameter classes for the simulator and compiler */
 

/** \brief Executes a command.

    Executes the command name with args as arguments. Returns
    the return code of the command. It also prints the executed 
    command. 
    
    @parm[in] name  A string with the name of the command. 
    @parm[in] args A string containing the arguments of the command. 
    @parm[in] async flag to run the process in the background
    @return The return code of the command. In async mode, it returns if
    the process is successfully launched. 
*/
int
exe(string name, string args, bool async = false);

/** \brief Mutual parameters of the compiler and the simulator \todo
    Make it virtual? */
class mutual_parms {
public:
  /** \brief Prints the usage string */
  void usage();
  /** \brief Parse the args and updated its state 
   *
   * This methods removes the arguments that are found from the
   * vector of strings.  
   *
   * @param args A vector of strings containg all
   * the arguments. The arguments that are found are removed from the
   * vector.
   * \return The validity of this object.
   */
  bool parseArgs(vector<string>& args);
  /** \brief Checks if the parameters "make sence" */
  bool valid();
  
  /** \brief Overloading the << operator */
  friend ostream& operator<<(ostream& os, const mutual_parms& parms);

protected: 
  int nb_cores  = 1;   /** \brief Number of cores used. Note that if running in decoupled mode, 
			   it represents the number of supply/compute pairs. */
  int nb_files = 1; /** Number of files/pipes to use for the communication with the native run. */
  bool debug     = false;   /** \brief Running in debug mode. */
  bool decoupled = false;   /** \brief Running in decoupled mode. */
  bool DeSC = false;   /** \brief Running in DeSC mode. */
};


/** \brief Simulator parameters */
class sim_parms: public virtual mutual_parms {
public:
  /** \bref  Default constructor. 

      Upates simCnfigs coreConfigs and DDRConfigs.  */
  sim_parms();
  /** \brief Prints the usage string. 
     
     If print_first_line is false, then only the parameters are
     printed. If print_mutual is false, then the mutual parameters are
     not printed. 
     
     If extract_input is set to false, then the input dir is nor considered
  */
  void usage(bool print_first_line = true, bool extract_input = true);
  /** \brief Checks if the inDir exists and is correct. 

     Depending on the parameters, checks if the appropriate directory  
     exists and checks if all the core's directory  exists. It also counts
     the number of cores available is equal to the the nb_cores. 
  */
  bool check_inDir();
  /** \brief Configures the sim objects. 

      Reads the simulator's configuration and Registers all the cores
     and update the rest of the sim's parameters.  It also registers
     one accelerator Tile (this was done in the previouse version of
     the driver). It also initializes the DRAM
  */
  void config_simulator(Simulator *sim);
  /** \brief Configures the sim objects. 

      Reads the simulator's configuration and Registers all the cores
     and update the rest of the sim's parameters.  It also registers
     one accelerator Tile (this was done in the previouse version of
     the driver). It also initializes the DRAM
  */
  void update_config(Config &sim_cfg);
  /** \brief  Returns the Simulator's global configuration file. */ 
  string get_simConfig();
  /** \brief  Returns the Cores global configuration file. */ 
  string get_coreConfig();
  /** \brief  Returns the DRAM's system file. */ 
  string get_DDR_system();
  /** \brief  Returns the DRAMs device file.  */ 
  string get_DDR_device();
  /** \brief Parse the args and updated its state. 
      
      This methods removes the arguments that are found from the
      vector of strings.  If check_inDIr is false, then it does not
      call teh check_inDir() method 
  */
  bool parseArgs(vector<string>& args, bool extract_input = true);
  /** \brief Check if its state "makes sence" */
  bool valid();
  /** \brief Overloading the <<  operator. 

      Internaly calls the mutual_parms's << operator  */
  friend ostream& operator<<(ostream& os, const sim_parms& parms);
  
protected: 
  vector<string> simConfigs;
  vector<string> coreConfigs;
  vector<string> DDRConfigs;
  vector<pair<string, int>> extra_simConfigs;
  vector<pair<string, int>> extra_coreConfigs;
  
  string simConfig   = "default";   /** \brief Simulator's configuration */
  string coreConfig  = "inorder"; /** \brief The configuration for all the cores */
  string DRAM_system = "sys"; /** \brief The system for the DRAM */
  string DRAM_device = "3_micron_16M_8B_x8_sg3"; /** \brief The device for the DRAM  */
  string inDir       = "./"; /** \brief Input directory */
  string outDir      = "./"; /** \brief Output directory */
  int verbosity  = -1; /** \brief verbosity level */
  bool optmode   = false; /** \brief run Optimizied mode */
  bool seq_exe   = false; /** \brief run in sequential mode */
};

class sim_driver_parms: public  sim_parms {

public:  
  bool parseArgs(vector<string>& args);
  void  config_simulator(Simulator *sim);
protected:
  bool ASCI_input  = false;
};

  
/** \brief Compiler arguments */
class compiler_parms: public virtual mutual_parms {
public:
  /** \brief Prints the usage string. 
      
      If print_first_line is false, then only the parameters are
      printed. If print_mutual is false, then the mutual parameters are
      not printed. */
  void usage(bool print_first_line = true, bool print_mutual = true);
  /** \brief Parse the args and updated its state. 
      
      This methods removes the arguments that are found from the
      vector of strings.
  */
  bool parseArgs(vector<string>& args, bool extract_intpu = true);
  /** \brief Check if its state "makes sence" */
  bool valid();
  /** \brief Gets the argument list for the DEC++ compiler */
  string get_DEC_args();
  /** \brief Gets the compiled executable name */
  string get_exe_name();
  /** \brief Gets the compiled executable argument liest */
  string get_exe_args();

  void config_compiler();
  /** \brief Overloading the <<  operator. 
      
      Internaly calls the mutual_parms's << operator  */
  friend ostream& operator<<(ostream& os, const compiler_parms& parms);

  /** Flag that tells us if we should use PREPROC_PIPES or PREPROC_FILES */
  bool use_files = false;

protected: 
  bool use_accelerators = false; /** \brief Use the accelerators */
  string DEC_args   = " "; /** \brief Additional DEC++ arguments */
  string exe_args   = " "; /** \brief The compiled executable argumetns */
  vector<string> input_files; /** \brief The input C++ file */
  bool xterm_debug = false; /** \brief The native run will be run in a separate xterm  with GDB */  
};

/** \breif  Mosaic's argumetns */
class mosaic_parms: public sim_parms, public compiler_parms {
public:
  /** \brief Prints the usage string. */
  void usage();
  /** \brief Parse the args and updated its state. 
      
      This methods removes the arguments that are found from the
      vector of strings.
  */
  bool parseArgs(vector<string>& args);
  /** \brief Check if its state "makes sence" */
  bool valid();
  
  /** \brief Overloading the <<  operator. 

      Internaly calls the << operator of the compiler_parms and
      sim_parms classes. */
  friend ostream& operator<<(ostream& os, const mosaic_parms& parms);
};

#endif  // __PARMS_HPP
