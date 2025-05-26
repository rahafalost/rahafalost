#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <cassert>
#include <map>
#include "circuit.h"
#include "utils.h"

using namespace std;

void Circuit::display(const bool verbose)
{
  cout << endl
       << "*** Circuit ***" << endl
       << "Number of qubits: " << number_of_qubits << endl
       << "Number of gates: " << number_of_gates << endl
       << "Number of stages: " << number_of_stages << endl;

  // Compute gate distribution
  map<int,int> inputhist;
  for (const auto& pg : circuit)
    for (const auto& g : pg)
      inputhist[g.size()]++;
  cout << "Distribution of gates: ";
  for (const auto& hp : inputhist)
    cout << hp.first << "-input: " << hp.second*100.0/number_of_gates << "%, ";
  cout << endl;
  
  if (verbose)
    for (const auto& parallel_gates : circuit)
      displayGates(parallel_gates, true);
}


bool Circuit::readFromFile(const string& file_name)
{
  circuit.clear();
  ifstream input_file(file_name);
  if (!input_file.is_open())
    return false;

  int min_qubit = numeric_limits<int>::max();
  int max_qubit = numeric_limits<int>::min();
  string line;
  while (getline(input_file, line))
    {
      istringstream iss(line);
      ParallelGates parallel_gates;

      char opening_parenthesis, closing_parenthesis;
      while (iss >> opening_parenthesis)
	{
	  Gate gate;
	  int qubit;

	  while (iss >> qubit)
	    {
	      gate.push_back(qubit);
	      if (qubit < min_qubit) min_qubit = qubit;
	      if (qubit > max_qubit) max_qubit = qubit;
	    }
	  
	  parallel_gates.push_back(gate);
	  number_of_gates++;
	  
	  iss.clear();
	  iss >> closing_parenthesis;
	}

      if (!parallel_gates.empty())
	{
	  circuit.push_back(parallel_gates);
	  number_of_stages++;
	}
    }

  input_file.close();

  if (min_qubit != 0)
    {
      cout << "qubits must start from 0" << endl;
      return false;
    }
  
  number_of_qubits = max_qubit - min_qubit + 1;
  
  return true;
}

void Circuit::generateCircuit(const int nqubits, const int ngates,
			      const vector<float>& gateprob)
{
  circuit.clear();
  int gatecount = 0;
  int nstages = 0;
  set<int> used_qubits;
  ParallelGates parallel_gates;

  while (gatecount != ngates)
    {
      int fanin = getRandomNumber(gateprob) + 1;
      set<int> qubits = getRandomNoRepetition(nqubits, fanin);

      set<int> intersection_set;	
      set_intersection(used_qubits.begin(), used_qubits.end(),
		       qubits.begin(), qubits.end(),
		       inserter(intersection_set, intersection_set.begin()));

      if (intersection_set.empty())
	{
	  // add the gate into the parallel_gates set
	  Gate gate(qubits.begin(), qubits.end());
	  parallel_gates.push_back(gate);
	  gatecount++;
	  used_qubits.insert(qubits.begin(), qubits.end());
	}

      if (!intersection_set.empty() || gatecount == ngates)
	{
	  assert(!parallel_gates.empty());
	  circuit.push_back(parallel_gates);

	  // start a new stage
	  parallel_gates.clear();
	  used_qubits.clear();
	  nstages++;
	}
    }

  // update attributes
  number_of_qubits = nqubits;
  number_of_gates = ngates;
  number_of_stages = nstages;
}
