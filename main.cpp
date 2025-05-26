#include <iostream>
#include <cmath>
#include <map>
#include <cassert>
#include "utils.h"
#include "architecture.h"
#include "gate.h"
#include "core.h"
#include "circuit.h"
#include "communication.h"
#include "mapping.h"
#include "statistics.h"
#include "communication_time.h"
#include "noc.h"
#include "parameters.h"
#include "simulation.h"
#include "command_line.h"

using namespace std;

		   
int main(int argc, char* argv[])
{
  string circuit_fn, architecture_fn, parameters_fn;
  map<string,string> params_override; // parameter name -> value
  
  if (!checkCommandLine(argc, argv, circuit_fn, architecture_fn, parameters_fn, params_override))
    {
      cerr << "Usage " << argv[0] << " -c <circuit> -a <architecture> -p <parameters> [-o <param> <value>]" << endl;
      
      return -1;
    }

  
  Circuit circuit;
  if (!circuit.readFromFile(circuit_fn))
    {
      cerr << "error reading circuit file" << endl;
      return -2;
    }
  
  Architecture architecture;
  if (!architecture.readFromFile(architecture_fn))
    {
      cerr << "error reading architecture file" << endl;
      return -3;
    }
  
  Parameters parameters;
  if (!parameters.readFromFile(parameters_fn))
    {
      cerr << "error reading parameters file" << endl;
      return -4;
    }

  overrideParameters(params_override, architecture, parameters);

  circuit.display(false);
  architecture.display();
  parameters.display();

  NoC noc(architecture.mesh_x, architecture.mesh_y, architecture.link_width, parameters.noc_clock_time,
	  ceil(log2(architecture.qubits_per_core * architecture.number_of_cores)));
  if (architecture.wireless_enabled)
    noc.enableWiNoC(parameters.wbit_rate, architecture.radio_channels, parameters.token_pass_time);
      
  noc.display();
  
  Mapping mapping(circuit.number_of_qubits, architecture.number_of_cores, architecture.mapping_type);

  Cores cores(architecture, mapping);
  cores.display();
  
  Simulation simulation;
  Statistics stats = simulation.simulate(circuit, architecture, noc, parameters, mapping, cores);
  
  stats.display(circuit, cores, architecture, parameters.stats_detailed);
  
  
  return 0;
}
