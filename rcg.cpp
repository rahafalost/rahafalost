#include <iostream>
#include <vector>
#include <cassert>
#include "gate.h"
#include "circuit.h"

using namespace std;


bool checkCommandLine(int argc, char* argv[],
		      int& nqubits, int& ngates, vector<float>& prob)
{
  if (argc < 4)
    return false;

  nqubits = atoi(argv[1]);
  ngates = atoi(argv[2]);
  prob.clear();
  for (int i=3; i<argc; i++)
    prob.push_back(atof(argv[i]));
  
  return true;
}

bool checkProbabilities(const vector<float>& vprob)
{
  float sum = 0.0;

  for (const auto& prob : vprob)
    sum += prob;

  return (sum == 1.0);
}

int main(int argc, char* argv[])
{

  int nqubits, ngates;
  vector<float> prob;
  if (!checkCommandLine(argc, argv, nqubits, ngates, prob))
    {
      cout << "Use " << argv[0] << " <nqubits> <ngates> <prob1 prob2 ... prob_n>" << endl;
      assert(false);
    }

  if (!checkProbabilities(prob))
    {
      cout << "ERROR: sum of probabilities must be 1" << endl;
      assert(false);
    }
  
  Circuit circuit;
  //  vector<float> prob = {0.5, 0.4, 0.1};
  //circuit.generateCircuit(8, 30, prob);
  circuit.generateCircuit(nqubits, ngates, prob);
  
  for (const auto& parallel_gates : circuit.circuit)
    displayGates(parallel_gates, true);

  return 0;
}
