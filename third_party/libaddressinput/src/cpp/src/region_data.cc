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

#include <libaddressinput/region_data.h>

#include <cstddef>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

RegionData::RegionData(const std::string& region_code)
    : key_(region_code),
      name_(region_code),
      parent_(nullptr),
      sub_regions_() {}

RegionData::~RegionData() {
  for (std::vector<const RegionData*>::const_iterator it = sub_regions_.begin();
       it != sub_regions_.end(); ++it) {
    delete *it;
  }
}

RegionData* RegionData::AddSubRegion(const std::string& key,
                                     const std::string& name) {
  RegionData* sub_region = new RegionData(key, name, this);
  sub_regions_.push_back(sub_region);
  return sub_region;
}

RegionData::RegionData(const std::string& key,
                       const std::string& name,
                       RegionData* parent)
    : key_(key), name_(name), parent_(parent), sub_regions_() {}

}  // namespace addressinput
}  // namespace i18n
