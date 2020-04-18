// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/base/serializers.h"

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"

namespace chromecast {

std::unique_ptr<base::Value> DeserializeFromJson(const std::string& text) {
  JSONStringValueDeserializer deserializer(text);

  int error_code = -1;
  std::string error_msg;
  std::unique_ptr<base::Value> value =
      deserializer.Deserialize(&error_code, &error_msg);
  DLOG_IF(ERROR, !value) << "JSON error " << error_code << ":" << error_msg;

  // Value will hold the nullptr in case of an error.
  return value;
}

std::unique_ptr<std::string> SerializeToJson(const base::Value& value) {
  std::unique_ptr<std::string> json_str(new std::string());
  JSONStringValueSerializer serializer(json_str.get());
  if (!serializer.Serialize(value))
    json_str.reset(nullptr);
  return json_str;
}

std::unique_ptr<base::Value> DeserializeJsonFromFile(
    const base::FilePath& path) {
  JSONFileValueDeserializer deserializer(path);

  int error_code = -1;
  std::string error_msg;
  std::unique_ptr<base::Value> value =
      deserializer.Deserialize(&error_code, &error_msg);
  DLOG_IF(ERROR, !value) << "JSON error " << error_code << ":" << error_msg;

  // Value will hold the nullptr in case of an error.
  return value;
}

bool SerializeJsonToFile(const base::FilePath& path, const base::Value& value) {
  JSONFileValueSerializer serializer(path);
  return serializer.Serialize(value);
}

}  // namespace chromecast
