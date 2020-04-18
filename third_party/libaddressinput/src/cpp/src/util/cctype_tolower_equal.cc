// Copyright (C) 2014 Google Inc.
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

#include "cctype_tolower_equal.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>

namespace i18n {
namespace addressinput {

namespace {

struct EqualToTolowerChar
    : public std::binary_function<std::string::value_type,
                                  std::string::value_type, bool> {
  result_type operator()(first_argument_type a, second_argument_type b) const {
    return std::tolower(a) == std::tolower(b);
  }
};

}  // namespace

EqualToTolowerString::result_type EqualToTolowerString::operator()(
    const first_argument_type& a, const second_argument_type& b) const {
  return a.size() == b.size() &&
         std::equal(a.begin(), a.end(), b.begin(), EqualToTolowerChar());
}

}  // namespace addressinput
}  // namespace i18n
