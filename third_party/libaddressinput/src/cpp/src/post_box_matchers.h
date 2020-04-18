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
//
// Post office box regular expressions.

#ifndef I18N_ADDRESSINPUT_POST_BOX_MATCHERS_H_
#define I18N_ADDRESSINPUT_POST_BOX_MATCHERS_H_

#include <vector>

namespace i18n {
namespace addressinput {

class Rule;
struct RE2ptr;

class PostBoxMatchers {
 public:
  // Returns pointers to RE2 regular expression objects to test address lines
  // for those languages that are relevant for |country_rule|.
  static std::vector<const RE2ptr*> GetMatchers(const Rule& country_rule);

  PostBoxMatchers(const PostBoxMatchers&) = delete;
  PostBoxMatchers& operator=(const PostBoxMatchers&) = delete;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_POST_BOX_MATCHERS_H_
