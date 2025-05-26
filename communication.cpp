#include <iostream>
#include "communication.h"


bool Communication::operator<(const Communication& other) const
{
  if (src_core < other.src_core) {
    return true;
  } else if (src_core == other.src_core) {
    return dst_core < other.dst_core;
  } else {
    return false;
  }
}

void Communication::display() const
{
  cout << src_core << "->" << dst_core << "(" << volume << ")";
}

void displayParallelCommunications(const ParallelCommunications& pc)
{
  for (const auto& comm : pc)
    {
      comm.display();
      cout << " ";
    }
  cout << endl;
}

int getTotalCommunicationVolume(const ParallelCommunications& pc)
{
  int volume = 0;

  for (Communication comm : pc)
    volume += comm.volume;

  return volume;
}
