#include <iostream>
#include <fstream>
#include <sstream>
#include "parameters.h"

void Parameters::display() const
{
  cout << endl
       << "*** Parameters ***" << endl
       << "gate delay (s): " << gate_delay << endl
       << "epr delay (s): " << epr_delay << endl
       << "dist delay (s): " << dist_delay << endl
       << "pre delay (s): " << pre_delay << endl
       << "post delay (s): " << post_delay << endl
       << "noc clock time (s): " << noc_clock_time << endl
       << "wbit rate (bps): " << wbit_rate << endl
       << "token pass time (s): " << token_pass_time << endl
       << "memory mandwidth (bps): " << memory_bandwidth << endl
       << "bits instruction (bits): " << bits_instruction << endl
       << "decode time per instruction (s): " << decode_time_per_instruction << endl;
}

bool Parameters::readFromFile(const string& file_name)
{
  ifstream input_file(file_name);
  if (!input_file.is_open())
    return(false);

  string line;
  while (getline(input_file, line))
    {
      istringstream iss(line);
      string param;
      
      iss >> param;
      if (param == string("gate_delay"))
	iss >> gate_delay;
      else if (param == string("epr_delay"))
	iss >> epr_delay;
      else if (param == string("dist_delay"))
	iss >> dist_delay;
      else if (param == string("pre_delay"))
	iss >> pre_delay;
      else if (param == string("post_delay"))
	iss >> post_delay;
      else if (param == string("noc_clock_time"))
	iss >> noc_clock_time;
      else if (param == string("wbit_rate"))
	iss >> wbit_rate;
      else if (param == string("token_pass_time"))
	iss >> token_pass_time;
      else if (param == string("memory_bandwidth"))
	iss >> memory_bandwidth;
      else if (param == string("bits_instruction"))
	iss >> bits_instruction;
      else if (param == string("decode_time_per_instruction"))
	iss >> decode_time_per_instruction;
      else if (param == string("stats_detailed"))
	iss >> stats_detailed;
      else {
	cout << "Invalid patameter reading " << file_name
	     << ": '" << param << "'" << endl;
	return(false);
      }
    }

  input_file.close();
  
  return true;
}

void Parameters::updateGateDelay(const double nv)
{
  gate_delay = nv;
}

void Parameters::updateEPRDelay(const double nv)
{
  epr_delay = nv;
}

void Parameters::updateDistDelay(const double nv)
{
  dist_delay = nv;
}

void Parameters::updatePreDelay(const double nv)
{
  pre_delay = nv;
}

void Parameters::updatePostDelay(const double nv)
{
  post_delay = nv;
}

void Parameters::updateNoCClockTime(const double nv)
{
  noc_clock_time = nv;
}
  
void Parameters::updateWBitRate(const double nv)
{
  wbit_rate = nv;
}

void Parameters::updateTokenPassTime(const double nv)
{
  token_pass_time = nv;
}

void Parameters::updateMemoryBandwidth(const double nv)
{
  memory_bandwidth = nv;
}

void Parameters::updateBitsInstruction(const int nv)
{
  bits_instruction = nv;
}

void Parameters::updateDecodeTime(const double nv)
{
  decode_time_per_instruction = nv;
}

void Parameters::updateStatsDetailed(const bool nv)
{
  stats_detailed = nv;
}
