// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/webstore/webstore_api_constants.h"

#include "base/macros.h"
#include "chrome/common/extensions/webstore_install_result.h"

namespace extensions {
namespace api {
namespace webstore {

// IMPORTANT: Keep this list in sync with both the definition in
// chrome/common/extensions/api/webstore.json and
// chrome/common/extensions/webstore_install_result.h!
const char* const kInstallResultCodes[] = {
  "success",
  "otherError",
  "aborted",
  "installInProgress",
  "notPermitted",
  "invalidId",
  "webstoreRequestError",
  "invalidWebstoreResponse",
  "invalidManifest",
  "iconError",
  "userCancelled",
  "blacklisted",
  "missingDependencies",
  "requirementViolations",
  "blockedByPolicy",
  "launchFeatureDisabled",
  "launchUnsupportedExtensionType",
  "launchInProgress",
};

static_assert(arraysize(kInstallResultCodes) ==
                  webstore_install::RESULT_LAST + 1,
              "kInstallResultCodes should equal RESULT_LAST + 1");

// The "downloading" stage begins when the installer starts downloading modules
// for the extension.
const char kInstallStageDownloading[] = "downloading";

// The "installing" stage begins once all downloads are complete, and the
// CrxInstaller begins.
const char kInstallStageInstalling[] = "installing";

// The method in custom_webstore_bindings.js triggered when we enter a new
// install stage ("downloading" or "installing").
const char kOnInstallStageChangedMethodName[] = "onInstallStageChanged";

// The method in custom_webstore_bindings.js triggered when we update
// download progress.
const char kOnDownloadProgressMethodName[] = "onDownloadProgress";

}  // namespace webstore
}  // namespace api
}  // namespace extensions
