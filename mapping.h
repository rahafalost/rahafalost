#ifndef __MAPPING_H__
#define __MAPPING_H__

#include <map>

#define MAP_RANDOM     0
#define MAP_SEQUENTIAL 1

using namespace std;

struct Mapping
{
  map<int,int> qubit2core; // Indicates where a qubit is mapped onto which core

  Mapping() {}

  Mapping(const int nqubits, const int ncores, const int mapping_type);

  void display();

  map<int,int> sequentialMapping(const int nqubits, const int ncores);
  map<int,int> randomMapping(const int nqubits, const int ncores);

  bool isMapped(const int qb) const;
  int qubit2CoreSafe(const int qb) const;
};

#endif
