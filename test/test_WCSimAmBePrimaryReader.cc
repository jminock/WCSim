#include <iostream>
#include <string>

#include "WCSimAmBePrimaryReader.hh"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " input.root [tree_name]" << std::endl;
    return 1;
  }

  std::string inputFile = argv[1];
  std::string treeName = "EmergingParticles";
  if (argc >= 3) {
    treeName = argv[2];
  }

  WCSimAmBePrimaryReader reader;

  if (!reader.Open(inputFile, treeName)) {
    std::cerr << "ERROR: failed to open input file/tree" << std::endl;
    return 2;
  }

  std::cout << "Opened file: " << reader.GetFileName() << std::endl;
  std::cout << "Tree name:   " << reader.GetTreeName() << std::endl;
  std::cout << "Entries:     " << reader.GetEntries() << std::endl;

  WCSimAmBeEvent event;
  int nToPrint = 3;
  int nRead = 0;

  while (nRead < nToPrint && reader.NextEvent(event)) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Reader entry: " << reader.GetCurrentEntry() - 1 << std::endl;
    std::cout << "Rank:         " << event.rank << std::endl;
    std::cout << "Thread:       " << event.thread_id << std::endl;
    std::cout << "EventId:      " << event.event_id << std::endl;
    std::cout << "N particles:  " << event.particles.size() << std::endl;

    for (std::size_t i = 0; i < event.particles.size(); ++i) {
      const auto& p = event.particles[i];

      std::cout << "  Particle " << i << std::endl;
      std::cout << "    track_id:  " << p.track_id << std::endl;
      std::cout << "    parent_id: " << p.parent_id << std::endl;
      std::cout << "    pdg:       " << p.pdg << std::endl;
      std::cout << "    process:   " << p.process << std::endl;

      std::cout << "    position:  ("
                << p.position.X() << ", "
                << p.position.Y() << ", "
                << p.position.Z() << ", "
                << p.position.T() << ")" << std::endl;

      std::cout << "    momentum:  ("
                << p.momentum.X() << ", "
                << p.momentum.Y() << ", "
                << p.momentum.Z() << ", "
                << p.momentum.T() << ")" << std::endl;
    }

    ++nRead;
  }

  std::cout << "\nDone." << std::endl;
  return 0;
}