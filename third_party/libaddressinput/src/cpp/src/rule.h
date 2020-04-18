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
// An object to store address metadata, describing the addressing rules for
// regions and sub-regions. The address metadata format is documented here:
//
// https://github.com/googlei18n/libaddressinput/wiki/AddressValidationMetadata

#ifndef I18N_ADDRESSINPUT_RULE_H_
#define I18N_ADDRESSINPUT_RULE_H_

#include <libaddressinput/address_field.h>

#include <memory>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

class FormatElement;
class Json;
struct RE2ptr;

// Stores address metadata addressing rules, to be used for determining the
// layout of an address input widget or for address validation. Sample usage:
//    Rule rule;
//    if (rule.ParseSerializedRule("{\"fmt\": \"%A%n%C%S %Z\"}")) {
//      Process(rule.GetFormat());
//    }
class Rule {
 public:
  Rule(const Rule&) = delete;
  Rule& operator=(const Rule&) = delete;

  Rule();
  ~Rule();

  // Returns the default rule at a country level. If a country does not specify
  // address format, for example, then the format from this rule should be used
  // instead.
  static const Rule& GetDefault();

  // Copies all data from |rule|.
  void CopyFrom(const Rule& rule);

  // Parses |serialized_rule|. Returns |true| if the |serialized_rule| has valid
  // format (JSON dictionary).
  bool ParseSerializedRule(const std::string& serialized_rule);

  // Reads data from |json|, which must already have parsed a serialized rule.
  void ParseJsonRule(const Json& json);

  // Returns the ID string for this rule.
  const std::string& GetId() const { return id_; }

  // Returns the format elements for this rule. The format can include the
  // relevant address fields, but also strings used for formatting, or newline
  // information.
  const std::vector<FormatElement>& GetFormat() const { return format_; }

  // Returns the approximate address format with the Latin order of fields. The
  // format can include the relevant address fields, but also strings used for
  // formatting, or newline information.
  const std::vector<FormatElement>& GetLatinFormat() const {
    return latin_format_;
  }

  // Returns the required fields for this rule.
  const std::vector<AddressField>& GetRequired() const { return required_; }

  // Returns the sub-keys for this rule, which are the administrative areas of a
  // country, the localities of an administrative area, or the dependent
  // localities of a locality. For example, the rules for "US" have sub-keys of
  // "CA", "NY", "TX", etc.
  const std::vector<std::string>& GetSubKeys() const { return sub_keys_; }

  // Returns all of the language tags supported by this rule, for example ["de",
  // "fr", "it"].
  const std::vector<std::string>& GetLanguages() const { return languages_; }

  // Returns a pointer to a RE2 regular expression object created from the
  // postal code format string, if specified, or nullptr otherwise. The regular
  // expression is anchored to the beginning of the string so that it can be
  // used either with RE2::PartialMatch() to perform prefix matching or else
  // with RE2::FullMatch() to perform matching against the entire string.
  const RE2ptr* GetPostalCodeMatcher() const {
    return postal_code_matcher_.get();
  }

  // Returns the sole postal code for this rule, if there is one.
  const std::string& GetSolePostalCode() const { return sole_postal_code_; }

  // The message string identifier for admin area name. If not set, then
  // INVALID_MESSAGE_ID.
  int GetAdminAreaNameMessageId() const { return admin_area_name_message_id_; }

  // The message string identifier for postal code name. If not set, then
  // INVALID_MESSAGE_ID.
  int GetPostalCodeNameMessageId() const {
    return postal_code_name_message_id_;
  }

  // The message string identifier for locality name. If not set, then
  // INVALID_MESSAGE_ID.
  int GetLocalityNameMessageId() const {
    return locality_name_message_id_;
  }

  // The message string identifier for sublocality name. If not set, then
  // INVALID_MESSAGE_ID.
  int GetSublocalityNameMessageId() const {
    return sublocality_name_message_id_;
  }

  // Returns the name for the most specific place described by this rule, if
  // there is one. This is typically set when it differs from the key.
  const std::string& GetName() const { return name_; }

  // Returns the Latin-script name for the most specific place described by this
  // rule, if there is one.
  const std::string& GetLatinName() const { return latin_name_; }

  // Returns the postal code example string for this rule.
  const std::string& GetPostalCodeExample() const {
    return postal_code_example_;
  }

  // Returns the post service URL string for this rule.
  const std::string& GetPostServiceUrl() const { return post_service_url_; }

 private:
  std::string id_;
  std::vector<FormatElement> format_;
  std::vector<FormatElement> latin_format_;
  std::vector<AddressField> required_;
  std::vector<std::string> sub_keys_;
  std::vector<std::string> languages_;
  std::unique_ptr<const RE2ptr> postal_code_matcher_;
  std::string sole_postal_code_;
  int admin_area_name_message_id_;
  int postal_code_name_message_id_;
  int locality_name_message_id_;
  int sublocality_name_message_id_;
  std::string name_;
  std::string latin_name_;
  std::string postal_code_example_;
  std::string post_service_url_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_RULE_H_
