#include <limits>
#include <iostream>
#include <cmath>
#include <cassert>
#include "noc.h"

NoC::NoC(int _mesh_x, int _mesh_y, int _link_width, double _clock_time, int _qubit_addr_bits)
{
  winoc = false;
  
  mesh_x = _mesh_x;
  mesh_y = _mesh_y;
  link_width = _link_width;
  clock_time = _clock_time;
  qubit_addr_bits = _qubit_addr_bits;
  
  // int address_size = ceil(log2(mesh_x * mesh_y * _qubits_per_core));

  /*
  packet_size = 2 + 2*address_size; // 2 bits per teleportation
				    // protocol, 2*address_size bit
				    // for src and dst
  cycles_per_packet = ceil((double)packet_size / link_width);
  */
}

void NoC::enableWiNoC(const double _bit_rate, const int _radio_channels, double _token_pass_time)
{
  winoc = true;

  wbit_rate = _bit_rate;
  radio_channels = _radio_channels;
  token_pass_time = _token_pass_time;
}

void NoC::display()
{
  if (!winoc)
    {
      cout << endl
	   << "*** NoC ***" << endl
	   << "mesh_x x mesh_y: " << mesh_x << "x" << mesh_y << endl
	   << "clock period (s): " << clock_time << endl
	   << "link width (bits): " << link_width << endl;
    }
  else
    {
      cout << "*** WiNoC ***" << endl
	   << "bit rate (bps): " << wbit_rate << endl
	   << "radio channels: " << radio_channels << endl
	   << "token pass time (s): " << token_pass_time << endl;
    }
}


map<int,Communication>  NoC::assignCommunicationIds(const ParallelCommunications& pcomms) const
{
  map<int,Communication> pcomms_id;
  int id = 0;

  for (Communication comm : pcomms)
    pcomms_id.insert({id++, comm});
  
  return pcomms_id;
}

bool NoC::commIsInQueue(const int id, const queue<pair<int,int> >& qc) const
{
  queue<pair<int,int> > temp = qc;

  while (!temp.empty())
    {
      if (temp.front().first == id)
	return true;

      temp.pop();
    }
  
  return false;
}

int NoC::computeStartTime(const queue<pair<int,int>, deque<pair<int,int>>>& qc) const
{
  /*
  int st = 0;
  queue<pair<int,int> > temp = qc;

  while (!temp.empty())
    {
      // st += temp.front().second;
      st = temp.front().second; 
      temp.pop();
    }

  return st;
  */
  
  return qc.back().second;
}

int NoC::nextClockCycle(const map<pair<int,int>, queue<pair<int,int> > >& links_occupation) const
{
  // the next clock cycle corresponds to the minimum waiting time in the links_occupation structure

  int min_cc = std::numeric_limits<int>::max();
  for (const auto& l : links_occupation)
    {
      queue<pair<int,int> > q = l.second;
      while (!q.empty())
	{
	  int cc = q.front().second;
	  q.pop();
	  
	  if (cc < min_cc)
	    min_cc = cc;
	}
    }

  return min_cc;
}

bool NoC::updateLinksOccupation(map<pair<int,int>, queue<pair<int,int>, deque<pair<int,int>>>>& links_occupation,
				const int cid, Communication& comm,
				const int clock_cycle) const
{
  bool drained = false;
  
  int next_core = routingXY(comm.src_core, comm.dst_core);

  pair<int,int> link(comm.src_core, next_core);

  auto it_links_occupation = links_occupation.find(link);

  if (it_links_occupation == links_occupation.end())
    {
      // comm_id is the first one traversing the link
      queue<pair<int,int> > q;
      q.push(pair<int,int>(cid, clock_cycle + linkTraversalCycles(comm.volume)));
      links_occupation[link] = q;
    }
  else
    {
      // link is already used
      if (!commIsInQueue(cid, it_links_occupation->second))
	{
	  // if the link exists then the queue must contains communications otherwise
	  // the link is removed from the links_occupation structure
	  assert(!it_links_occupation->second.empty());

	  // if comm is not yet in queue for the link, add it in queue. It will start when all the
	  // previous communications left the link
	  it_links_occupation->second.push(pair<int,int>(cid,
							 computeStartTime(it_links_occupation->second) +
							 linkTraversalCycles(comm.volume)));	  
	}
      else
	{
	  // communication is in queue. If it is in the front and its
	  // release time passed than it can advance and removed from
	  // the queue
	  if (it_links_occupation->second.front().first == cid &&
	      clock_cycle >= it_links_occupation->second.front().second)
	    {
	      comm.src_core = next_core;

	      // check if drained
	      if (comm.src_core == comm.dst_core)
		drained = true;

	      // remove the communication from the queue of the current link
	      it_links_occupation->second.pop();

	      // check if the queue is empty and in this case remove the link from links_occupation
	      if (it_links_occupation->second.empty())
		links_occupation.erase(it_links_occupation);
	    }
	}
    }

  return drained;
}

