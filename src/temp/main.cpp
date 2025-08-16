#include "app.hpp"

int main() {
  logInit();

  int code = 0;
  {
    App app("Rampage", 60);
    if (app.getStatus() == Status::CriticalError)
      return -1;

    code = app.run();
  }

  return code;
}
