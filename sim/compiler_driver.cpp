#include "config.hpp"
#include "sim.hpp"

#include <iostream>

#include "parms.hpp"

using namespace std;

int
main(int argc, char **argv)
{
  vector<string> args(argv+1, argv+argc); 
  compiler_parms parms;
  int ret;
  
  parms.parseArgs(args);
  parms.use_files = true;
  
  cout << parms <<  endl;
  ret = exe(DEC, parms.get_DEC_args());
  if(ret) {
    cerr << "DEC++ has failed with "<< ret << endl; 
    exit(ret);
  }
  
  parms.config_compiler();
  
  ret = exe(parms.get_exe_name(), parms.get_exe_args());
  if(ret) {
    cerr << parms.get_exe_name() << " has failed with " << ret << endl; 
    exit(ret);
  }
  
  return 0;
}
