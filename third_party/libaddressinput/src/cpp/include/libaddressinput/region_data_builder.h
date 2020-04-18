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

#ifndef I18N_ADDRESSINPUT_REGION_DATA_BUILDER_H_
#define I18N_ADDRESSINPUT_REGION_DATA_BUILDER_H_

#include <map>
#include <string>

namespace i18n {
namespace addressinput {

class PreloadSupplier;
class RegionData;

class RegionDataBuilder {
 public:
  RegionDataBuilder(const RegionDataBuilder&) = delete;
  RegionDataBuilder& operator=(const RegionDataBuilder&) = delete;

  // Does not take ownership of |supplier|, which should not be nullptr.
  explicit RegionDataBuilder(PreloadSupplier* supplier);
  ~RegionDataBuilder();

  // Returns a tree of administrative subdivisions for the |region_code|.
  // Examples:
  //   US with en-US UI language.
  //   |______________________
  //   |           |          |
  //   v           v          v
  // AL:Alabama  AK:Alaska  AS:American Samoa  ...
  //
  //   KR with ko-Latn UI language.
  //   |______________________________________
  //       |               |                  |
  //       v               v                  v
  // 강원도:Gangwon  경기도:Gyeonggi  경상남도:Gyeongnam  ...
  //
  //   KR with ko-KR UI language.
  //   |_______________________________
  //       |            |              |
  //       v            v              v
  // 강원도:강원  경기도:경기  경상남도:경남  ...
  //
  // The BCP 47 |ui_language_tag| is used to choose the best supported language
  // tag for this region (assigned to |best_region_tree_language_tag|). For
  // example, Canada has both English and French names for its administrative
  // subdivisions. If the UI language is French, then the French names are used.
  // The |best_region_tree_language_tag| value may be an empty string.
  //
  // Should be called only if supplier->IsLoaded(region_code) returns true. The
  // |best_region_tree_language_tag| parameter should not be nullptr.
  const RegionData& Build(const std::string& region_code,
                          const std::string& ui_language_tag,
                          std::string* best_region_tree_language_tag);

 private:
  typedef std::map<std::string, const RegionData*> LanguageRegionMap;
  typedef std::map<std::string, LanguageRegionMap*> RegionCodeDataMap;

  PreloadSupplier* const supplier_;  // Not owned.
  RegionCodeDataMap cache_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_REGION_DATA_BUILDER_H_