double NoC::getCommunicationTimeWired(const ParallelCommunications& pcomms) const
{
  map<pair<int,int>, queue<pair<int,int>, deque<pair<int,int>>>> links_occupation; // links_occupation[(node1,node2)] --> queue of pairs (comm_id, when the link is released)
  map<int,Communication> pcomms_id = assignCommunicationIds(pcomms);
  int clock_cycle = 0;
  
  while (!pcomms_id.empty())
    {      
      // DEBUG      cout << endl << endl << "cc: " << clock_cycle << endl;
      
      for (map<int,Communication>::iterator it = pcomms_id.begin(); it != pcomms_id.end(); )
	{
	  bool drained = updateLinksOccupation(links_occupation, it->first, it->second, clock_cycle);
	  /* DEBUG
	  cout << "\tcomm id " << it->first << ": " << it->second.src_core << "-->" << it->second.dst_core << " (" << it->second.volume << ")" << endl;
	  cout << "\tlinks occupation:" << endl;
	  
	  for (const auto& lo : links_occupation)
	    {
	      cout << "\t" << lo.first.first << "-->" << lo.first.second << ": ";
	      queue<pair<int,int> > q = lo.second;
	      while (!q.empty())
		{
		  cout << "(" << q.front().first << "," << q.front().second << "), ";
		  q.pop();
		}
	      cout << endl;
	    }
	  */
	  
	  if (drained)
	    it = pcomms_id.erase(it);
	  else
	    ++it;
	}

      if (!links_occupation.empty())
	clock_cycle = nextClockCycle(links_occupation);
    }

  return clock_cycle * clock_time;
}

double NoC::getCommunicationTimeWireless(const ParallelCommunications& pcomms) const
{
  double ctime = 0.0;

  for (Communication comm : pcomms)
    ctime += getTransferTime(comm.volume);
  
  return ctime;
}

double NoC::getCommunicationTime(const ParallelCommunications& pcomms) const
{
  if (!winoc)
    return getCommunicationTimeWired(pcomms);
  else
    return getCommunicationTimeWireless(pcomms);  
}

double NoC::getTransferTime(int volume) const
{
  if (!winoc)
    return volume / (link_width / clock_time);
  else
    {
        int nodes = mesh_x * mesh_y;
	double avg_token_waiting_time = (nodes/2) * token_pass_time / radio_channels;
	return avg_token_waiting_time + (volume / wbit_rate);
    }

}

int NoC::linkTraversalCycles(int volume) const
{
  return ceil((double)volume/link_width);
}

int NoC::routingXY(const int src_core, const int dst_core) const
{
  int x, y, xd, yd;
  
  getCoreXY(src_core, x, y);
  getCoreXY(dst_core, xd, yd);

  if (x < xd)
    x++;
  else if (x > xd)
    x--;
  else if (y < yd)
    y++;
  else if (y > yd)
    y--;

  return getCoreID(x, y);
}

void NoC::getCoreXY(const int core_id, int& x, int& y) const
{
  x = core_id % mesh_x;
  y = core_id / mesh_x;
}

int NoC::getCoreID(const int x, const int y) const
{
  return y * mesh_x + x;
}


double NoC::getThroughput(int volume, double etime) const
{
  return volume / etime;
}
