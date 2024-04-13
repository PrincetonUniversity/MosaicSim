#include <chrono>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.hpp"
#include "parms.hpp"

using namespace std;


int
main(int argc, char **argv){
  int ret;
  vector<string> args(argv+1, argv+argc); 
  mosaic_parms parms;
  Simulator *sim;
  
  parms.parseArgs(args);

  cout << parms <<  endl;
  ret = exe(DEC, parms.get_DEC_args());
  if(ret) {
    cerr << DEC << " has failed with "<< ret << endl; 
    exit(ret);
  }
  cout << parms <<  endl;
  
  sim = new Simulator(parms.get_simConfig(), parms.get_DDR_system(), parms.get_DDR_device());
  
  parms.config_simulator(sim);

  ret = exe(parms.get_exe_name(), parms.get_exe_args(), true);

  if(ret) {
    cerr << parms.get_exe_name() << " has failed with " << ret << endl;
    return ret;
  }

  sim->run();

  return 0;
}
