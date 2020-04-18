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

#ifndef I18N_ADDRESSINPUT_LANGUAGE_H_
#define I18N_ADDRESSINPUT_LANGUAGE_H_

#include <string>

namespace i18n {
namespace addressinput {

class Rule;

// Helper for working with a BCP 47 language tag.
// http://tools.ietf.org/html/bcp47
struct Language {
  explicit Language(const std::string& language_tag);
  ~Language();

  // The language tag (with '_' replaced with '-'), for example "zh-Latn-CN".
  std::string tag;

  // The base language, for example "zh". Always lowercase.
  std::string base;

  // True if the language tag explicitly has a Latin script. For example, this
  // is true for "zh-Latn", but false for "zh". Only the second and third subtag
  // positions are supported for script.
  bool has_latin_script;
};

Language ChooseBestAddressLanguage(const Rule& address_region_rule,
                                   const Language& ui_language);

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_LANGUAGE_H_
