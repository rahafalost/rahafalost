#include <iostream>
#include <cassert>
#include "core.h"

Cores::Cores(const Architecture& architecture, const Mapping& mapping)
{
  ancilla_counter = 0;
  
  cores.resize(architecture.number_of_cores);
  
  int nqubits = mapping.qubit2core.size();
  for (int qb=0; qb<nqubits; qb++)
    {
      if (!mapping.isMapped(qb))
	{
	  cerr << "qubit " << qb << " is not mapped!" << endl;
	  assert(false);
	}
      int core_no = mapping.qubit2core.at(qb);
      
      cores[core_no].insert(qb);
      
      if ((int)cores[core_no].size() > architecture.qubits_per_core)
	{
	  cout << "Number of qubits mapped on core " << core_no
	       << " exceeds its capacity." << endl;
	  assert(false);
	}
    }
}

void Cores::saveHistory()
{
  history.push_back(cores);
}

void Cores::display()
{
  cout << endl
       << "*** Cores ***" << endl;
  int ncores = cores.size();  
  for (int core_id=0; core_id<ncores; core_id++)
    {
      cout << "core " << core_id << ": ";
      
      for (const auto& qubit : cores[core_id])
	cout << qubit << " ";
      cout << endl;
    }
}

int Cores::generateAncillaId()
{
  ancilla_counter--;
  return ancilla_counter;
}

bool Cores::allocateAncilla(const int core_id,
			    const Architecture& architecture,
			    Mapping& mapping, int& ancilla)
{
  Core core = cores[core_id];

  if ( core.size() >= static_cast<size_t>(architecture.qubits_per_core) )
    return false;

  ancilla = generateAncillaId();
  cores[core_id].insert(ancilla);
  mapping.qubit2core[ancilla] = core_id;
  
  return true;
}

