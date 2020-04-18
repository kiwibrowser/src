// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/file_handlers/non_native_file_system_delegate_chromeos.h"

#include "chrome/browser/chromeos/file_manager/filesystem_api_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

namespace extensions {

NonNativeFileSystemDelegateChromeOS::NonNativeFileSystemDelegateChromeOS() {}

NonNativeFileSystemDelegateChromeOS::~NonNativeFileSystemDelegateChromeOS() {}

bool NonNativeFileSystemDelegateChromeOS::IsUnderNonNativeLocalPath(
    content::BrowserContext* context,
    const base::FilePath& path) {
  return file_manager::util::IsUnderNonNativeLocalPath(
      Profile::FromBrowserContext(context), path);
}

void NonNativeFileSystemDelegateChromeOS::GetNonNativeLocalPathMimeType(
    content::BrowserContext* context,
    const base::FilePath& path,
    const base::Callback<void(bool, const std::string&)>& callback) {
  return file_manager::util::GetNonNativeLocalPathMimeType(
      Profile::FromBrowserContext(context), path, callback);
}

// Checks whether |path| points to a non-local filesystem directory and calls
// |callback| with the result asynchronously.
void NonNativeFileSystemDelegateChromeOS::IsNonNativeLocalPathDirectory(
    content::BrowserContext* context,
    const base::FilePath& path,
    const base::Callback<void(bool)>& callback) {
  file_manager::util::IsNonNativeLocalPathDirectory(
      Profile::FromBrowserContext(context), path, callback);
}

// Ensures a non-local file exists at |path|, i.e., it does nothing if a file
// is already present, or creates a file there if it isn't. Asynchronously
// calls |callback| with a success value.
void NonNativeFileSystemDelegateChromeOS::
    PrepareNonNativeLocalFileForWritableApp(
        content::BrowserContext* context,
        const base::FilePath& path,
        const base::Callback<void(bool)>& callback) {
  file_manager::util::PrepareNonNativeLocalFileForWritableApp(
      Profile::FromBrowserContext(context), path, callback);
}

}  // namespace extensions
