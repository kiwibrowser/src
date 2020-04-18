// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/updater/extension_downloader_delegate.h"

#include "base/logging.h"
#include "base/version.h"

namespace extensions {

ExtensionDownloaderDelegate::PingResult::PingResult() : did_ping(false) {
}

ExtensionDownloaderDelegate::PingResult::~PingResult() {
}

ExtensionDownloaderDelegate::~ExtensionDownloaderDelegate() {
}

void ExtensionDownloaderDelegate::OnExtensionDownloadFailed(
    const std::string& id,
    ExtensionDownloaderDelegate::Error error,
    const ExtensionDownloaderDelegate::PingResult& ping_result,
    const std::set<int>& request_id) {
}

bool ExtensionDownloaderDelegate::GetPingDataForExtension(
    const std::string& id,
    ManifestFetchData::PingData* ping) {
  return false;
}

std::string ExtensionDownloaderDelegate::GetUpdateUrlData(
    const std::string& id) {
  return std::string();
}

}  // namespace extensions
