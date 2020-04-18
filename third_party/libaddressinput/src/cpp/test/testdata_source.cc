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

#include "testdata_source.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>

namespace i18n {
namespace addressinput {

const char kDataFileName[] = TEST_DATA_DIR "/countryinfo.txt";

namespace {

// For historical reasons, normal and aggregated data is here stored in the
// same data structure, differentiated by giving each key a prefix. It does
// seem like a good idea to refactor this.
const char kNormalPrefix = '-';
const char kAggregatePrefix = '+';

// Each data key begins with this string. Example of a data key:
//     data/CH/AG
const char kDataKeyPrefix[] = "data/";

// The number of characters in the data key prefix.
const size_t kDataKeyPrefixLength = sizeof kDataKeyPrefix - 1;

// The number of characters in a CLDR region code, e.g. 'CH'.
const size_t kCldrRegionCodeLength = 2;

// The number of characters in an aggregate data key, e.g. 'data/CH'.
const size_t kAggregateDataKeyLength =
    kDataKeyPrefixLength + kCldrRegionCodeLength;

std::map<std::string, std::string> InitData(const std::string& src_path) {
  std::map<std::string, std::string> data;
  std::ifstream file(src_path);
  if (!file.is_open()) {
    std::cerr << "Error opening \"" << src_path << "\"." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  const std::string normal_prefix(1, kNormalPrefix);
  const std::string aggregate_prefix(1, kAggregatePrefix);

  std::string key;
  std::string value;

  std::map<std::string, std::string>::iterator last_data_it = data.end();
  std::map<std::string, std::string>::iterator aggregate_data_it = data.end();

  while (file.good()) {
    // Example line from countryinfo.txt:
    //     data/CH/AG={"name": "Aargau"}
    // Example key:
    //     data/CH/AG
    std::getline(file, key, '=');

    if (!key.empty()) {
      // Example value:
      //     {"name": "Aargau"}
      std::getline(file, value, '\n');

      // For example, map '-data/CH/AG' to '{"name": "Aargau"}'.
      last_data_it = data.insert(
          last_data_it,
          std::make_pair(normal_prefix + key, value));

      // Aggregate keys that begin with 'data/'. We don't aggregate keys that
      // begin with 'example/' because example data is not used anywhere.
      if (key.compare(0,
                      kDataKeyPrefixLength,
                      kDataKeyPrefix,
                      kDataKeyPrefixLength) == 0) {
        // If aggregate_data_it and key have the same prefix, e.g. "data/ZZ".
        if (aggregate_data_it != data.end() &&
            key.compare(0,
                        kAggregateDataKeyLength,
                        aggregate_data_it->first,
                        sizeof kAggregatePrefix,
                        kAggregateDataKeyLength) == 0) {
          // Append more data to the aggregate string, for example:
          //     , "data/CH/AG": {"name": "Aargau"}
          aggregate_data_it->second.append(", \"" + key + "\": " + value);
        } else {
          // The countryinfo.txt file must be sorted so that subkey data
          // follows directly after its parent key data.
          assert(key.size() == kAggregateDataKeyLength);

          // Make the aggregate data strings valid. For example, this incomplete
          // JSON data:
          //     {"data/CH/AG": {"name": "Aargau"},
          //      "data/CH": {"name": "SWITZERLAND"}
          //
          // becomes valid JSON data like so:
          //
          //     {"data/CH/AG": {"name": "Aargau"},
          //      "data/CH": {"name": "SWITZERLAND"}}
          if (aggregate_data_it != data.end()) {
            aggregate_data_it->second.push_back('}');
          }

          // Example aggregate prefixed key:
          //     +data/CH
          const std::string& aggregate_key =
              aggregate_prefix + key.substr(0, kAggregateDataKeyLength);

          // Begin a new aggregate string, for example:
          //     {"data/CH/AG": {"name": "Aargau"}
          aggregate_data_it = data.insert(
              aggregate_data_it,
              std::make_pair(aggregate_key, "{\"" + key + "\": " + value));
        }
      }
    }
  }

  file.close();
  return data;
}

const std::map<std::string, std::string>& GetData(const std::string& src_path) {
  static const std::map<std::string, std::string> kData(InitData(src_path));
  return kData;
}

}  // namespace

TestdataSource::TestdataSource(bool aggregate, const std::string& src_path)
    : aggregate_(aggregate), src_path_(src_path) {}

TestdataSource::TestdataSource(bool aggregate)
    : aggregate_(aggregate), src_path_(kDataFileName) {}

TestdataSource::~TestdataSource() {}

void TestdataSource::Get(const std::string& key,
                         const Callback& data_ready) const {
  std::string prefixed_key(1, aggregate_ ? kAggregatePrefix : kNormalPrefix);
  prefixed_key += key;
  std::map<std::string, std::string>::const_iterator data_it =
      GetData(src_path_).find(prefixed_key);
  bool success = data_it != GetData(src_path_).end();
  std::string* data = nullptr;
  if (success) {
    data = new std::string(data_it->second);
  } else {
    // URLs that start with "https://chromium-i18n.appspot.com/ssl-address/" or
    // "https://chromium-i18n.appspot.com/ssl-aggregate-address/" prefix, but do
    // not have associated data will always return "{}" with status code 200.
    // TestdataSource imitates this behavior.
    success = true;
    data = new std::string("{}");
  }
  data_ready(success, key, data);
}

}  // namespace addressinput
}  // namespace i18n
