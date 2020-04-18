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

#ifndef I18N_ADDRESSINPUT_UTIL_CCTYPE_TOLOWER_EQUAL_H_
#define I18N_ADDRESSINPUT_UTIL_CCTYPE_TOLOWER_EQUAL_H_

#include <functional>
#include <string>

namespace i18n {
namespace addressinput {

// Performs case insensitive comparison of |a| and |b| by calling std::tolower()
// from <cctype>.
struct EqualToTolowerString
    : public std::binary_function<std::string, std::string, bool> {
  result_type operator()(const first_argument_type& a,
                         const second_argument_type& b) const;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_UTIL_CCTYPE_TOLOWER_EQUAL_H_
