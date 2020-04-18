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
// ValidatingUtil wraps data with checksum and timestamp. Format:
//
//    timestamp=<timestamp>
//    checksum=<checksum>
//    <data>
//
// The timestamp is the time_t that was returned from time() function. The
// timestamp does not need to be portable because it is written and read only by
// ValidatingUtil. The value is somewhat human-readable: it is the number of
// seconds since the epoch.
//
// The checksum is the 32-character hexadecimal MD5 checksum of <data>. It is
// meant to protect from random file changes on disk.

#include "validating_util.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#include "util/md5.h"

namespace i18n {
namespace addressinput {

namespace {

const char kTimestampPrefix[] = "timestamp=";
const size_t kTimestampPrefixLength = sizeof kTimestampPrefix - 1;

const char kChecksumPrefix[] = "checksum=";
const size_t kChecksumPrefixLength = sizeof kChecksumPrefix - 1;

const char kSeparator = '\n';

// Places the header value into |header_value| parameter and erases the header
// from |data|. Returns |true| if the header format is valid.
bool UnwrapHeader(const char* header_prefix,
                  size_t header_prefix_length,
                  std::string* data,
                  std::string* header_value) {
  assert(header_prefix != nullptr);
  assert(data != nullptr);
  assert(header_value != nullptr);

  if (data->compare(
          0, header_prefix_length, header_prefix, header_prefix_length) != 0) {
    return false;
  }

  std::string::size_type separator_position =
      data->find(kSeparator, header_prefix_length);
  if (separator_position == std::string::npos) {
    return false;
  }

  header_value->assign(
      *data, header_prefix_length, separator_position - header_prefix_length);
  data->erase(0, separator_position + 1);

  return true;
}

}  // namespace

// static
void ValidatingUtil::Wrap(time_t timestamp, std::string* data) {
  assert(data != nullptr);
  char timestamp_string[2 + 3 * sizeof timestamp];
  int size =
      std::sprintf(timestamp_string, "%ld", static_cast<long>(timestamp));
  assert(size > 0);
  assert(size < sizeof timestamp_string);
  (void)size;

  std::string header;
  header.append(kTimestampPrefix, kTimestampPrefixLength);
  header.append(timestamp_string);
  header.push_back(kSeparator);

  header.append(kChecksumPrefix, kChecksumPrefixLength);
  header.append(MD5String(*data));
  header.push_back(kSeparator);

  data->reserve(header.size() + data->size());
  data->insert(0, header);
}

// static
bool ValidatingUtil::UnwrapTimestamp(std::string* data, time_t now) {
  assert(data != nullptr);
  if (now < 0) {
    return false;
  }

  std::string timestamp_string;
  if (!UnwrapHeader(
          kTimestampPrefix, kTimestampPrefixLength, data, &timestamp_string)) {
    return false;
  }

  time_t timestamp = atol(timestamp_string.c_str());
  if (timestamp < 0) {
    return false;
  }

  // One month contains:
  //    30 days *
  //    24 hours per day *
  //    60 minutes per hour *
  //    60 seconds per minute.
  static const double kOneMonthInSeconds = 30.0 * 24.0 * 60.0 * 60.0;
  double age_in_seconds = difftime(now, timestamp);
  return !(age_in_seconds < 0.0) && age_in_seconds < kOneMonthInSeconds;
}

// static
bool ValidatingUtil::UnwrapChecksum(std::string* data) {
  assert(data != nullptr);
  std::string checksum;
  if (!UnwrapHeader(kChecksumPrefix, kChecksumPrefixLength, data, &checksum)) {
    return false;
  }
  return checksum == MD5String(*data);
}

}  // namespace addressinput
}  // namespace i18n
