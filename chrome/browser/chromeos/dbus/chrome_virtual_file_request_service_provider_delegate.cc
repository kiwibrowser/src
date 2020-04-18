// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/chrome_virtual_file_request_service_provider_delegate.h"

#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_bridge.h"
#include "chrome/browser/profiles/profile.h"

namespace chromeos {

namespace {

arc::ArcFileSystemBridge* GetArcFileSystemBridge() {
  arc::ArcSessionManager* session_manager = arc::ArcSessionManager::Get();
  if (!session_manager)
    return nullptr;
  Profile* profile = session_manager->profile();
  if (!profile)
    return nullptr;
  return arc::ArcFileSystemBridge::GetForBrowserContext(profile);
}

}  // namespace

ChromeVirtualFileRequestServiceProviderDelegate::
    ChromeVirtualFileRequestServiceProviderDelegate() = default;

ChromeVirtualFileRequestServiceProviderDelegate::
    ~ChromeVirtualFileRequestServiceProviderDelegate() = default;

bool ChromeVirtualFileRequestServiceProviderDelegate::HandleReadRequest(
    const std::string& id,
    int64_t offset,
    int64_t size,
    base::ScopedFD pipe_write_end) {
  arc::ArcFileSystemBridge* bridge = GetArcFileSystemBridge();
  if (!bridge)
    return false;
  return bridge->HandleReadRequest(id, offset, size, std::move(pipe_write_end));
}

bool ChromeVirtualFileRequestServiceProviderDelegate::HandleIdReleased(
    const std::string& id) {
  arc::ArcFileSystemBridge* bridge = GetArcFileSystemBridge();
  if (!bridge)
    return false;
  return bridge->HandleIdReleased(id);
}

}  // namespace chromeos
