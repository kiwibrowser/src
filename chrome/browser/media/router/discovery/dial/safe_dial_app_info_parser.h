// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_APP_INFO_PARSER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_APP_INFO_PARSER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/values.h"
#include "chrome/browser/media/router/discovery/dial/parsed_dial_app_info.h"

namespace service_manager {
class Connector;
}

namespace media_router {

// SafeDialAppInfoParser parses the given app info XML file safely via a utility
// process.
// Spec for DIAL app info XML:
// http://www.dial-multiscreen.org/dial-protocol-specification
// Section 6.1.2 Server response.
class SafeDialAppInfoParser {
 public:
  enum ParsingResult {
    kSuccess = 0,
    kInvalidXML = 1,
    kFailToReadName = 2,
    kFailToReadState = 3,
    kMissingName = 4,
    kInvalidState = 5
  };

  // |connector| should be a valid connector to the ServiceManager.
  explicit SafeDialAppInfoParser(service_manager::Connector* connector);
  virtual ~SafeDialAppInfoParser();

  // Callback function invoked when done parsing DIAL app info XML.
  // |app_info|: app info object. Empty if parsing failed.
  // |parsing_error|: error encountered while parsing the DIAL app info XML.
  using ParseCallback =
      base::OnceCallback<void(std::unique_ptr<ParsedDialAppInfo> app_info,
                              ParsingResult parsing_result)>;

  // Parses the DIAL app info in |xml_text| in a utility process.
  // If the parsing succeeds, invokes callback with a valid
  // |app_info|, otherwise invokes callback with an empty
  // |app_info| and sets parsing error to detail the failure.
  // Note that it's safe to call this method multiple times and when making
  // multiple calls they may be grouped in the same utility process. The
  // utility process is still cleaned up automatically if unused after some
  // time, even if this object is still alive.
  // Note also that the callback is not called if the object is deleted.
  virtual void Parse(const std::string& xml_text, ParseCallback callback);

 private:
  void OnXmlParsingDone(ParseCallback callback,
                        std::unique_ptr<base::Value> value,
                        const base::Optional<std::string>& error);

  // Connector to the ServiceManager, used to retrieve the XmlParser service.
  service_manager::Connector* const connector_;

  // The batch ID used to group XML parsing calls to SafeXmlParser into a single
  // process.
  std::string xml_parser_batch_id_;

  base::WeakPtrFactory<SafeDialAppInfoParser> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SafeDialAppInfoParser);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_SAFE_DIAL_APP_INFO_PARSER_H_
