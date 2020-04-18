// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cloud_devices/common/cloud_device_description.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "components/cloud_devices/common/cloud_device_description_consts.h"

namespace cloud_devices {

CloudDeviceDescription::CloudDeviceDescription()
    : root_(std::make_unique<base::DictionaryValue>()) {
  root_->SetString(json::kVersion, json::kVersion10);
}

CloudDeviceDescription::~CloudDeviceDescription() = default;

bool CloudDeviceDescription::InitFromString(const std::string& json) {
  auto parsed = base::DictionaryValue::From(base::JSONReader::Read(json));
  if (!parsed)
    return false;

  root_ = std::move(parsed);
  const base::Value* version = root_->FindKey(json::kVersion);
  return version && version->GetString() == json::kVersion10;
}

std::string CloudDeviceDescription::ToString() const {
  std::string json;
  base::JSONWriter::WriteWithOptions(
      *root_, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  return json;
}

const base::DictionaryValue* CloudDeviceDescription::GetItem(
    const std::string& path) const {
  const base::DictionaryValue* value = nullptr;
  root_->GetDictionary(path, &value);
  return value;
}

base::DictionaryValue* CloudDeviceDescription::CreateItem(
    const std::string& path) {
  return root_->SetDictionary(path, std::make_unique<base::DictionaryValue>());
}

const base::ListValue* CloudDeviceDescription::GetListItem(
    const std::string& path) const {
  const base::ListValue* value = nullptr;
  root_->GetList(path, &value);
  return value;
}

base::ListValue* CloudDeviceDescription::CreateListItem(
    const std::string& path) {
  return root_->SetList(path, std::make_unique<base::ListValue>());
}

}  // namespace cloud_devices
