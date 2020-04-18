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

#ifndef I18N_ADDRESSINPUT_UTIL_JSON_H_
#define I18N_ADDRESSINPUT_UTIL_JSON_H_

#include <memory>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

// Parses a JSON dictionary of strings. Sample usage:
//    Json json;
//    if (json.ParseObject("{'key1':'value1', 'key2':'value2'}") &&
//        json.HasStringKey("key1")) {
//      Process(json.GetStringValueForKey("key1"));
//    }
class Json {
 public:
  Json(const Json&) = delete;
  Json& operator=(const Json&) = delete;

  Json();
  ~Json();

  // Parses the |json| string and returns true if |json| is valid and it is an
  // object.
  bool ParseObject(const std::string& json);

  // Returns the list of sub dictionaries. The JSON object must be parsed
  // successfully in ParseObject() before invoking this method. The caller does
  // not own the result.
  const std::vector<const Json*>& GetSubDictionaries() const;

  // Returns true if the parsed JSON contains a string value for |key|. Sets
  // |value| to the string value of the |key|. The JSON object must be parsed
  // successfully in ParseObject() before invoking this method. The |value|
  // parameter should not be nullptr.
  bool GetStringValueForKey(const std::string& key, std::string* value) const;

 private:
  class JsonImpl;
  friend class JsonImpl;

  // Constructor to be called by JsonImpl.
  explicit Json(JsonImpl* impl);

  std::unique_ptr<JsonImpl> impl_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_UTIL_JSON_H_
