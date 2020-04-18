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

#include "post_box_matchers.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <re2/re2.h>

#include "language.h"
#include "rule.h"
#include "util/re2ptr.h"

namespace i18n {
namespace addressinput {

namespace {

std::map<std::string, const RE2ptr*> InitMatchers() {
  static const struct {
    const char* const language;
    const RE2ptr ptr;
  } kMatchers[] = {
      {"ar", new RE2(u8R"(صندوق بريد|ص[-. ]ب)")},
      {"cs", new RE2(u8R"((?i)p\.? ?p\.? \d)")},
      {"da", new RE2(u8R"((?i)Postboks)")},
      {"de", new RE2(u8R"((?i)Postfach)")},
      {"el", new RE2(u8R"((?i)T\.? ?Θ\.? \d{2})")},
      {"en", new RE2(u8R"(Private Bag|Post(?:al)? Box)")},
      {"es", new RE2(u8R"((?i)(?:Apartado|Casillas) de correos?)")},
      {"fi", new RE2(u8R"((?i)Postilokero|P\.?L\.? \d)")},
      {"hr", new RE2(u8R"((?i)p\.? ?p\.? \d)")},
      {"hu", new RE2(u8R"((?i)Postafi(?:[oó]|ó)k|Pf\.? \d)")},
      {"fr", new RE2(u8R"((?i)Bo(?:[iî]|î)te Postale|BP \d|CEDEX \d)")},
      {"ja", new RE2(u8R"(私書箱\d{1,5}号)")},
      {"nl", new RE2(u8R"((?i)Postbus)")},
      {"no", new RE2(u8R"((?i)Postboks)")},
      {"pl", new RE2(u8R"((?i)Skr(?:\.?|ytka) poczt(?:\.?|owa))")},
      {"pt", new RE2(u8R"((?i)Apartado)")},
      {"ru", new RE2(u8R"((?i)абонентский ящик|[аa]"я (?:(?:№|#|N) ?)?\d)")},
      {"sv", new RE2(u8R"((?i)Box \d)")},
      {"zh", new RE2(u8R"(郵政信箱.{1,5}號|郵局第.{1,10}號信箱)")},
      {"und", new RE2(u8R"(P\.? ?O\.? Box)")},
  };

  std::map<std::string, const RE2ptr*> matchers;

  for (size_t i = 0; i < sizeof kMatchers / sizeof *kMatchers; ++i) {
    matchers.insert(std::make_pair(kMatchers[i].language, &kMatchers[i].ptr));
  }

  return matchers;
}

}  // namespace

// static
std::vector<const RE2ptr*> PostBoxMatchers::GetMatchers(
    const Rule& country_rule) {
  static const std::map<std::string, const RE2ptr*> kMatchers(InitMatchers());

  // Always add any expressions defined for "und" (English-like defaults).
  std::vector<std::string> languages(1, "und");
  for (std::vector<std::string>::const_iterator
       it = country_rule.GetLanguages().begin();
       it != country_rule.GetLanguages().end(); ++it) {
    Language language(*it);
    languages.push_back(language.base);
  }

  std::vector<const RE2ptr*> result;

  for (std::vector<std::string>::const_iterator
       it = languages.begin();
       it != languages.end(); ++it) {
    std::map<std::string, const RE2ptr*>::const_iterator
        jt = kMatchers.find(*it);
    if (jt != kMatchers.end()) {
      result.push_back(jt->second);
    }
  }

  return result;
}

}  // namespace addressinput
}  // namespace i18n
