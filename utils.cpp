#include <algorithm>
#include <random>
#include "utils.h"

//----------------------------------------------------------------------
// returns a random integer between 0 and prob.size()-1 with prob(i) =
// prob[i]
int getRandomNumber(const vector<float> prob)
{
  random_device rd;
  mt19937 gen(rd());

  std::discrete_distribution<int> dist(prob.begin(), prob.end());

  return dist(gen);
}

//----------------------------------------------------------------------
// returns a set of size set_size of random integer without repetition
// between 0 and n-1
set<int> getRandomNoRepetition(const int n, const int set_size)
{
  vector<int> numbers;

  for (int i = 0; i < n; ++i)
    numbers.push_back(i);

  random_device rd;
  mt19937 gen(rd());

  shuffle(numbers.begin(), numbers.end(), gen);

  set<int> result;
  for (int i = 0; i < set_size; ++i)
    result.insert(numbers[i]);

  return result;
}
