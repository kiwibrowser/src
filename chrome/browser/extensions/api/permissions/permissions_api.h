// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PERMISSIONS_PERMISSIONS_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_PERMISSIONS_PERMISSIONS_API_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/permissions/permission_set.h"

namespace extensions {

// chrome.permissions.contains
class PermissionsContainsFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("permissions.contains", PERMISSIONS_CONTAINS)

 protected:
  ~PermissionsContainsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// chrome.permissions.getAll
class PermissionsGetAllFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("permissions.getAll", PERMISSIONS_GETALL)

 protected:
  ~PermissionsGetAllFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// chrome.permissions.remove
class PermissionsRemoveFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("permissions.remove", PERMISSIONS_REMOVE)

 protected:
  ~PermissionsRemoveFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// chrome.permissions.request
class PermissionsRequestFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("permissions.request", PERMISSIONS_REQUEST)

  PermissionsRequestFunction();

  // FOR TESTS ONLY to bypass the confirmation UI.
  static void SetAutoConfirmForTests(bool should_proceed);
  static void SetIgnoreUserGestureForTests(bool ignore);

 protected:
  ~PermissionsRequestFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void OnInstallPromptDone(ExtensionInstallPrompt::Result result);

  std::unique_ptr<ExtensionInstallPrompt> install_ui_;
  std::unique_ptr<const PermissionSet> requested_permissions_;

  DISALLOW_COPY_AND_ASSIGN(PermissionsRequestFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_PERMISSIONS_PERMISSIONS_API_H_
