// Copyright (C) 2013 Google Inc.
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

#ifndef I18N_ADDRESSINPUT_REGION_DATA_CONSTANTS_H_
#define I18N_ADDRESSINPUT_REGION_DATA_CONSTANTS_H_

#include <cstddef>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

class RegionDataConstants {
 public:
  static bool IsSupported(const std::string& region_code);
  static const std::vector<std::string>& GetRegionCodes();
  static const std::string& GetRegionData(const std::string& region_code);
  static const std::string& GetDefaultRegionData();
  static size_t GetMaxLookupKeyDepth(const std::string& region_code);

  RegionDataConstants(const RegionDataConstants&) = delete;
  RegionDataConstants& operator=(const RegionDataConstants&) = delete;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_REGION_DATA_CONSTANTS_H_
