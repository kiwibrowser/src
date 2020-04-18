// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef LIBLOUIS_NACL_TRANSLATION_PARAMS_H_
#define LIBLOUIS_NACL_TRANSLATION_PARAMS_H_

#include <string>
#include <vector>

namespace liblouis_nacl {

// Struct containing the parameters of translation.
struct TranslationParams {
 public:
  std::string table_names;
  std::string text;
  int cursor_position;
  std::vector<unsigned char> form_type_map;
};

}

#endif  // LIBLOUIS_NACL_TRANSLATION_PARAMS_H_
