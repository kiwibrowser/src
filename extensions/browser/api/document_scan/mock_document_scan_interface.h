// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DOCUMENT_SCAN_MOCK_DOCUMENT_SCAN_INTERFACE_H_
#define EXTENSIONS_BROWSER_API_DOCUMENT_SCAN_MOCK_DOCUMENT_SCAN_INTERFACE_H_

#include <string>

#include <gmock/gmock.h>

#include "extensions/browser/api/document_scan/document_scan_interface.h"

namespace extensions {

namespace api {

class MockDocumentScanInterface : public DocumentScanInterface {
 public:
  MockDocumentScanInterface();
  ~MockDocumentScanInterface() override;

  MOCK_METHOD4(Scan,
               void(const std::string& scanner_name,
                    ScanMode mode,
                    int resolution_dpi,
                    const ScanResultsCallback& callback));
  MOCK_METHOD1(ListScanners, void(const ListScannersResultsCallback& callback));
  MOCK_CONST_METHOD0(GetImageMimeType, std::string());
};

}  // namespace api

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DOCUMENT_SCAN_MOCK_DOCUMENT_SCAN_INTERFACE_H_
