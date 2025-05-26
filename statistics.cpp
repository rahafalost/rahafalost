#include <cmath>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cassert>
#include "statistics.h"

using namespace std;

Statistics::Statistics()
{
  executed_gates = 0;
  intercore_comms = 0;
  intercore_volume = 0;
  computation_time = 0.0;
  avg_throughput = 0.0;
  max_throughput = 0.0;
  samples = 0;
  fetch_time = 0.0;
  decode_time = 0.0;
  dispatch_time = 0.0;
}


int Statistics::countCommunications(const Cores& cores, const int src, const int dst)
{
  int n = 0;
  
  for (list<vector<Core> >::const_iterator it_curr = cores.history.begin();
       it_curr != cores.history.end(); ++it_curr)
    {
      list<vector<Core> >::const_iterator it_next = next(it_curr);
      if (it_next != cores.history.end())
	{
	  Core core_src = (*it_curr)[src];
	  Core core_dst = (*it_next)[dst];

	  vector<int> intersection;
	  set_intersection(core_src.begin(), core_src.end(),
			   core_dst.begin(), core_dst.end(),
			   inserter(intersection, intersection.begin()));

	  n += intersection.size();
	}
    }
  
  return n;
}

vector<vector<int> > Statistics::getIntercoreCommunications(const Cores& cores)
{
  /*
  int t = 0;
  for (const auto& vcores : cores.history)
    {
      cout << "time " << t++ << endl;
      for (int i=0; i<vcores.size(); i++)
	{
	  cout << "core " << i << ": ";
	  for (int q : vcores[i])
	    cout << q << ", ";
	  cout << endl;
	}
      cout << endl;
    }
  */
  
  int ncores = cores.cores.size();
  vector<vector<int> > icc(ncores, vector<int>(ncores, 0)); // icc[s][d] = number of communications from s to d 

  for (int s=0; s<ncores; s++)
    for (int d=0; d<ncores; d++)
      if (s != d)
	icc[s][d] = countCommunications(cores, s, d);
  
  return icc;
}

void Statistics::displayIntercoreCommunications(const Cores& cores)
{
  vector<vector<int> > icc = getIntercoreCommunications(cores);

  int ncores = cores.cores.size();

  for (int s=0; s<ncores; s++)
    {
      for (int d=0; d<ncores; d++)
	cout << icc[s][d] << " ";
      cout << endl;
    }
}

vector<int> Statistics::getOperationsPerQubit(const Circuit& circuit)
{
  vector<int> opsqb(circuit.number_of_qubits, 0);

  for (ParallelGates pg : circuit.circuit)
    for (Gate g : pg)
      for (int qb : g)
	opsqb[qb]++;

  return opsqb;
}

void Statistics::displayOperationsPerQubit(const Circuit& circuit)
{
  vector<int> opsqb = getOperationsPerQubit(circuit);

  for (int ops : opsqb)
    cout << ops << ", ";
  cout << endl;
}

// return core idx where qb is located. -1 if not found
int Statistics::qbitToCore(const int qb, const vector<Core> cores)
{
  int n = cores.size();

  for (int i=0; i<n; i++)
    if (cores[i].find(qb) != cores[i].end())
      return i;

  return -1;
}

int Statistics::getTeleportationsPerQubit(const int qb, const Cores& cores)
{
  int tps = 0;

  for (list<vector<Core> >::const_iterator it_curr = cores.history.begin();
       it_curr != cores.history.end(); ++it_curr)
    {
      list<vector<Core> >::const_iterator it_next = next(it_curr);
      if (it_next != cores.history.end())
	{
	  int core_s = qbitToCore(qb, *it_curr);
	  int core_d = qbitToCore(qb, *it_next);
	  assert(core_s != -1 && core_d != -1);

	  if (core_s != core_d)
	    tps++;
	}
    }
  
  return tps;
}

vector<int> Statistics::getTeleportationsPerQubit(const Circuit& circuit, const Cores& cores)
{
  vector<int> tpsqb(circuit.number_of_qubits);

  for (int qb = 0; qb < circuit.number_of_qubits; qb++)
    tpsqb[qb] = getTeleportationsPerQubit(qb, cores);
  
  return tpsqb;
}

