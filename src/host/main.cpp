#include <iostream>
#include <filesystem>
#include <fstream>

#include "host.hpp"

using namespace rmp;

int main () {
  Host host;

  if (host.getStatus() == Status::CriticalError) {
    host.log("Host error, status returned is critical error\n");
    return 1;
  }

  int rcode = host.run();
  if (rcode != 0) {
    host.log("Error running host %i\n", rcode);
  }

  return rcode;
}