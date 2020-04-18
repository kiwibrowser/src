// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/install_static/install_modes.h"

namespace install_static {

namespace {

std::wstring GetUnregisteredKeyPathForProduct(const wchar_t* product) {
  return std::wstring(L"Software\\").append(product);
}

std::wstring GetClientsKeyPathForApp(const wchar_t* app_guid) {
  return std::wstring(L"Software\\Google\\Update\\Clients\\").append(app_guid);
}

std::wstring GetClientStateKeyPathForApp(const wchar_t* app_guid) {
  return std::wstring(L"Software\\Google\\Update\\ClientState\\")
      .append(app_guid);
}

std::wstring GetClientStateMediumKeyPathForApp(const wchar_t* app_guid) {
  return std::wstring(L"Software\\Google\\Update\\ClientStateMedium\\")
      .append(app_guid);
}

}  // namespace

std::wstring GetClientsKeyPath(const wchar_t* app_guid) {
  if (!kUseGoogleUpdateIntegration)
    return GetUnregisteredKeyPathForProduct(kProductPathName);
  return GetClientsKeyPathForApp(app_guid);
}

std::wstring GetClientStateKeyPath(const wchar_t* app_guid) {
  if (!kUseGoogleUpdateIntegration)
    return GetUnregisteredKeyPathForProduct(kProductPathName);
  return GetClientStateKeyPathForApp(app_guid);
}

std::wstring GetBinariesClientsKeyPath() {
  if (!kUseGoogleUpdateIntegration)
    return GetUnregisteredKeyPathForProduct(kBinariesPathName);
  return GetClientsKeyPathForApp(kBinariesAppGuid);
}

std::wstring GetBinariesClientStateKeyPath() {
  if (!kUseGoogleUpdateIntegration)
    return GetUnregisteredKeyPathForProduct(kBinariesPathName);
  return GetClientStateKeyPathForApp(kBinariesAppGuid);
}

std::wstring GetClientStateMediumKeyPath(const wchar_t* app_guid) {
  if (!kUseGoogleUpdateIntegration)
    return GetUnregisteredKeyPathForProduct(kProductPathName);
  return GetClientStateMediumKeyPathForApp(app_guid);
}

std::wstring GetBinariesClientStateMediumKeyPath() {
  if (!kUseGoogleUpdateIntegration)
    return GetUnregisteredKeyPathForProduct(kBinariesPathName);
  return GetClientStateMediumKeyPathForApp(kBinariesAppGuid);
}

}  // namespace install_static
