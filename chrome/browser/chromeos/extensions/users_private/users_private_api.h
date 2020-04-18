// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_USERS_PRIVATE_USERS_PRIVATE_API_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_USERS_PRIVATE_USERS_PRIVATE_API_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

// Implements the chrome.usersPrivate.getWhitelistedUsers method.
class UsersPrivateGetWhitelistedUsersFunction
    : public UIThreadExtensionFunction {
 public:
  UsersPrivateGetWhitelistedUsersFunction();
  DECLARE_EXTENSION_FUNCTION("usersPrivate.getWhitelistedUsers",
                             USERSPRIVATE_GETWHITELISTEDUSERS);

 protected:
  ~UsersPrivateGetWhitelistedUsersFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  ChromeExtensionFunctionDetails chrome_details_;

  DISALLOW_COPY_AND_ASSIGN(UsersPrivateGetWhitelistedUsersFunction);
};

// Implements the chrome.usersPrivate.addWhitelistedUser method.
class UsersPrivateAddWhitelistedUserFunction
    : public UIThreadExtensionFunction {
 public:
  UsersPrivateAddWhitelistedUserFunction();
  DECLARE_EXTENSION_FUNCTION("usersPrivate.addWhitelistedUser",
                             USERSPRIVATE_ADDWHITELISTEDUSER);

 protected:
  ~UsersPrivateAddWhitelistedUserFunction() override;

  // UIThreadExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  ChromeExtensionFunctionDetails chrome_details_;

  DISALLOW_COPY_AND_ASSIGN(UsersPrivateAddWhitelistedUserFunction);
};

// Implements the chrome.usersPrivate.removeWhitelistedUser method.
class UsersPrivateRemoveWhitelistedUserFunction
    : public UIThreadExtensionFunction {
 public:
  UsersPrivateRemoveWhitelistedUserFunction();
  DECLARE_EXTENSION_FUNCTION("usersPrivate.removeWhitelistedUser",
                             USERSPRIVATE_REMOVEWHITELISTEDUSER);

 protected:
  ~UsersPrivateRemoveWhitelistedUserFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  ChromeExtensionFunctionDetails chrome_details_;

  DISALLOW_COPY_AND_ASSIGN(UsersPrivateRemoveWhitelistedUserFunction);
};

// Implements the chrome.usersPrivate.isWhitelistManaged method.
class UsersPrivateIsWhitelistManagedFunction
    : public UIThreadExtensionFunction {
 public:
  UsersPrivateIsWhitelistManagedFunction();
  DECLARE_EXTENSION_FUNCTION("usersPrivate.isWhitelistManaged",
                             USERSPRIVATE_ISWHITELISTMANAGED);

 protected:
  ~UsersPrivateIsWhitelistManagedFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UsersPrivateIsWhitelistManagedFunction);
};

// Implements the chrome.usersPrivate.getCurrentUser method.
class UsersPrivateGetCurrentUserFunction : public UIThreadExtensionFunction {
 public:
  UsersPrivateGetCurrentUserFunction();
  DECLARE_EXTENSION_FUNCTION("usersPrivate.getCurrentUser",
                             USERSPRIVATE_GETCURRENTUSER);

 protected:
  ~UsersPrivateGetCurrentUserFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(UsersPrivateGetCurrentUserFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_USERS_PRIVATE_USERS_PRIVATE_API_H_
