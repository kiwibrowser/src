// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation notes about interactions with VideoCaptureImpl.
//
// How is VideoCaptureImpl used:
//
// VideoCaptureImpl is an IO thread object while VideoCaptureImplManager
// lives only on the render thread. It is only possible to access an
// object of VideoCaptureImpl via a task on the IO thread.
//
// How is VideoCaptureImpl deleted:
//
// A task is posted to the IO thread to delete a VideoCaptureImpl.
// Immediately after that the pointer to it is dropped. This means no
// access to this VideoCaptureImpl object is possible on the render
// thread. Also note that VideoCaptureImpl does not post task to itself.
//
// The use of Unretained:
//
// We make sure deletion is the last task on the IO thread for a
// VideoCaptureImpl object. This allows the use of Unretained() binding.

#include "content/renderer/media/video_capture_impl_manager.h"

#include <algorithm>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_process.h"
#include "content/renderer/media/video_capture_impl.h"

namespace content {

struct VideoCaptureImplManager::DeviceEntry {
  media::VideoCaptureSessionId session_id;

  // To be used and destroyed only on the IO thread.
  std::unique_ptr<VideoCaptureImpl> impl;

  // Number of clients using |impl|.
  int client_count;

  // This is set to true if this device is being suspended, via
  // VideoCaptureImplManager::Suspend().
  // See also: VideoCaptureImplManager::is_suspending_all_.
  bool is_individually_suspended;

  DeviceEntry()
      : session_id(0), client_count(0), is_individually_suspended(false) {}
  DeviceEntry(DeviceEntry&& other) = default;
  DeviceEntry& operator=(DeviceEntry&& other) = default;
  ~DeviceEntry() = default;
};

VideoCaptureImplManager::VideoCaptureImplManager()
    : next_client_id_(0),
      render_main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      is_suspending_all_(false),
      weak_factory_(this) {}

VideoCaptureImplManager::~VideoCaptureImplManager() {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  if (devices_.empty())
    return;
  // Forcibly release all video capture resources.
  for (auto& entry : devices_) {
    ChildProcess::current()->io_task_runner()->DeleteSoon(FROM_HERE,
                                                          entry.impl.release());
  }
  devices_.clear();
}

base::Closure VideoCaptureImplManager::UseDevice(
    media::VideoCaptureSessionId id) {
  DVLOG(1) << __func__ << " session id: " << id;
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  if (it == devices_.end()) {
    devices_.push_back(DeviceEntry());
    it = devices_.end() - 1;
    it->session_id = id;
    it->impl = CreateVideoCaptureImplForTesting(id);
    if (!it->impl)
      it->impl.reset(new VideoCaptureImpl(id));
  }
  ++it->client_count;

  // Design limit: When there are multiple clients, VideoCaptureImplManager
  // would have to individually track which ones requested suspending/resuming,
  // in order to determine whether the whole device should be suspended.
  // Instead, handle the non-common use case of multiple clients by just
  // resuming the suspended device, and disable suspend functionality while
  // there are multiple clients.
  if (it->is_individually_suspended)
    Resume(id);

  return base::Bind(&VideoCaptureImplManager::UnrefDevice,
                    weak_factory_.GetWeakPtr(), id);
}

base::Closure VideoCaptureImplManager::StartCapture(
    media::VideoCaptureSessionId id,
    const media::VideoCaptureParams& params,
    const VideoCaptureStateUpdateCB& state_update_cb,
    const VideoCaptureDeliverFrameCB& deliver_frame_cb) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());

  // This ID is used to identify a client of VideoCaptureImpl.
  const int client_id = ++next_client_id_;

  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::StartCapture,
                                base::Unretained(it->impl.get()), client_id,
                                params, state_update_cb, deliver_frame_cb));
  return base::Bind(&VideoCaptureImplManager::StopCapture,
                    weak_factory_.GetWeakPtr(), client_id, id);
}

void VideoCaptureImplManager::RequestRefreshFrame(
    media::VideoCaptureSessionId id) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::RequestRefreshFrame,
                                base::Unretained(it->impl.get())));
}

void VideoCaptureImplManager::Suspend(media::VideoCaptureSessionId id) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  if (it->is_individually_suspended)
    return;  // Device has already been individually suspended.
  if (it->client_count > 1)
    return;  // Punt when there is >1 client (see comments in UseDevice()).
  it->is_individually_suspended = true;
  if (is_suspending_all_)
    return;  // Device should already be suspended.
  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::SuspendCapture,
                                base::Unretained(it->impl.get()), true));
}

void VideoCaptureImplManager::Resume(media::VideoCaptureSessionId id) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  if (!it->is_individually_suspended)
    return;  // Device was not individually suspended.
  it->is_individually_suspended = false;
  if (is_suspending_all_)
    return;  // Device must remain suspended until all are resumed.
  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::SuspendCapture,
                                base::Unretained(it->impl.get()), false));
}

void VideoCaptureImplManager::GetDeviceSupportedFormats(
    media::VideoCaptureSessionId id,
    const VideoCaptureDeviceFormatsCB& callback) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::GetDeviceSupportedFormats,
                                base::Unretained(it->impl.get()), callback));
}

void VideoCaptureImplManager::GetDeviceFormatsInUse(
    media::VideoCaptureSessionId id,
    const VideoCaptureDeviceFormatsCB& callback) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::GetDeviceFormatsInUse,
                                base::Unretained(it->impl.get()), callback));
}

std::unique_ptr<VideoCaptureImpl>
VideoCaptureImplManager::CreateVideoCaptureImplForTesting(
    media::VideoCaptureSessionId session_id) const {
  return std::unique_ptr<VideoCaptureImpl>();
}

void VideoCaptureImplManager::StopCapture(int client_id,
                                          media::VideoCaptureSessionId id) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  ChildProcess::current()->io_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureImpl::StopCapture,
                                base::Unretained(it->impl.get()), client_id));
}

void VideoCaptureImplManager::UnrefDevice(
    media::VideoCaptureSessionId id) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  const auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [id] (const DeviceEntry& entry) { return entry.session_id == id; });
  DCHECK(it != devices_.end());
  DCHECK_GT(it->client_count, 0);
  --it->client_count;
  if (it->client_count > 0)
    return;
  ChildProcess::current()->io_task_runner()->DeleteSoon(FROM_HERE,
                                                        it->impl.release());
  devices_.erase(it);
}

void VideoCaptureImplManager::SuspendDevices(
    const MediaStreamDevices& video_devices,
    bool suspend) {
  DCHECK(render_main_task_runner_->BelongsToCurrentThread());
  if (is_suspending_all_ == suspend)
    return;
  is_suspending_all_ = suspend;
  for (const MediaStreamDevice& device : video_devices) {
    const media::VideoCaptureSessionId id = device.session_id;
    const auto it = std::find_if(
        devices_.begin(), devices_.end(),
        [id](const DeviceEntry& entry) { return entry.session_id == id; });
    DCHECK(it != devices_.end());
    if (it->is_individually_suspended)
      continue;  // Either: 1) Already suspended; or 2) Should not be resumed.
    ChildProcess::current()->io_task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&VideoCaptureImpl::SuspendCapture,
                                  base::Unretained(it->impl.get()), suspend));
  }
}

}  // namespace content
