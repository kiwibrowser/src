// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_XML_PARSER_H_
#define SERVICES_DATA_DECODER_XML_PARSER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "services/data_decoder/public/mojom/xml_parser.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace data_decoder {

class XmlParser : public mojom::XmlParser {
 public:
  explicit XmlParser(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~XmlParser() override;

 private:
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  // mojom::XmlParser implementation.
  void Parse(const std::string& xml, ParseCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(XmlParser);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_XML_PARSER_H_
