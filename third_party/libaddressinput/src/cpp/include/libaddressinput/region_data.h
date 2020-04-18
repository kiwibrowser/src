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

#ifndef I18N_ADDRESSINPUT_REGION_DATA_H_
#define I18N_ADDRESSINPUT_REGION_DATA_H_

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

// The key and name of a region that can be used as one of the items in a
// dropdown UI element.
class RegionData {
 public:
  RegionData(const RegionData&) = delete;
  RegionData& operator=(const RegionData&) = delete;

  // Creates a top-level RegionData object. Use AddSubRegion() to add data below
  // it. Does not make a copy of data in |region_code|.
  explicit RegionData(const std::string& region_code);
  ~RegionData();

  // Creates a sub-level RegionData object, with this object as its parent and
  // owner. Does not make copies of the data in |key| or |name|.
  RegionData* AddSubRegion(const std::string& key, const std::string& name);

  const std::string& key() const { return key_; }

  const std::string& name() const { return name_; }

  bool has_parent() const { return parent_ != nullptr; }

  // Should be called only if has_parent() returns true.
  const RegionData& parent() const {
    assert(parent_ != nullptr);
    return *parent_;
  }

  // The caller does not own the results. The results are not nullptr and have a
  // parent.
  const std::vector<const RegionData*>& sub_regions() const {
    return sub_regions_;
  }

 private:
  // Private constructor used by AddSubRegion().
  RegionData(const std::string& key,
             const std::string& name,
             RegionData* parent);

  const std::string& key_;
  const std::string& name_;
  const RegionData* const parent_;  // Not owned.
  std::vector<const RegionData*> sub_regions_;  // Owned.
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_REGION_DATA_H_
