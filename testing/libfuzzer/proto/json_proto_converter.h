// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_LIBFUZZER_PROTO_JSON_PROTO_CONVERTER_H
#define TESTING_LIBFUZZER_PROTO_JSON_PROTO_CONVERTER_H

#include "json.pb.h"

#include <sstream>
#include <string>

namespace json_proto {

class JsonProtoConverter {
 public:
  std::string Convert(const json_proto::JsonObject&);

 private:
  std::stringstream data_;

  void AppendArray(const json_proto::ArrayValue&);
  void AppendNumber(const json_proto::NumberValue&);
  void AppendObject(const json_proto::JsonObject&);
  void AppendValue(const json_proto::JsonValue&);
};

}  // namespace json_proto

#endif  // TESTING_LIBFUZZER_PROTO_JSON_PROTO_CONVERTER_H
