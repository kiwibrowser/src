#include <cassert>
#include <cstdlib>
#include <exception>

void expected_terminate() {
  exit(0);
}

void throw_exception() {
  // do nothing and return, so that std::terminate() can be invoked.
}

int main() {
  std::set_terminate(expected_terminate);
  std::set_unexpected(throw_exception);
  try {
    std::unexpected();
    assert(false);
  } catch (...) {
    assert(false);
  }
  assert(false);
  return 1;
}
