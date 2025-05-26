#ifndef __NOC_H__
#define __NOC_H__

#include <map>
#include <queue>

#include "communication.h"

using namespace std;

struct NoC
{
  int    mesh_x, mesh_y;
  int    link_width;
  double clock_time;
  int    qubit_addr_bits;
  //  double hop_time;
  //  int    packet_size;
  //  int    cycles_per_packet;
  double wbit_rate;
  int    radio_channels;
  double token_pass_time;
  //  double wpacket_time;
  
  bool   winoc;
  
  NoC(int _mesh_x, int _mesh_y, int _link_width, double _hop_time, int _qubits_per_core);

  void enableWiNoC(const double _bit_rate, const int _radio_channels, double _token_pass_time);
  
  void display();
    
  double getCommunicationTime(const ParallelCommunications& pc) const;
  double getThroughput(int ncomms, double etime) const;

  double getCommunicationTimeWired(const ParallelCommunications& pc) const;
  double getCommunicationTimeWireless(const ParallelCommunications& pc) const;
  
  int routingXY(const int src_core, const int dst_core) const;
  void getCoreXY(const int core_id, int& x, int& y) const;
  int getCoreID(const int x, const int y) const;

  int linkTraversalCycles(int volume) const;
  double getTransferTime(int volume) const;
  map<int,Communication> assignCommunicationIds(const ParallelCommunications& pcomms) const;
  bool commIsInQueue(const int id, const queue<pair<int,int> >& qc) const;
  int computeStartTime(const queue<pair<int,int> >& qc) const;
  bool updateLinksOccupation(map<pair<int,int>, queue<pair<int,int> > >& links_occupation,
			     const int cid, Communication& comm,
			     const int clock_cycle) const;
  int nextClockCycle(const map<pair<int,int>, queue<pair<int,int> > >& links_occupation) const;

};

#endif
