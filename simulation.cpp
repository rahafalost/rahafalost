#include <limits>
#include <iostream>
#include <cassert>
#include <cmath>
#include "simulation.h"
#include "gate.h"
#include "communication.h"
#include "communication_time.h"

using namespace std;

// ----------------------------------------------------------------------
bool Simulation::isLocalGate(const Gate& gate, const Mapping& mapping)
{
  assert(!gate.empty());
  
  int core_id = mapping.qubit2CoreSafe(gate.front()); // core where the first qubit of the gate is mapped to

  for (const auto& qb : gate)
    if (mapping.qubit2CoreSafe(qb) != core_id)
      return false;

  return true;
}

// ----------------------------------------------------------------------
void Simulation::splitLocalRemoteGates(const ParallelGates& pgates, const Mapping& mapping,
				       ParallelGates& lgates, ParallelGates& rgates)
{
  lgates.clear();
  rgates.clear();
  
  for (const auto& gate : pgates)
    {
      if (isLocalGate(gate, mapping))
	lgates.push_back(gate);
      else
	rgates.push_back(gate);
    }
}

// ----------------------------------------------------------------------
Statistics Simulation::localExecution(const ParallelGates& lgates,
				      const Parameters& params)
{
  Statistics stats;

  if (!lgates.empty())
    {
      stats.computation_time = params.gate_delay;
      // TODO: add swap contribution like in the remote execution
      stats.executed_gates = lgates.size();
    }
  
  return stats;
}

// ----------------------------------------------------------------------
// Choose the target core based on the number of allocated
// qubits. Select the core with the fewest allocated qubits.  This is
// valid only for telepot_type all-to-all and if option TODO is set
// otherwise it is assumed that the first qubit is the source and the
// second is the destination.
int Simulation::selectDestinationCore(const Architecture& architecture,
				      const Gate& gate, const Mapping& mapping, const Cores& cores)
{
  int selected_core;
  
  if (architecture.teleportation_type == TP_TYPE_MESH || architecture.dst_selection_mode == DST_SEL_LOAD_INDEPENDENT)
    {
      assert(gate.size() == 2);
      auto it = gate.begin(); 
      advance(it, 1);    
      selected_core = mapping.qubit2CoreSafe(*it);
    }
  else
    {
      int min_qb = numeric_limits<int>::max();
      selected_core = -1;
      for (const auto& qb : gate)
	{
	  int core_id = mapping.qubit2CoreSafe(qb);
	  
	  if ((int)cores.cores[core_id].size() < min_qb)
	    {
	      min_qb = cores.cores[core_id].size();
	      selected_core = core_id;
	    }
	}
      
      assert(selected_core != -1);
    }
  
  return selected_core;
}

// ----------------------------------------------------------------------
// qubits in gate are allocated to dst_core. Both mapping and cores
// structures are updated accordingly.
void Simulation::updateMappingAndCores(const Architecture& architecture,
				       Mapping& mapping, Cores& cores,
				       const Gate& gate, const int dst_core)
{
  for (const auto& qb : gate)
    {
      int src_core = mapping.qubit2core[qb];

      if (src_core != dst_core)
	{
	  mapping.qubit2core[qb] = dst_core;
	  cores.cores[dst_core].insert(qb);
	  assert((int)cores.cores[dst_core].size() < architecture.qubits_per_core);
	  
	  auto it = cores.cores[src_core].find(qb);
	  assert(it != cores.cores[src_core].end());
	  cores.cores[src_core].erase(it);
	}
    }
}

// ----------------------------------------------------------------------
// Generate communications from the core where the qubits of gate are
// mapped onto to the dst_core and insert them into
// parallel_communications
void Simulation::addParallelCommunications(ParallelCommunications& parallel_communications,
					   const Gate& gate, const int dst_core,
					   const Mapping& mapping,
					   const int volume)
{
  for (const auto& qb : gate)
    {
      int src_core = mapping.qubit2CoreSafe(qb);
      if (src_core != dst_core)
	{
	  Communication comm(src_core, dst_core, volume);
	  parallel_communications.push_back(comm); // insert(comm);
	}
    }
}

// ----------------------------------------------------------------------
CommunicationTime Simulation::getCommunicationTime(const ParallelCommunications& pcomms,
						   const NoC& noc,
						   const Parameters& params)
{
  CommunicationTime ct;

  ct.t_epr = params.epr_delay;
  ct.t_dist = params.dist_delay;
  ct.t_pre = params.pre_delay;
  ct.t_clas = noc.getCommunicationTime(pcomms);
  ct.t_post = params.post_delay;
  
  return ct;
}

