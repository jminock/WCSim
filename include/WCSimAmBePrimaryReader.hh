#ifndef WCSIMAMBEPRIMARYREADER_HH
#define WCSIMAMBEPRIMARYREADER_HH

#include <string>
#include <vector>

#include "Math/Vector4D.h"

class TFile;
class TTree;

struct WCSimAmBeParticle {
  int track_id;
  int parent_id;
  int pdg;
  ROOT::Math::XYZTVector position;
  ROOT::Math::XYZTVector momentum;
  std::string process;

  WCSimAmBeParticle()
      : track_id(-1),
        parent_id(-1),
        pdg(0),
        position(),
        momentum(),
        process("") {}
};

struct WCSimAmBeEvent {
  int rank;
  int thread_id;
  int event_id;
  std::vector<WCSimAmBeParticle> particles;

  WCSimAmBeEvent() : rank(-1), thread_id(-1), event_id(-1), particles() {}

  void Clear() {
    rank = -1;
    thread_id = -1;
    event_id = -1;
    particles.clear();
  }
};

class WCSimAmBePrimaryReader {
 public:
  WCSimAmBePrimaryReader();
  virtual ~WCSimAmBePrimaryReader();

  bool Open(const std::string& filename,
            const std::string& treeName = "EmergingParticles");
  void Close();

  bool IsOpen() const;
  bool HasTree() const;

  bool NextEvent(WCSimAmBeEvent& event);
  void Reset();

  long long GetEntries() const;
  long long GetCurrentEntry() const;
  std::string GetFileName() const;
  std::string GetTreeName() const;

 private:
  bool SetupBranches();
  void ClearBranchPointers();
  bool ValidateCurrentEntry() const;

 private:
  TFile* fFile;
  TTree* fTree;

  std::string fFileName;
  std::string fTreeName;
  long long fCurrentEntry;
  long long fNEntries;

  // Event-level branches
  int fbranchEmergingRank;
  int fbranchEmergingThreadId;
  int fbranchEmergingEventId;

  // Particle-level branches
  std::vector<int>* fbranchEmergingId;
  std::vector<int>* fbranchEmergingParentId;
  std::vector<int>* fbranchEmergingPDG;
  std::vector<ROOT::Math::XYZTVector>* fbranchEmergingPos;
  std::vector<ROOT::Math::XYZTVector>* fbranchEmergingP;
  std::vector<std::string>* fbranchEmergingProcess;
};

#endif