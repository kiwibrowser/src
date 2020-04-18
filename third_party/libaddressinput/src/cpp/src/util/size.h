// Copyright (C) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef I18N_ADDRESSINPUT_UTIL_SIZE_H_
#define I18N_ADDRESSINPUT_UTIL_SIZE_H_

#include <cstddef>
#include <iterator>

namespace i18n {
namespace addressinput {

// If the C++17 std::size is provided by the standard library then the fallback
// C++11 implementation must not be used for that would make it ambiguous which
// one of the two implementations a call should be resolved to.
//
// Although libaddressinput.gyp explicitly sets -std=c++11 it's possible that
// this is overridden at build time to use a newer version of the standard.
//
// It's also possible that C++17 std::size is defined even when building for an
// older version of the standard, which is done in the Microsoft implementation
// of the C++ Standard Library:
//
// https://docs.microsoft.com/en-us/cpp/visual-cpp-language-conformance

#if __cpp_lib_nonmember_container_access >= 201411 || \
    (_LIBCPP_VERSION >= 1101 && _LIBCPP_STD_VER > 14) || \
    (!defined(_LIBCPP_STD_VER) && _MSC_VER >= 1900)

using std::size;

#else

// A C++11 implementation of the C++17 std::size, copied from the standard:
// https://isocpp.org/files/papers/n4280.pdf

template <class T, size_t N>
constexpr size_t size(const T (&array)[N]) {
  return N;
}

#endif

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_UTIL_SIZE_H_