// ----------------------------------------------------------------------
void Simulation::addCommunicationTime(CommunicationTime& total_ct,
				      const CommunicationTime& ct)
{
  total_ct.t_epr  += ct.t_epr;
  total_ct.t_dist += ct.t_dist;
  total_ct.t_pre  += ct.t_pre;
  total_ct.t_clas += ct.t_clas;
  total_ct.t_post += ct.t_post;
}

// ----------------------------------------------------------------------
void Simulation::updateRemoteExecutionStats(Statistics& stats,
				const ParallelGates& pgates,
				const ParallelCommunications& pcomms,
				const NoC& noc,
				const Parameters& params)
{
  stats.executed_gates += pgates.size();

  stats.intercore_comms += pcomms.size();

  stats.intercore_volume += getTotalCommunicationVolume(pcomms);
    
  CommunicationTime comm_time = getCommunicationTime(pcomms, noc, params);
  				       
  addCommunicationTime(stats.communication_time, comm_time);

  // We assume that all the gates in the slice are executed
  // concurrently, each contributing with a gate delay, assuming that
  // the gate delay is constant regardless of the gate type.
  stats.computation_time += params.gate_delay;
}

// ----------------------------------------------------------------------
void Simulation::removeExecutedGates(const ParallelGates& scheduled_gates,
				     ParallelGates& gates)
{
  // Remove scheduled_gates from gates
  gates.remove_if([&scheduled_gates](const Gate &gate) {
    return std::find(scheduled_gates.begin(), scheduled_gates.end(), gate) != scheduled_gates.end();
    });
}

// ----------------------------------------------------------------------
Statistics Simulation::remoteExecution(const Architecture& architecture, const NoC& noc,
				       const Parameters& parameters,
				       const ParallelGates& rgates,
				       Mapping& mapping, Cores& cores)
{
  Statistics stats;

  if (!rgates.empty())
    {
      ParallelGates gates = rgates;

      while (!gates.empty())
	{
	  vector<int> available_ltm_ports(architecture.number_of_cores, architecture.ltm_ports);
	  ParallelGates parallel_gates;
	  ParallelCommunications parallel_communications;
	  
	  bool first_gate_to_map = true;
	  for (const auto& gate : gates)
	    {
	      bool skip_this_gate = false;
	      int dst_core = selectDestinationCore(architecture, gate, mapping, cores);
	      vector<int> tmp_available_ltm_ports = available_ltm_ports;
	      for (const auto& qb : gate)
		{		  
		  int src_core = mapping.qubit2core[qb];
		  if (src_core != dst_core)
		    {
		      if (tmp_available_ltm_ports[src_core] && tmp_available_ltm_ports[dst_core])
			{
			  // qb can be teleported from src_core to dst_core
			  tmp_available_ltm_ports[src_core]--;
			  tmp_available_ltm_ports[dst_core]--;
			}
		      else
			{
			  if (first_gate_to_map)
			    assert(false); // at least the first gate of the parallel_gates must be mapped!
			  skip_this_gate = true;
			  break;
			}
		    }
		} // for (const auto& qb : gate)

	      if (!skip_this_gate)
		{
		  // IMPORTANT: addParallelCommunication must be called before updateMappingAndCores
		  addParallelCommunications(parallel_communications, gate, dst_core, mapping,
					    ceil(log2(2+architecture.qubits_per_core*architecture.number_of_cores))); 
		  available_ltm_ports = tmp_available_ltm_ports;
		  parallel_gates.push_back(gate);
		  updateMappingAndCores(architecture, mapping, cores, gate, dst_core);
		}

	      first_gate_to_map = false;

	    } // for (const auto& gate : gates)
	  
	  updateRemoteExecutionStats(stats, parallel_gates, parallel_communications,
				     noc, parameters);
	  cores.saveHistory();
	  removeExecutedGates(parallel_gates, gates);
	} //  while (!gates.empty())
    }
  
  return stats;
}


// ----------------------------------------------------------------------
Statistics Simulation::mergeLocalRemoteStatistics(const Statistics& stats_local,
						  const Statistics& stats_remote)
{
  Statistics stats;

  // TODO: check this function!!!
  stats.executed_gates = stats_local.executed_gates + stats_remote.executed_gates;
  stats.intercore_comms = stats_remote.intercore_comms;
  stats.intercore_volume = stats_remote.intercore_volume;
  
  stats.communication_time = stats_remote.communication_time;
  stats.computation_time = (stats_local.computation_time > stats_remote.computation_time) ?
    stats_local.computation_time : stats_remote.computation_time;
  
  return stats;
}

