#include <iostream>
#include <filesystem>
#include <fstream>

#include "host.hpp"

using namespace rmp;

void pause() {
  std::cout << "Press enter to continue...";
  char c;
  std::cin >> c;
}

int main () {
  Host host("module.json");

  if (host.getStatus() == Status::CriticalError) {
    host.log("Host error, status returned is critical error\n");
    pause();
    return 1;
  }

  int rcode = host.run();
  if (rcode != 0) {
    host.log("Error running host %i\n", rcode);
  }

  pause();
  return rcode;
}