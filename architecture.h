#ifndef __ARCHITECTURE_H__
#define __ARCHITECTURE_H__

#include <string>

using namespace std;

#define TP_TYPE_A2A  0
#define TP_TYPE_MESH 1

#define DST_SEL_LOAD_INDEPENDENT 0
#define DST_SEL_LOAD_AWARE       1


struct Architecture
{
  int    mesh_x, mesh_y;
  int    link_width;
  int    qubits_per_core;
  int    ltm_ports;
  int    radio_channels;
  int    number_of_cores; // derived value
  bool   wireless_enabled;
  int    teleportation_type;
  int    dst_selection_mode;
  int    mapping_type;
  bool   configured;
  
  Architecture() : configured(false) {}

  void display() const;
  
  bool readFromFile(const string& file_name);

  void updateMeshX(const int nv);
  void updateMeshY(const int nv);
  void updateLinkWidth(const int nv);
  void updateQubitsPerCore(const int nv);
  void updateLTMPorts(const int nv);
  void updateRadioChannels(const int nv);
  void updateWirelessEnabled(const int nv);
  void updateTeleportationType(const int nv);
  void updateDstSelectionMode(const int nv);
  void updateMappingType(const int nv);

  void updateDerivedVariables();
};

#endif
