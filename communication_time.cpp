#include <iostream>
#include <iomanip>
#include "communication_time.h"

using namespace std;

void CommunicationTime::display() const
{
  double total_time = getTotalTime();
  
  cout << "Communication time (s): " << total_time << endl
       << "\tEPR pair generation time (s): " << t_epr << " (" << 100*t_epr/total_time << "%)" << endl
       << "\tEPR pair distribution time (s): " << t_dist << " (" << 100*t_dist/total_time << "%)" << endl
       << "\tPre-processing time (s): " << t_pre << " (" << 100*t_pre/total_time << "%)" << endl
       << "\tClassical transfer time (s): " << t_clas << " (" << 100*t_clas/total_time << "%)" << endl
       << "\tPost-processing time (s): " << t_post << " (" << 100*t_post/total_time << "%)" << endl;
}

double CommunicationTime::getTotalTime() const
{
  return t_epr + t_dist + t_pre + t_clas + t_post;
}
