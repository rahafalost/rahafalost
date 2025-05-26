#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>
#include <set>

using namespace std;

// returns a random integer between 0 and prob.size()-1 with prob(i) =
// prob[i]
int getRandomNumber(const vector<float> prob);

// returns a set of size set_size of random integer without repetition
// between 0 and n-1
set<int> getRandomNoRepetition(const int n, const int set_size);


#endif
