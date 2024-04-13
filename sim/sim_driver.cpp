#include <chrono>
#include "tile/Accelerator.hpp"
#include "parms.hpp"

using namespace std;

int
main(int argc, char **argv)  {
  vector<string> args(argv+1, argv+argc); 
  sim_driver_parms parms;

  parms.parseArgs(args);
  cout << parms;
  Simulator *sim = new Simulator(parms.get_simConfig(), parms.get_DDR_system(), parms.get_DDR_device());
  parms.config_simulator(sim);

  sim->run();
  
  return 0;  
}
