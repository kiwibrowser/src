// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_ui_data.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"

namespace chromeos {

// Top-level UI data dictionary keys.
const char NetworkUIData::kKeyONCSource[] = "onc_source";
const char NetworkUIData::kKeyUserSettings[] = "user_settings";
const char NetworkUIData::kONCSourceUserImport[] = "user_import";
const char NetworkUIData::kONCSourceDevicePolicy[] = "device_policy";
const char NetworkUIData::kONCSourceUserPolicy[] = "user_policy";

namespace {

template <typename Enum>
struct StringEnumEntry {
  const char* string;
  Enum enum_value;
};

const StringEnumEntry< ::onc::ONCSource> kONCSourceTable[] = {
  { NetworkUIData::kONCSourceUserImport, ::onc::ONC_SOURCE_USER_IMPORT },
  { NetworkUIData::kONCSourceDevicePolicy, ::onc::ONC_SOURCE_DEVICE_POLICY },
  { NetworkUIData::kONCSourceUserPolicy, ::onc::ONC_SOURCE_USER_POLICY }
};

// Converts |enum_value| to the corresponding string according to |table|. If no
// enum value of the table matches (which can only occur if incorrect casting
// was used to obtain |enum_value|), returns an empty string instead.
template <typename Enum, int N>
std::string EnumToString(const StringEnumEntry<Enum>(& table)[N],
                         Enum enum_value) {
  for (int i = 0; i < N; ++i) {
    if (table[i].enum_value == enum_value)
      return table[i].string;
  }
  return std::string();
}

// Converts |str| to the corresponding enum value according to |table|. If no
// string of the table matches, returns |fallback| instead.
template<typename Enum, int N>
Enum StringToEnum(const StringEnumEntry<Enum>(& table)[N],
                  const std::string& str,
                  Enum fallback) {
  for (int i = 0; i < N; ++i) {
    if (table[i].string == str)
      return table[i].enum_value;
  }
  return fallback;
}

}  // namespace

NetworkUIData::NetworkUIData() : onc_source_(::onc::ONC_SOURCE_NONE) {
}

NetworkUIData::NetworkUIData(const NetworkUIData& other) {
  *this = other;
}

NetworkUIData& NetworkUIData::operator=(const NetworkUIData& other) {
  onc_source_ = other.onc_source_;
  if (other.user_settings_)
    user_settings_.reset(other.user_settings_->DeepCopy());
  return *this;
}

NetworkUIData::NetworkUIData(const base::DictionaryValue& dict) {
  std::string source;
  dict.GetString(kKeyONCSource, &source);
  onc_source_ = StringToEnum(kONCSourceTable, source, ::onc::ONC_SOURCE_NONE);

  std::string type_string;

  const base::DictionaryValue* user_settings = NULL;
  if (dict.GetDictionary(kKeyUserSettings, &user_settings))
    user_settings_.reset(user_settings->DeepCopy());
}

NetworkUIData::~NetworkUIData() = default;

void NetworkUIData::set_user_settings(
    std::unique_ptr<base::DictionaryValue> dict) {
  user_settings_ = std::move(dict);
}

std::string NetworkUIData::GetONCSourceAsString() const {
  return EnumToString(kONCSourceTable, onc_source_);
}

void NetworkUIData::FillDictionary(base::DictionaryValue* dict) const {
  dict->Clear();

  std::string source_string = GetONCSourceAsString();
  if (!source_string.empty())
    dict->SetString(kKeyONCSource, source_string);

  if (user_settings_)
    dict->SetKey(kKeyUserSettings, user_settings_->Clone());
}

// static
std::unique_ptr<NetworkUIData> NetworkUIData::CreateFromONC(
    ::onc::ONCSource onc_source) {
  std::unique_ptr<NetworkUIData> ui_data(new NetworkUIData());

  ui_data->onc_source_ = onc_source;

  return ui_data;
}

}  // namespace chromeos
