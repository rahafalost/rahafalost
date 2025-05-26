#include <algorithm>
#ifndef __GATE_H__
#define __GATE_H__

#include <list>

using namespace std;

typedef list<int> Gate;
typedef list<Gate> ParallelGates;
inline bool operator==(const Gate& a, const Gate& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

void displayGate(const Gate& gate, bool newline);
void displayGates(const ParallelGates& gates, bool newline);


#endif
