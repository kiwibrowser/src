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
//
// An object to wrap data with a checksum and a timestamp. These fields are used
// to verify that the data is not stale or corrupted. Staleness threshold is 1
// month.

#ifndef I18N_ADDRESSINPUT_VALIDATING_UTIL_H_
#define I18N_ADDRESSINPUT_VALIDATING_UTIL_H_

#include <ctime>
#include <string>

namespace i18n {
namespace addressinput {

// Wraps data with a checksum and a timestamp. Sample usage:
//    std::string data = ...
//    ValidatingUtil::Wrap(time(nullptr), &data);
//    Process(data);
//
//    std::string unwrapped = wrapped;
//    if (ValidatingUtil::UnwrapTimestamp(&unwrapped, time(nullptr)) &&
//        ValidatingUtil::UnwrapChecksum(&unwrapped)) {
//      Process(unwrapped);
//    }
class ValidatingUtil {
 public:
  // Adds checksum and given |timestamp| to |data|.
  static void Wrap(time_t timestamp, std::string* data);

  // Strips out the timestamp from |data|. Returns |true| if the timestamp is
  // present, formatted correctly, valid, and recent with respect to |now|.
  static bool UnwrapTimestamp(std::string* data, time_t now);

  // Strips out the checksum from |data|. Returns |true| if the checksum is
  // present, formatted correctly, and valid for this data.
  static bool UnwrapChecksum(std::string* data);

  ValidatingUtil(const ValidatingUtil&) = delete;
  ValidatingUtil& operator=(const ValidatingUtil&) = delete;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_VALIDATING_UTIL_H_