// ----------------------------------------------------------------------
void Simulation::fetchContribution(Statistics& stats,
				   const ParallelGates& pgates,
				   const Architecture& architecture,
				   const Parameters& parameters)
{
  int bundle_size = 0;
  int total_qubits = architecture.qubits_per_core * architecture.number_of_cores;
  int bits_qubit_addr = ceil(log2(total_qubits));
  
  for (Gate g : pgates)
    bundle_size += parameters.bits_instruction + g.size() * bits_qubit_addr;

  stats.fetch_time = bundle_size / parameters.memory_bandwidth;
}

// ----------------------------------------------------------------------
void Simulation::decodeContribution(Statistics& stats,
				    const ParallelGates& pgates,
				    const Parameters& parameters)
{
  stats.decode_time = pgates.size() * parameters.decode_time_per_instruction;
  
}

// ----------------------------------------------------------------------
ParallelCommunications Simulation::makeDispatchCommunications(const ParallelGates& pgates,
							      const Architecture& architecture,
							      const Parameters& parameters,
							      const Mapping& mapping)
{
  int bits_qubit_laddr = ceil(log2(architecture.qubits_per_core));
  ParallelCommunications pc;
  
  for (Gate g : pgates)
    {
      int volume = parameters.bits_instruction + g.size() * bits_qubit_laddr;
      // all the qubits of gate in lgates are in the same core. Thus,
      // to determine the target core. The target core cannot be
      // inferred from the gate in general. For the case of
      // teleportation_type == MESH the target core is that hosting
      // the qubit in the secon input of the gate.
      
      assert(g.size() <= 2);
      auto it = g.begin(); 
      advance(it, 1);    
      int qb = *it;      
      int dst_core = mapping.qubit2CoreSafe(qb);
      Communication comm(0, dst_core, volume);
      pc.push_back(comm);
    }

  return pc;
}

// ----------------------------------------------------------------------
// We assume the memory controller is connected to core 0
void Simulation::dispatchContribution(Statistics& stats,
				      const ParallelGates& pgates,
				      const Architecture& architecture,
				      const Parameters& parameters,
				      const Mapping& mapping,
				      const NoC& noc)
{
  ParallelCommunications pcomms = makeDispatchCommunications(pgates, architecture, parameters, mapping);

  if (architecture.wireless_enabled)
    stats.dispatch_time = noc.getTransferTime(getTotalCommunicationVolume(pcomms));
  else {
    stats.dispatch_time = noc.getCommunicationTime(pcomms);

    // Only for wired interconnect add the one-hop delay for communication between MC to core 0.
    stats.dispatch_time += noc.getTransferTime(getTotalCommunicationVolume(pcomms));
  }
  
  stats.intercore_comms += pcomms.size();
  stats.intercore_volume += getTotalCommunicationVolume(pcomms);
}

// ----------------------------------------------------------------------
// Simulate the execution of parallel gates
Statistics Simulation::simulate(const ParallelGates& pgates, const Architecture& architecture,
				const NoC& noc, const Parameters& parameters,
				Mapping& mapping, Cores& cores)
{
  ParallelGates lgates, rgates;

  splitLocalRemoteGates(pgates, mapping, lgates, rgates);  
  
  assert(!rgates.empty() || !lgates.empty());

  Statistics stats_local = localExecution(lgates, parameters);

  Statistics stats_remote = remoteExecution(architecture, noc, parameters,
					    rgates, mapping, cores);

  Statistics stats_overall = mergeLocalRemoteStatistics(stats_local, stats_remote);

  fetchContribution(stats_overall, pgates, architecture, parameters);

  decodeContribution(stats_overall, pgates, parameters);

  dispatchContribution(stats_overall, pgates, architecture, parameters, mapping, noc);
		       
  return stats_overall;
}

// ----------------------------------------------------------------------
void Simulation::freeAncillas(const set<int>& ancilla, Mapping& mapping, Cores& cores)
{
  for (int qba : ancilla)
    {
      int core_id = mapping.qubit2CoreSafe(qba);

      cores.cores[core_id].erase(qba);
      mapping.qubit2core.erase(qba);
    }
}

