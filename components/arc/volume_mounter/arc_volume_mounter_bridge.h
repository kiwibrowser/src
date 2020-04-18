// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VOLUME_MOUNTER_ARC_VOLUME_MOUNTER_BRIDGE_H_
#define COMPONENTS_ARC_VOLUME_MOUNTER_ARC_VOLUME_MOUNTER_BRIDGE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "components/arc/common/volume_mounter.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

// This class handles Volume mount/unmount requests from cros-disks and
// send them to Android.
class ArcVolumeMounterBridge
    : public KeyedService,
      public chromeos::disks::DiskMountManager::Observer,
      public ConnectionObserver<mojom::VolumeMounterInstance>,
      public mojom::VolumeMounterHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcVolumeMounterBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcVolumeMounterBridge(content::BrowserContext* context,
                         ArcBridgeService* bridge_service);
  ~ArcVolumeMounterBridge() override;

  // ConnectionObserver<mojom::VolumeMounterInstance> overrides:
  void OnConnectionReady() override;

  // chromeos::disks::DiskMountManager::Observer overrides:
  void OnAutoMountableDiskEvent(
      chromeos::disks::DiskMountManager::DiskEvent event,
      const chromeos::disks::DiskMountManager::Disk& disk) override;
  void OnBootDeviceDiskEvent(
      chromeos::disks::DiskMountManager::DiskEvent event,
      const chromeos::disks::DiskMountManager::Disk& disk) override;
  void OnDeviceEvent(chromeos::disks::DiskMountManager::DeviceEvent event,
                     const std::string& device_path) override;
  void OnMountEvent(chromeos::disks::DiskMountManager::MountEvent event,
                    chromeos::MountError error_code,
                    const chromeos::disks::DiskMountManager::MountPointInfo&
                        mount_info) override;
  void OnFormatEvent(chromeos::disks::DiskMountManager::FormatEvent event,
                     chromeos::FormatError error_code,
                     const std::string& device_path) override;
  void OnRenameEvent(chromeos::disks::DiskMountManager::RenameEvent event,
                     chromeos::RenameError error_code,
                     const std::string& device_path) override;

  // mojom::VolumeMounterHost overrides:
  void RequestAllMountPoints() override;

 private:
  void SendAllMountEvents();

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  base::WeakPtrFactory<ArcVolumeMounterBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcVolumeMounterBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_VOLUME_MOUNTER_ARC_VOLUME_MOUNTER_BRIDGE_H_
