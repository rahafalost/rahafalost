#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <random>
#include <string>


using namespace std;
using Gate = vector<int>;

using GateGrover = tuple<string, vector<int>>;
using CircuitSlicesGrover = vector<vector<GateGrover>>;


void gate1(const int q, const bool nl)
{
  cout << "(" << q << ") ";
  if (nl)
    cout << endl;
}

void gate2(const int q1, const int q2, const bool nl)
{
  cout << "(" << q1 << " " << q2 << ") ";
  if (nl)
    cout << endl;
}

void gate3(const int q1, const int q2, const int q3, const bool nl)
{
  cout << "(" << q1 << " " << q2 << " " << q3 << ") ";
  if (nl)
    cout << endl;
}

void qft(const int nqubits, const int batch)
{
  int nqubits_per_batch = nqubits / batch;
  
  for (int qb1=0; qb1<nqubits_per_batch; qb1++)
    {
      gate1(qb1, true);
      for (int qb2=qb1+1; qb2<nqubits_per_batch; qb2++)
	{
	  for (int b=0; b<batch; b++)
	    gate2(qb1+(b * nqubits_per_batch), qb2+(b * nqubits_per_batch), false);
	  cout << endl;
	}
    }
}

void cuccaroAdder(const int n)
{
  for (int i=0; i<n; i++)
    {
      gate2(i, n+i, false);
      if ( (i+1) < n )
	gate2(i+1, n+i+1, true);
    }
}


vector<vector<Gate>> generateQuantumVolumeCircuit(const int n) {
    int depth = n; // Profondità = numero di qubit (come in Qiskit)
    vector<vector<Gate>> circuit; // Lista di slice
    set<int> activeQubits; // Qubit già usati in una slice
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> gateType(1, 2);  // Solo gate a 1 o 2 ingressi
    uniform_int_distribution<int> qubitSelector(0, n - 1);

    for (int d = 0; d < depth; d++) {
        vector<Gate> slice; // Slice corrente
        activeQubits.clear();

        while ((int)activeQubits.size() < n) {
            int size = gateType(gen);  // Numero di qubit del gate (1 o 2)
            Gate gate;
            
            while ((int)gate.size() < size && (int)activeQubits.size() + (int)gate.size() < n) {
                int q = qubitSelector(gen);
                if (activeQubits.find(q) == activeQubits.end()) {
                    gate.push_back(q);
                    activeQubits.insert(q);
                }
            }

            if (!gate.empty()) {
                slice.push_back(gate);
            }
        }
        circuit.push_back(slice);
    }

    return circuit;
}

void quantumVolume(const int n)
{
  vector<vector<Gate>> circuit = generateQuantumVolumeCircuit(n);

  for (size_t i = 0; i < circuit.size(); i++)
    {
        for (size_t j = 0; j < circuit[i].size(); j++) {
            cout << "(";
            for (size_t k = 0; k < circuit[i][j].size(); k++) {
                cout << circuit[i][j][k];
                if (k < circuit[i][j].size() - 1) cout << " ";
            }
            cout << ")";
            if (j < circuit[i].size() - 1) cout << " ";
        }
        cout << endl;
    }
}






// Aggiunge un gate a una slice
void add_gate(CircuitSlicesGrover &slices, const string &gate_name, vector<int> qubits) {
    if (slices.empty() || gate_name == "CZ") {
        slices.push_back({});
    }
    slices.back().push_back(make_tuple(gate_name, qubits));
}

// Costruisce il circuito di Grover
CircuitSlicesGrover grover_circuit(int n) {
    CircuitSlicesGrover slices;

    // 1. Applicare H a tutti i qubit (inizializzazione)
    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("H", vector<int>{i}));
    }

    // 2. Oracle: Supponiamo di voler marcare |111...1>
    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("X", vector<int>{i}));
    }
    
    slices.push_back({});
    slices.back().push_back(make_tuple("CZ", vector<int>{n-2, n-1})); // Controllo di fase

    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("X", vector<int>{i}));
    }

    // 3. Diffusione di Grover (amplificazione dell'ampiezza)
    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("H", vector<int>{i}));
    }

    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("X", vector<int>{i}));
    }

    slices.push_back({});
    slices.back().push_back(make_tuple("CZ", vector<int>{n-2, n-1})); // Controllo di fase

    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("X", vector<int>{i}));
    }

    slices.push_back({});
    for (int i = 0; i < n; i++) {
        slices.back().push_back(make_tuple("H", vector<int>{i}));
    }

    return slices;
}

void grover(const int n)
{
  CircuitSlicesGrover slices = grover_circuit(n);

  for (const auto &slice : slices) {
    for (const auto &gate : slice) {
      vector<int> qubits = get<1>(gate);
      if (qubits.size() == 1) {
	gate1(qubits[0], false);  
      } else if (qubits.size() == 2) {
	gate2(qubits[0], qubits[1], false);
      }
    }
    cout << endl;
  }
}


int main(int argc, char* argv[])
{
  if (argc < 3)
    {
      cerr << "Usage " << argv[0] << " <circuit> <qubits> [<batch>]" << endl
	   << "circuits: qft, grover, cuccaro, qv" << endl;
      
      return 1;
    }

  string cname = argv[1];
  int    nqubits = stoi(argv[2]);
  int    batch;
  
  if (argc == 3)
    batch = 1;
  else
    batch = stoi(argv[3]);
  
  if (cname == "qft")
    qft(nqubits, batch);
  else if (cname == "cuccaro")
    cuccaroAdder(nqubits/2);
  else if (cname == "qv")
    quantumVolume(nqubits);
  else if (cname == "grover")
    grover(nqubits);
  
  return 0;
}
