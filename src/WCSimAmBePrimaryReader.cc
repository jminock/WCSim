#include "WCSimAmBePrimaryReader.hh"

#include <iostream>

// ROOT
#include "TFile.h"
#include "TTree.h"

WCSimAmBePrimaryReader::WCSimAmBePrimaryReader()
    : fFile(0),
      fTree(0),
      fFileName(""),
      fTreeName("EmergingParticles"),
      fCurrentEntry(0),
      fNEntries(0),
      fbranchEmergingRank(-1),
      fbranchEmergingThreadId(-1),
      fbranchEmergingEventId(-1),
      fbranchEmergingId(0),
      fbranchEmergingParentId(0),
      fbranchEmergingPDG(0),
      fbranchEmergingPos(0),
      fbranchEmergingP(0),
      fbranchEmergingProcess(0) {}

WCSimAmBePrimaryReader::~WCSimAmBePrimaryReader() {
  Close();
}

bool WCSimAmBePrimaryReader::Open(const std::string& filename,
                                  const std::string& treeName) {
  Close();

  fFileName = filename;
  fTreeName = treeName;
  fCurrentEntry = 0;
  fNEntries = 0;

  fFile = TFile::Open(filename.c_str(), "READ");
  if (!fFile || fFile->IsZombie()) {
    std::cerr << "WCSimAmBePrimaryReader::Open(): failed to open file: "
              << filename << std::endl;
    Close();
    return false;
  }

  fTree = dynamic_cast<TTree*>(fFile->Get(treeName.c_str()));
  if (!fTree) {
    std::cerr << "WCSimAmBePrimaryReader::Open(): failed to find tree '"
              << treeName << "' in file: " << filename << std::endl;
    Close();
    return false;
  }

  if (!SetupBranches()) {
    std::cerr << "WCSimAmBePrimaryReader::Open(): failed to set branch addresses"
              << std::endl;
    Close();
    return false;
  }

  fNEntries = static_cast<long long>(fTree->GetEntries());

  std::cout << "WCSimAmBePrimaryReader: opened file " << fFileName
            << " with tree " << fTreeName
            << " containing " << fNEntries << " entries." << std::endl;

  return true;
}

void WCSimAmBePrimaryReader::Close() {
  ClearBranchPointers();

  if (fFile) {
    fFile->Close();
    delete fFile;
    fFile = 0;
  }

  fTree = 0;
  fFileName = "";
  fTreeName = "EmergingParticles";
  fCurrentEntry = 0;
  fNEntries = 0;
  fbranchEmergingRank = -1;
  fbranchEmergingThreadId = -1;
  fbranchEmergingEventId = -1;
}

bool WCSimAmBePrimaryReader::IsOpen() const {
  return (fFile != 0);
}

bool WCSimAmBePrimaryReader::HasTree() const {
  return (fTree != 0);
}

void WCSimAmBePrimaryReader::Reset() {
  fCurrentEntry = 0;
}

long long WCSimAmBePrimaryReader::GetEntries() const {
  return fNEntries;
}

long long WCSimAmBePrimaryReader::GetCurrentEntry() const {
  return fCurrentEntry;
}

std::string WCSimAmBePrimaryReader::GetFileName() const {
  return fFileName;
}

std::string WCSimAmBePrimaryReader::GetTreeName() const {
  return fTreeName;
}

bool WCSimAmBePrimaryReader::SetupBranches() {
  if (!fTree) return false;

  ClearBranchPointers();

  fTree->SetBranchAddress("Rank", &fbranchEmergingRank);
  fTree->SetBranchAddress("Thread", &fbranchEmergingThreadId);
  fTree->SetBranchAddress("EventId", &fbranchEmergingEventId);
  fTree->SetBranchAddress("TrackId", &fbranchEmergingId);
  fTree->SetBranchAddress("ParentId", &fbranchEmergingParentId);
  fTree->SetBranchAddress("PDG", &fbranchEmergingPDG);
  fTree->SetBranchAddress("Vertex", &fbranchEmergingPos);
  fTree->SetBranchAddress("Momentum", &fbranchEmergingP);
  fTree->SetBranchAddress("Process", &fbranchEmergingProcess);

  return true;
}

void WCSimAmBePrimaryReader::ClearBranchPointers() {
  fbranchEmergingId = 0;
  fbranchEmergingParentId = 0;
  fbranchEmergingPDG = 0;
  fbranchEmergingPos = 0;
  fbranchEmergingP = 0;
  fbranchEmergingProcess = 0;
}

bool WCSimAmBePrimaryReader::ValidateCurrentEntry() const {
  if (!fbranchEmergingId ||
      !fbranchEmergingParentId ||
      !fbranchEmergingPDG ||
      !fbranchEmergingPos ||
      !fbranchEmergingP ||
      !fbranchEmergingProcess) {
    std::cerr << "WCSimAmBePrimaryReader::ValidateCurrentEntry(): "
              << "one or more branch pointers are null." << std::endl;
    return false;
  }

  const std::size_t n = fbranchEmergingPDG->size();

  if (fbranchEmergingId->size() != n ||
      fbranchEmergingParentId->size() != n ||
      fbranchEmergingPos->size() != n ||
      fbranchEmergingP->size() != n ||
      fbranchEmergingProcess->size() != n) {
    std::cerr << "WCSimAmBePrimaryReader::ValidateCurrentEntry(): "
              << "inconsistent vector sizes in EventId "
              << fbranchEmergingEventId << std::endl;
    return false;
  }

  return true;
}

bool WCSimAmBePrimaryReader::NextEvent(WCSimAmBeEvent& event) {
  event.Clear();

  if (!fTree) {
    std::cerr << "WCSimAmBePrimaryReader::NextEvent(): no tree is loaded."
              << std::endl;
    return false;
  }

  if (fCurrentEntry >= fNEntries) {
    return false;
  }

  Long64_t bytesRead = fTree->GetEntry(fCurrentEntry);
  if (bytesRead <= 0) {
    std::cerr << "WCSimAmBePrimaryReader::NextEvent(): failed to read entry "
              << fCurrentEntry << std::endl;
    return false;
  }

  if (!ValidateCurrentEntry()) {
    return false;
  }

  event.rank = fbranchEmergingRank;
  event.thread_id = fbranchEmergingThreadId;
  event.event_id = fbranchEmergingEventId;

  event.particles.reserve(fbranchEmergingPDG->size());

  for (std::size_t i = 0; i < fbranchEmergingPDG->size(); ++i) {
    WCSimAmBeParticle particle;
    particle.track_id = fbranchEmergingId->at(i);
    particle.parent_id = fbranchEmergingParentId->at(i);
    particle.pdg = fbranchEmergingPDG->at(i);
    particle.position = fbranchEmergingPos->at(i);
    particle.momentum = fbranchEmergingP->at(i);
    particle.process = fbranchEmergingProcess->at(i);

    event.particles.push_back(particle);
  }

  ++fCurrentEntry;
  return true;
}