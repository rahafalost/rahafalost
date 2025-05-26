#ifndef __CORE_H__
#define __CORE_H__

#include <set>
#include <vector>
#include <list>
#include "mapping.h"
#include "architecture.h"

typedef set<int> Core; // set of qubits mapped in the core

struct Cores
{
  vector<Core> cores;
  list<vector<Core> > history;
  int ancilla_counter;
  
  Cores(const Architecture& architecture, const Mapping& mapping);

  bool allocateAncilla(const int core_id,
		       const Architecture& architecture,
		       Mapping& mapping, int& ancilla);
  int generateAncillaId();
  
  void saveHistory();
  
  void display();  
};

#endif
