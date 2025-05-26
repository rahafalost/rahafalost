#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <string>

using namespace std;

struct Parameters
{
  double gate_delay;
  double epr_delay;
  double dist_delay;
  double pre_delay;
  double post_delay;
  double noc_clock_time;
  double wbit_rate;
  double token_pass_time;
  double memory_bandwidth; // bits/sec
  int    bits_instruction; // number of bits used for encoding an instruction
  double decode_time_per_instruction;
  bool   stats_detailed;
  
  Parameters() : gate_delay(0.0), epr_delay(0.0), dist_delay(0.0), pre_delay(0.0), post_delay(0.0), noc_clock_time(0.0), wbit_rate(0.0), token_pass_time(0.0), memory_bandwidth(0.0), bits_instruction(0), decode_time_per_instruction(0.0) {}

  void display() const;

  bool readFromFile(const string& file_name);

  void updateGateDelay(const double nv);
  void updateEPRDelay(const double nv);
  void updateDistDelay(const double nv);
  void updatePreDelay(const double nv);
  void updatePostDelay(const double nv);
  void updateNoCClockTime(const double nv);
  void updateWBitRate(const double nv);
  void updateTokenPassTime(const double nv);
  void updateMemoryBandwidth(const double nv);
  void updateBitsInstruction(const int nv);
  void updateDecodeTime(const double nv);
  void updateStatsDetailed(const bool nv);

};

#endif
