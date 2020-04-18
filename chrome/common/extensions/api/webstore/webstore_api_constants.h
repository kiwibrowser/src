// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_API_WEBSTORE_WEBSTORE_API_CONSTANTS_H_
#define CHROME_COMMON_EXTENSIONS_API_WEBSTORE_WEBSTORE_API_CONSTANTS_H_

namespace extensions {
namespace api {
namespace webstore {

// An enum for listener types. This is used when creating/reading the mask for
// IPC messages.
enum ListenerType {
  INSTALL_STAGE_LISTENER = 1,
  DOWNLOAD_PROGRESS_LISTENER = 1 << 1
};

// An enum to represent which stage the installation is in.
enum InstallStage {
  INSTALL_STAGE_DOWNLOADING = 0,
  INSTALL_STAGE_INSTALLING,
};

// Result codes returned by WebstoreStandaloneInstaller and its subclasses.
// IMPORTANT: Keep this list in sync with both the definition in
// chrome/common/extensions/api/webstore.json and
// chrome/common/extensions/webstore_install_result.h!
extern const char* const kInstallResultCodes[];

extern const char kInstallStageDownloading[];
extern const char kInstallStageInstalling[];
extern const char kOnInstallStageChangedMethodName[];
extern const char kOnDownloadProgressMethodName[];

}  // namespace webstore
}  // namespace api
}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_API_WEBSTORE_WEBSTORE_API_CONSTANTS_H_