// ----------------------------------------------------------------------
void Simulation::removeUsedAncillas(set<int>& ancillas, const ParallelGates& pg)
{
  for (const auto& gate : pg) {
    for (int qb : gate) {
      ancillas.erase(qb);
    }
  }
}

// ----------------------------------------------------------------------
set<int> Simulation::getAncillas(const ParallelGates& pg)
{
  set<int> ancillas;

  for (const auto& gate : pg)
    for (int qb : gate)
      if (qb < 0)
	ancillas.insert(qb);
  
  return ancillas;
}

// ----------------------------------------------------------------------
// Remove the ancillas used in the current slice if they are not
// referred in subsequent slices
void Simulation::freeUnusedAncillas(list<ParallelGates>::iterator it_pgates,
				    const list<ParallelGates>& circuit,
				    Mapping& mapping, Cores& cores)
{
    
  set<int> ancillas = getAncillas(*it_pgates);
  
  it_pgates++;
  while (it_pgates != circuit.end() && !ancillas.empty())
    {
      removeUsedAncillas(ancillas, *it_pgates);
      it_pgates++;
    }
  
  // The qubits into ancillas are not used thus they can be released
  freeAncillas(ancillas, mapping, cores);
}

// ----------------------------------------------------------------------
// Simulate the entire circuit
Statistics Simulation::simulate(const Circuit& circuit, const Architecture& architecture,
				const NoC& noc, const Parameters& parameters,
				Mapping& mapping, Cores& cores)
{
  Statistics global_stats;

  cores.saveHistory(); // save the initial state of the cores
  
  // make a copy of the circuit that might be modified when not all-to-all connectivity is used for teleportation
  list<ParallelGates> lcircuit = circuit.circuit;
  
  for (list<ParallelGates>::iterator it_pgates = lcircuit.begin();
       it_pgates != lcircuit.end(); it_pgates++)
    {
      ParallelGates parallel_gates = FixParallelGatesAndUpdateCircuit(it_pgates, lcircuit,
								      architecture, mapping, cores);
      Statistics stats = simulate(parallel_gates, architecture, noc,
				  parameters, mapping, cores);
            
      freeUnusedAncillas(it_pgates, lcircuit, mapping, cores);
      
      double th = noc.getThroughput(stats.intercore_volume,
				    stats.communication_time.getTotalTime());

      global_stats.updateStatistics(stats, th);
      
    }

  return global_stats;
}

// ----------------------------------------------------------------------
vector<int> Simulation::computeTPPathMesh(const int qubit_src, const int qubit_dst,
					  const Architecture& architecture,
					  const Mapping& mapping)
{
  vector<int> path;
  
  int src_core = mapping.qubit2CoreSafe(qubit_src);
  int dst_core = mapping.qubit2CoreSafe(qubit_dst);
  
  // XY routing
  int xs = src_core % architecture.mesh_x;
  int ys = src_core / architecture.mesh_x;
  int xd = dst_core % architecture.mesh_x;
  int yd = dst_core / architecture.mesh_x;

  path.push_back(src_core);
  int x = xs;
  int y = ys;
  while (x != xd || y != yd)
    {
      if (x < xd)
	x++;
      else if (x > xd)
	x--;
      else if (y < yd)
	y++;
      else if (y > yd)
	y--;

      int curr_core = y * architecture.mesh_x + x;
      path.push_back(curr_core);
    }

  return path;
}

// ----------------------------------------------------------------------
// Computhe the path from source qubit to destination qubit based on
// the current teleportation type
vector<int> Simulation::computeTPPath(const int qubit_src, const int qubit_dst,
				      const Architecture& architecture,
				      const Mapping& mapping)
{
  if (architecture.teleportation_type == TP_TYPE_MESH)
    return computeTPPathMesh(qubit_src, qubit_dst, architecture, mapping);
  else
    assert(false);
}

// ----------------------------------------------------------------------
int Simulation::allocateAncilla(const int core_id,
				const Architecture& architecture,
				Mapping& mapping, Cores& cores)
{
  int ancilla;
  
  if (!cores.allocateAncilla(core_id, architecture, mapping, ancilla))
    {
      cerr << "Cannot allocate ancilla on core " << core_id << endl;
      assert(false);
    }

  return ancilla;
}

