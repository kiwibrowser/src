// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/document_scan/document_scan_interface.h"

namespace extensions {

namespace api {

DocumentScanInterface::DocumentScanInterface() {
}

DocumentScanInterface::~DocumentScanInterface() {
}

DocumentScanInterface::ScannerDescription::ScannerDescription() {
}

DocumentScanInterface::ScannerDescription::ScannerDescription(
    const ScannerDescription& other) = default;

DocumentScanInterface::ScannerDescription::~ScannerDescription() {
}

}  // namespace api

}  // namespace extensions
