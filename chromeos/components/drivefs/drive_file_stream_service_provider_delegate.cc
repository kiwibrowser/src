// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/drivefs/drive_file_stream_service_provider_delegate.h"

#include <utility>

#include "chromeos/components/drivefs/pending_connection_manager.h"

namespace drivefs {

DriveFileStreamServiceProviderDelegate::
    DriveFileStreamServiceProviderDelegate() = default;
DriveFileStreamServiceProviderDelegate::
    ~DriveFileStreamServiceProviderDelegate() = default;

bool DriveFileStreamServiceProviderDelegate::OpenIpcChannel(
    const std::string& identity,
    base::ScopedFD ipc_channel) {
  return PendingConnectionManager::Get().OpenIpcChannel(identity,
                                                        std::move(ipc_channel));
}

}  // namespace drivefs
