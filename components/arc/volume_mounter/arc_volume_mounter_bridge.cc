// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/volume_mounter/arc_volume_mounter_bridge.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"

using chromeos::disks::DiskMountManager;

namespace arc {

namespace {

// Singleton factory for ArcVolumeMounterBridge.
class ArcVolumeMounterBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcVolumeMounterBridge,
          ArcVolumeMounterBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcVolumeMounterBridgeFactory";

  static ArcVolumeMounterBridgeFactory* GetInstance() {
    return base::Singleton<ArcVolumeMounterBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcVolumeMounterBridgeFactory>;
  ArcVolumeMounterBridgeFactory() = default;
  ~ArcVolumeMounterBridgeFactory() override = default;
};

}  // namespace

// static
ArcVolumeMounterBridge* ArcVolumeMounterBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcVolumeMounterBridgeFactory::GetForBrowserContext(context);
}

ArcVolumeMounterBridge::ArcVolumeMounterBridge(content::BrowserContext* context,
                                               ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service), weak_ptr_factory_(this) {
  arc_bridge_service_->volume_mounter()->AddObserver(this);
  arc_bridge_service_->volume_mounter()->SetHost(this);
  DCHECK(DiskMountManager::GetInstance());
  DiskMountManager::GetInstance()->AddObserver(this);
}

ArcVolumeMounterBridge::~ArcVolumeMounterBridge() {
  DiskMountManager::GetInstance()->RemoveObserver(this);
  arc_bridge_service_->volume_mounter()->SetHost(nullptr);
  arc_bridge_service_->volume_mounter()->RemoveObserver(this);
}

// Sends MountEvents of all existing MountPoints in cros-disks.
void ArcVolumeMounterBridge::SendAllMountEvents() {
  for (const auto& keyValue : DiskMountManager::GetInstance()->mount_points()) {
    OnMountEvent(DiskMountManager::MountEvent::MOUNTING,
                 chromeos::MountError::MOUNT_ERROR_NONE, keyValue.second);
  }
}

void ArcVolumeMounterBridge::OnConnectionReady() {
  // Deferring the SendAllMountEvents as a task to current thread to not
  // block the mojo request since SendAllMountEvents might takes non trivial
  // amount of time.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&ArcVolumeMounterBridge::SendAllMountEvents,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ArcVolumeMounterBridge::OnAutoMountableDiskEvent(
    chromeos::disks::DiskMountManager::DiskEvent event,
    const chromeos::disks::DiskMountManager::Disk& disk) {
  // Ignored. DiskEvents will be maintained in Vold during MountEvents.
}

void ArcVolumeMounterBridge::OnBootDeviceDiskEvent(
    chromeos::disks::DiskMountManager::DiskEvent event,
    const chromeos::disks::DiskMountManager::Disk& disk) {
  // Ignored. ARC doesn't care about boot device disk events.
}

void ArcVolumeMounterBridge::OnDeviceEvent(
    chromeos::disks::DiskMountManager::DeviceEvent event,
    const std::string& device_path) {
  // Ignored. ARC doesn't care about events other than Disk and Mount events.
}

void ArcVolumeMounterBridge::OnFormatEvent(
    chromeos::disks::DiskMountManager::FormatEvent event,
    chromeos::FormatError error_code,
    const std::string& device_path) {
  // Ignored. ARC doesn't care about events other than Disk and Mount events.
}

void ArcVolumeMounterBridge::OnRenameEvent(
    chromeos::disks::DiskMountManager::RenameEvent event,
    chromeos::RenameError error_code,
    const std::string& device_path) {
  // Ignored. ARC doesn't care about events other than Disk and Mount events.
}

void ArcVolumeMounterBridge::OnMountEvent(
    DiskMountManager::MountEvent event,
    chromeos::MountError error_code,
    const chromeos::disks::DiskMountManager::MountPointInfo& mount_info) {
  if (error_code != chromeos::MountError::MOUNT_ERROR_NONE) {
    DVLOG(1) << "Error " << error_code << "occurs during MountEvent " << event;
    return;
  }

  // Get disks informations that are needed by Android MountService.
  const chromeos::disks::DiskMountManager::Disk* disk =
      DiskMountManager::GetInstance()->FindDiskBySourcePath(
          mount_info.source_path);
  std::string fs_uuid, device_label;
  chromeos::DeviceType device_type = chromeos::DeviceType::DEVICE_TYPE_UNKNOWN;
  // There are several cases where disk can be null:
  // 1. The disk is removed physically before being ejected/unmounted.
  // 2. The disk is inserted, but then immediately removed physically. The
  //    disk removal will race with mount event in this case.
  if (disk) {
    fs_uuid = disk->fs_uuid();
    device_label = disk->device_label();
    device_type = disk->device_type();
  } else {
    DVLOG(1) << "Disk at " << mount_info.source_path
             << " is null during MountEvent " << event;
  }

  mojom::VolumeMounterInstance* volume_mounter_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->volume_mounter(),
                                  OnMountEvent);

  if (!volume_mounter_instance)
    return;

  volume_mounter_instance->OnMountEvent(mojom::MountPointInfo::New(
      event, mount_info.source_path, mount_info.mount_path, fs_uuid,
      device_label, device_type));
}

void ArcVolumeMounterBridge::RequestAllMountPoints() {
  // Deferring the SendAllMountEvents as a task to current thread to not
  // block the mojo request since SendAllMountEvents might takes non trivial
  // amount of time.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&ArcVolumeMounterBridge::SendAllMountEvents,
                                weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace arc
