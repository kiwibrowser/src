// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/device_info/local_device_info_provider_impl.h"

#include "base/bind.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "components/sync/base/get_session_name.h"
#include "components/sync/driver/sync_util.h"

namespace syncer {

namespace {

sync_pb::SyncEnums::DeviceType GetLocalDeviceType(bool is_tablet) {
#if defined(OS_CHROMEOS)
  return sync_pb::SyncEnums_DeviceType_TYPE_CROS;
#elif defined(OS_LINUX)
  return sync_pb::SyncEnums_DeviceType_TYPE_LINUX;
#elif defined(OS_ANDROID) || defined(OS_IOS)
  return is_tablet ? sync_pb::SyncEnums_DeviceType_TYPE_TABLET
                   : sync_pb::SyncEnums_DeviceType_TYPE_PHONE;
#elif defined(OS_MACOSX)
  return sync_pb::SyncEnums_DeviceType_TYPE_MAC;
#elif defined(OS_WIN)
  return sync_pb::SyncEnums_DeviceType_TYPE_WIN;
#else
  return sync_pb::SyncEnums_DeviceType_TYPE_OTHER;
#endif
}

}  // namespace

LocalDeviceInfoProviderImpl::LocalDeviceInfoProviderImpl(
    version_info::Channel channel,
    const std::string& version,
    bool is_tablet)
    : channel_(channel),
      version_(version),
      is_tablet_(is_tablet),
      weak_factory_(this) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

LocalDeviceInfoProviderImpl::~LocalDeviceInfoProviderImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

const DeviceInfo* LocalDeviceInfoProviderImpl::GetLocalDeviceInfo() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return local_device_info_.get();
}

std::string LocalDeviceInfoProviderImpl::GetSyncUserAgent() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return MakeUserAgentForSync(channel_, is_tablet_);
}

std::string LocalDeviceInfoProviderImpl::GetLocalSyncCacheGUID() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return cache_guid_;
}

std::unique_ptr<LocalDeviceInfoProvider::Subscription>
LocalDeviceInfoProviderImpl::RegisterOnInitializedCallback(
    const base::Closure& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!local_device_info_);
  return callback_list_.Add(callback);
}

void LocalDeviceInfoProviderImpl::Initialize(
    const std::string& cache_guid,
    const std::string& signin_scoped_device_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!cache_guid.empty());
  cache_guid_ = cache_guid;

  GetSessionName(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
      base::Bind(&LocalDeviceInfoProviderImpl::InitializeContinuation,
                 weak_factory_.GetWeakPtr(), cache_guid,
                 signin_scoped_device_id));
}

void LocalDeviceInfoProviderImpl::InitializeContinuation(
    const std::string& guid,
    const std::string& signin_scoped_device_id,
    const std::string& session_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (guid != cache_guid_) {
    // Clear() happened before this callback; abort.
    return;
  }

  local_device_info_ = std::make_unique<DeviceInfo>(
      guid, session_name, version_, GetSyncUserAgent(),
      GetLocalDeviceType(is_tablet_), signin_scoped_device_id);

  // Notify observers.
  callback_list_.Notify();
}

void LocalDeviceInfoProviderImpl::Clear() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  cache_guid_ = "";
  local_device_info_.reset();
}

}  // namespace syncer
