// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_DRIVEFS_DRIVE_FILE_STREAM_SERVICE_PROVIDER_DELEGATE_H_
#define CHROMEOS_COMPONENTS_DRIVEFS_DRIVE_FILE_STREAM_SERVICE_PROVIDER_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "chromeos/dbus/services/drive_file_stream_service_provider.h"

namespace drivefs {

// DriveFileStreamServiceProvider::Delegate implementation.
class COMPONENT_EXPORT(DRIVEFS) DriveFileStreamServiceProviderDelegate
    : public chromeos::DriveFileStreamServiceProvider::Delegate {
 public:
  DriveFileStreamServiceProviderDelegate();
  ~DriveFileStreamServiceProviderDelegate() override;

  // DriveFileStreamServiceProvider::Delegate overrides:
  bool OpenIpcChannel(const std::string& identity,
                      base::ScopedFD ipc_channel) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DriveFileStreamServiceProviderDelegate);
};

}  // namespace drivefs

#endif  // CHROMEOS_COMPONENTS_DRIVEFS_DRIVE_FILE_STREAM_SERVICE_PROVIDER_DELEGATE_H_
