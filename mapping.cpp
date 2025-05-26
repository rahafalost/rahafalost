#include <iostream>
#include <cassert>
#include <vector>
#include <cstdlib>
#include <random>
#include <chrono>
#include <algorithm>
#include "mapping.h"


Mapping::Mapping(const int nqubits, const int ncores, const int mapping_type)
{
  if (mapping_type == MAP_SEQUENTIAL)
    qubit2core = this->sequentialMapping(nqubits, ncores);
  else if (mapping_type == MAP_RANDOM)
    qubit2core = this->randomMapping(nqubits, ncores);
  else {
    cerr << "Invalid mapping type" << endl;
    assert(false);
  }
}

void Mapping::display()
{
  cout << endl
       << "*** Mapping ***" << endl;
  
  int nqb = qubit2core.size();    
  for (int qb=0; qb<nqb; qb++)
    {
      assert(isMapped(qb));
      cout << "qubit " << qb << " -> core " << qubit2core[qb] << endl;
    }
}

map<int,int> Mapping::sequentialMapping(const int nqubits, const int ncores)
{
  map<int,int> q2c;
  
  int core_id = 0;
  for (int qb=0; qb<nqubits; qb++)
    {
      q2c[qb] = core_id;
      
      core_id = (core_id + 1) % ncores;
    }
  
  return q2c;
}

map<int,int> Mapping::randomMapping(const int nqubits, const int ncores)
{
  srand(time(0));

  map<int, int> mapping;
  vector<int>   cores(nqubits);

  for (int i = 0; i < nqubits; ++i)
    cores[i] = i % ncores;
    

  unsigned seed = chrono::high_resolution_clock::now().time_since_epoch().count();
  mt19937 gen(seed);
  shuffle(cores.begin(), cores.end(), gen);

  for (int i = 0; i < nqubits; ++i)
    mapping[i] = cores[i];

  return mapping;
}

bool Mapping::isMapped(const int qb) const
{
  return (qubit2core.find(qb) != qubit2core.end());
}

int Mapping::qubit2CoreSafe(const int qb) const
{
  assert(qubit2core.find(qb) != qubit2core.end());

  return qubit2core.at(qb);
}