// ----------------------------------------------------------------------
// This method splits a remote gate into a sequence of remote gates
// involving qubits located in directly connected cores. As new qubits
// (ancilla qubits) are allocated, the mapping and core structures are
// updated accordingly.
ParallelGates Simulation::splitRemoteGate(const Gate& gate,
					  const Architecture& architecture,
					  Mapping& mapping, Cores& cores)
{
  assert(gate.size() == 2); // currently supported only two-input remote gates

  ParallelGates pg;
  
  // We assume that we want to teleport the qubit in input[0] of the
  // gate to the core where the qubit in input[1] of the gate is
  // located.
  auto it = gate.begin();
  int qubit_src = *it;
  ++it;
  int qubit_dst = *it;
  
  vector<int> path = computeTPPath(qubit_src, qubit_dst, architecture, mapping);

  int next_qubit;
  for (size_t i=1; i<path.size(); i++)
    {
      int next_core = path[i];

      if (i == path.size()-1) // next_core is the last core in the path
	next_qubit = qubit_dst;
      else
	next_qubit = allocateAncilla(next_core, architecture, mapping, cores);

      pg.push_back({qubit_src, next_qubit});
    }

  return pg;
}

// ----------------------------------------------------------------------
// This method splits each remote gate into a list of remote gates
// whose input qubits are located in two directly connected cores. As
// new qubits (ancilla qubits) are allocated, the cores and mapping
// are updated accordingly. It returns a list of ParallelGates, where
// each element in the list represents a set of gates resulting from
// the expansion of a remote gate into a sequence of remote gates
// involving qubits belonging to connected cores
list<ParallelGates> Simulation::splitRemoteGates(const ParallelGates& rgates,
						 const Architecture& architecture,
						 Mapping& mapping, Cores& cores)
{
  list<ParallelGates> pgates_list;
  
  for (const auto& gate : rgates)
    pgates_list.push_back(splitRemoteGate(gate, architecture, mapping, cores));
    
  return pgates_list;
}

// ----------------------------------------------------------------------

list<ParallelGates> Simulation::sequenceParallelGates(const ParallelGates& lgates,
						      const list<ParallelGates>& pgates_list_par)
{
  if (pgates_list_par.empty())
    return {lgates};

  size_t max_cols = 0;
  for (const auto& row : pgates_list_par)
    max_cols = max(max_cols, row.size());

  list<ParallelGates> slices(max_cols);

  auto it_out = slices.begin();
  for (size_t col = 0; col < max_cols; ++col, ++it_out)
    {
      for (const auto& row : pgates_list_par)
	{
	  auto it_row = row.begin();
	  if (col < row.size())
	    {
	      advance(it_row, col);
	      it_out->push_back(*it_row);
	    }
	}
    }


  slices.front().insert(slices.front().end(), lgates.begin(), lgates.end());
      
  return slices;
}

// ----------------------------------------------------------------------
// replace the ParallelGates in the circuit pointed by it_pgates with
// the pgates_list_seq and return the first ParallelGate in the list
ParallelGates Simulation::insertSequenceParallelGates(list<ParallelGates>::iterator& it_pgates,
						      list<ParallelGates>& circuit,
						      const list<ParallelGates>& pgates_list_seq)
{

  it_pgates = circuit.erase(it_pgates);
  it_pgates = circuit.insert(it_pgates, pgates_list_seq.begin(), pgates_list_seq.end());
  
  return *it_pgates;
}

// ----------------------------------------------------------------------
// This method applies only when teleportation connectivity is not
// all-to-all. In this case the execution of a remote gate is splitted
// in the execution of different teleportation aimed at moving one of
// the involved qubits from source to destination. The circuit is
// updated accordingly to accommodate the additional introduced slices
ParallelGates Simulation::FixParallelGatesAndUpdateCircuit(list<ParallelGates>::iterator& it_pgates,
							   list<ParallelGates>& circuit,
							   const Architecture& architecture,
							   Mapping& mapping, Cores& cores)
{
  ParallelGates pgates = *it_pgates;

  if (architecture.teleportation_type == TP_TYPE_A2A)
    return pgates; 

  ParallelGates lgates, rgates;
  splitLocalRemoteGates(pgates, mapping, lgates, rgates);
  
  list<ParallelGates> pgates_list_par = splitRemoteGates(rgates, architecture, mapping, cores);
  
  list<ParallelGates> pgates_list_seq = sequenceParallelGates(lgates, pgates_list_par);

  ParallelGates pg = insertSequenceParallelGates(it_pgates, circuit, pgates_list_seq);
  
  return pg;
}