void Statistics::displayTeleportationsPerQubit(const Circuit& circuit, const Cores& cores)
{
  vector<int> tpsqb = getTeleportationsPerQubit(circuit, cores);

  for (int tps : tpsqb)
    cout << tps << ", ";
  cout << endl;
  
}

void Statistics::display(const Circuit& circuit, const Cores& cores, const Architecture& arch,
			 const bool detailed)
{
  cout << endl
       << "*** Statistics ***" << endl
       << "Executed gates: " << executed_gates << endl
       << "Intercore communications: " << intercore_comms << endl
       << "Intercore traffic volume (bits): " << intercore_volume << endl
       << "Throughput (Mbps): " << avg_throughput/1.0e6 << " avg, " << max_throughput/1.0e6 << " peak"
       << endl;

  
  double avg, min, max;
  getCoresStats(cores.history, arch, avg, min, max);
  cout << "Core utilization: " << avg << " avg, " << min << " min, " << max << " max" << endl;

  if (detailed) {
    cout << "Intercore communications (row is source, col is target):" << endl;
    displayIntercoreCommunications(cores);
  }

  if (detailed) {
    cout << "Operations per qubit: ";
    displayOperationsPerQubit(circuit);
  }

  if (detailed) {
    cout << "Teleportations per qubit: ";
    displayTeleportationsPerQubit(circuit, cores);
  }

  
  communication_time.display();
  double execution_time = computation_time + communication_time.getTotalTime() + fetch_time + decode_time + dispatch_time;
  cout << "Computation time (s): " << computation_time << endl
       << "Fetch time (s): " << fetch_time << endl
       << "Decode time (s): " << decode_time << endl
       << "Dispatch time (s): " << dispatch_time << endl
       << "Execution time (s): " << execution_time << endl
       << "Coherence (%): " << 100.0*exp(-execution_time / 268e-6) << endl;
  
}

void Statistics::updateStatistics(const Statistics& stats, const double th)
{
  executed_gates += stats.executed_gates;
  intercore_comms += stats.intercore_comms;
  intercore_volume += stats.intercore_volume;
  computation_time += stats.computation_time;
  
  communication_time.t_epr += stats.communication_time.t_epr;
  communication_time.t_dist += stats.communication_time.t_dist;
  communication_time.t_pre += stats.communication_time.t_pre;
  communication_time.t_clas += stats.communication_time.t_clas;
  communication_time.t_post += stats.communication_time.t_post;

  fetch_time += stats.fetch_time;
  decode_time += stats.decode_time;
  dispatch_time += stats.dispatch_time;
  
  // update throughput stats
  if (th > 0.0)
    {      
      if (th > max_throughput)
	max_throughput = th;

      avg_throughput = (samples * avg_throughput) / (1 + samples) +
	th / (samples + 1);

      samples++;
  }
}


void Statistics::getCoresStats(const vector<Core>& cores, const Architecture& arch,
			       double& avg_u, double& min_u, double& max_u)
{
  min_u = numeric_limits<double>::max();
  max_u = numeric_limits<double>::min();
  double sum_u = 0.0;
  
  for (const auto& core : cores)
    {
      double utilization = (double)core.size() / arch.qubits_per_core;
      sum_u += utilization;
      if (utilization < min_u) min_u = utilization;
      if (utilization > max_u) max_u = utilization;      
    }

  avg_u = sum_u / cores.size();
}

void Statistics::getCoresStats(const list<vector<Core> >& history, const Architecture& arch,
			       double& avg_u, double& min_u, double& max_u)
{
  double _avg_u, _min_u, _max_u;
  double sum_avg_u = 0.0;
  min_u = numeric_limits<double>::max();
  max_u = numeric_limits<double>::min();
  
  for (const auto& coresh : history)
    {
      getCoresStats(coresh, arch, _avg_u, _min_u, _max_u);

      sum_avg_u += _avg_u;
      if (_min_u < min_u) min_u = _min_u;
      if (_max_u > max_u) max_u = _max_u;
    }

  avg_u = sum_avg_u / history.size();
}
