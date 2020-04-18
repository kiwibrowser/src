#include <cassert>
#include <exception>

void throw_exception() {
  throw 1;
}

int main() {
  std::set_unexpected(throw_exception);
  try {
    std::unexpected(); // it is OK to throw exception from std::unexpected()
    assert(false);
  } catch (int) {
    assert(true);
  } catch (...) {
    assert(false);
  }
  return 0;
}
