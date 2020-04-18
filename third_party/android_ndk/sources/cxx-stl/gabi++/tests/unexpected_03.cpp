#include <cassert>
#include <cstdlib>
#include <exception>

void expected_terminate() {
  exit(0);
}

int main() {
  std::set_terminate(expected_terminate);
  try {
    std::unexpected();
    assert(false);
  } catch (...) {
    assert(false);
  }
  assert(false);
  return 1;
}
