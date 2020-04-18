// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/msi_util_win.h"

#include <windows.h>

// By default msi.h includes wincrypt.h which clashes with OpenSSL
// (both define X509_NAME) so to be able to include
// third_party/openssl (indirectly) in the same translation unit we
// tell msi.h to not include wincrypt.h.
#define _MSI_NO_CRYPTO
#include <msi.h>
#include <msiquery.h>

#include <utility>

#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"

namespace {

// Most strings returned by the MSI API are smaller than this value, so only
// 1 call to the API is needed in the common case.
constexpr DWORD kBufferInitialSize = 256;

// Retrieves the file path to the product's installer.
bool GetMsiPath(const base::string16& product_guid, base::string16* result) {
  DWORD buffer_size = kBufferInitialSize;
  UINT ret =
      ::MsiGetProductInfo(product_guid.c_str(), INSTALLPROPERTY_LOCALPACKAGE,
                          base::WriteInto(result, buffer_size), &buffer_size);
  if (ret == ERROR_MORE_DATA) {
    // Must account for the null terminator.
    buffer_size++;

    ret =
        ::MsiGetProductInfo(product_guid.c_str(), INSTALLPROPERTY_LOCALPACKAGE,
                            base::WriteInto(result, buffer_size), &buffer_size);
  }

  if (ret == ERROR_SUCCESS) {
    result->resize(buffer_size);
    return true;
  }
  return false;
}

// Returns the string value at position |index| in the given |record_handle|.
// Note that columns are 1-indexed.
bool GetRecordString(MSIHANDLE record_handle,
                     size_t index,
                     base::string16* result) {
  DWORD buffer_size = kBufferInitialSize;
  UINT ret = ::MsiRecordGetString(
      record_handle, index, base::WriteInto(result, buffer_size), &buffer_size);
  if (ret == ERROR_MORE_DATA) {
    // Must account for the null terminator.
    buffer_size++;

    ret = ::MsiRecordGetString(record_handle, index,
                               base::WriteInto(result, buffer_size),
                               &buffer_size);
  }

  if (ret == ERROR_SUCCESS) {
    result->resize(buffer_size);
    return true;
  }
  return false;
}

// Inspects the installer file and extracts the component guids. Each .msi file
// is actually an SQL database.
bool GetMsiComponentGuids(const base::string16& msi_database_path,
                          std::vector<base::string16>* component_guids) {
  PMSIHANDLE msi_database_handle;
  if (::MsiOpenDatabase(msi_database_path.c_str(), MSIDBOPEN_READONLY,
                        &msi_database_handle) != ERROR_SUCCESS) {
    return false;
  }

  PMSIHANDLE components_view_handle;
  if (::MsiDatabaseOpenView(msi_database_handle,
                            L"SELECT ComponentId FROM Component",
                            &components_view_handle) != ERROR_SUCCESS) {
    return false;
  }

  if (::MsiViewExecute(components_view_handle, 0) != ERROR_SUCCESS)
    return false;

  PMSIHANDLE record_handle;
  while (::MsiViewFetch(components_view_handle, &record_handle) ==
         ERROR_SUCCESS) {
    // The record only have the ComponentId column, and its index is 1.
    base::string16 component_guid;
    if (GetRecordString(record_handle, 1, &component_guid))
      component_guids->push_back(std::move(component_guid));
  }

  return true;
}

// Retrieves the |path| to the given component.
bool GetMsiComponentPath(const base::string16& product_guid,
                         const base::string16& component_guid,
                         base::string16* path) {
  DWORD buffer_size = kBufferInitialSize;
  INSTALLSTATE ret =
      ::MsiGetComponentPath(product_guid.c_str(), component_guid.c_str(),
                            base::WriteInto(path, buffer_size), &buffer_size);
  if (ret == INSTALLSTATE_MOREDATA) {
    // Must account for the null terminator.
    buffer_size++;

    ret =
        ::MsiGetComponentPath(product_guid.c_str(), component_guid.c_str(),
                              base::WriteInto(path, buffer_size), &buffer_size);
  }

  if (ret == INSTALLSTATE_LOCAL) {
    path->resize(buffer_size);
    return true;
  }
  return false;
}

}  // namespace

// The most efficient way to get the list of components associated to an
// installed product is to inspect the installer file. A copy of the installer
// exists somewhere on the file system because Windows needs it to uninstall the
// product.
//
// So this function retrieves the path to the installer, extracts the component
// GUIDS from it, and uses those to find the path of each component.
bool MsiUtil::GetMsiComponentPaths(
    const base::string16& product_guid,
    std::vector<base::string16>* component_paths) const {
  base::AssertBlockingAllowed();

  base::string16 msi_path;
  if (!GetMsiPath(product_guid, &msi_path))
    return false;

  std::vector<base::string16> component_guids;
  if (!GetMsiComponentGuids(msi_path, &component_guids))
    return false;

  for (const auto& component_guid : component_guids) {
    base::string16 component_path;
    if (!GetMsiComponentPath(product_guid, component_guid, &component_path))
      continue;

    component_paths->push_back(std::move(component_path));
  }

  return true;
}
