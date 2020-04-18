// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/api/module/module.h"

#include <memory>
#include <string>

#include "base/values.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/manifest_url_handlers.h"

namespace extensions {

namespace extension {

namespace {

// A preference for storing the extension's update URL data. If not empty, the
// the ExtensionUpdater will append a ap= parameter to the URL when checking if
// a new version of the extension is available.
const char kUpdateURLData[] = "update_url_data";

}  // namespace

std::string GetUpdateURLData(const ExtensionPrefs* prefs,
                             const std::string& extension_id) {
  std::string data;
  prefs->ReadPrefAsString(extension_id, kUpdateURLData, &data);
  return data;
}

}  // namespace extension

ExtensionFunction::ResponseAction ExtensionSetUpdateUrlDataFunction::Run() {
  std::string data;
  EXTENSION_FUNCTION_VALIDATE(args_->GetString(0, &data));

  if (ManifestURL::UpdatesFromGallery(extension())) {
    return RespondNow(Error(kUnknownErrorDoNotUse));
  }

  ExtensionPrefs::Get(browser_context())
      ->UpdateExtensionPref(extension_id(), extension::kUpdateURLData,
                            std::make_unique<base::Value>(data));
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
ExtensionIsAllowedIncognitoAccessFunction::Run() {
  return RespondNow(OneArgument(std::make_unique<base::Value>(
      util::IsIncognitoEnabled(extension_id(), browser_context()))));
}

ExtensionFunction::ResponseAction
ExtensionIsAllowedFileSchemeAccessFunction::Run() {
  // Deny file scheme access.
  return RespondNow(OneArgument(std::make_unique<base::Value>(false)));
}

}  // namespace extensions
