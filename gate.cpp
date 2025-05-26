#include <iostream>
#include "gate.h"

//----------------------------------------------------------------------
void displayGate(const Gate& gate, bool newline)
{
  cout << "(";
  for (auto qb = gate.begin(); qb != gate.end(); ++qb)
    {
      cout << *qb;
      if (next(qb) != gate.end())
	cout << " ";
    }
  cout << ") ";

  if (newline)
    cout << endl;
}
		 
// ----------------------------------------------------------------------
void displayGates(const ParallelGates& gates, bool newline)
{
  for (const auto& gate : gates)
    displayGate(gate, false);

  if (newline)
    cout << endl;
}
