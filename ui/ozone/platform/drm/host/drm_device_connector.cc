// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_device_connector.h"

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/base/ui_base_features.h"
#include "ui/ozone/platform/drm/host/host_drm_device.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace {
// TODO(rjkroege): In the future when ozone/drm is always mojo-based, remove
// this utility code.
using BinderCallback = ui::GpuPlatformSupportHost::GpuHostBindInterfaceCallback;

void BindInterfaceInGpuProcess(const std::string& interface_name,
                               mojo::ScopedMessagePipeHandle interface_pipe,
                               const BinderCallback& binder_callback) {
  return binder_callback.Run(interface_name, std::move(interface_pipe));
}

template <typename Interface>
void BindInterfaceInGpuProcess(mojo::InterfaceRequest<Interface> request,
                               const BinderCallback& binder_callback) {
  BindInterfaceInGpuProcess(
      Interface::Name_, std::move(request.PassMessagePipe()), binder_callback);
}

}  // namespace

namespace ui {

DrmDeviceConnector::DrmDeviceConnector(
    service_manager::Connector* connector,
    scoped_refptr<HostDrmDevice> host_drm_device_)
    : connector_(connector),
      host_drm_device_(host_drm_device_),
      ws_runner_(base::ThreadTaskRunnerHandle::IsSet()
                     ? base::ThreadTaskRunnerHandle::Get()
                     : nullptr) {
  // Invariant: we only have a runner at startup if executing in mash mode.
  DCHECK((ws_runner_ && features::IsMashEnabled()) ||
         (!ws_runner_ && !features::IsMashEnabled()));
}

DrmDeviceConnector::~DrmDeviceConnector() {}

void DrmDeviceConnector::OnGpuProcessLaunched(
    int host_id,
    scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
    scoped_refptr<base::SingleThreadTaskRunner> send_runner,
    const base::RepeatingCallback<void(IPC::Message*)>& send_callback) {
  NOTREACHED();
}

void DrmDeviceConnector::OnChannelDestroyed(int host_id) {
  // TODO(rjkroege): Handle Viz restarting.
  NOTIMPLEMENTED();
}

void DrmDeviceConnector::OnGpuServiceLaunched(
    scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    GpuHostBindInterfaceCallback binder) {
  // We need to preserve |binder| to let us bind interfaces later.
  binder_callback_ = std::move(binder);
  if (am_running_in_ws_mode()) {
    ui::ozone::mojom::DrmDevicePtr drm_device_ptr_ui, drm_device_ptr_ws;
    ui::ozone::mojom::DeviceCursorPtr cursor_ptr_ws, cursor_ptr_io;

    BindInterfaceDrmDevice(&drm_device_ptr_ui);
    BindInterfaceDrmDevice(&drm_device_ptr_ws);
    BindInterfaceDeviceCursor(&cursor_ptr_ws);
    BindInterfaceDeviceCursor(&cursor_ptr_io);

    ws_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&HostDrmDevice::OnGpuServiceLaunched, host_drm_device_,
                       std::move(drm_device_ptr_ws), std::move(cursor_ptr_ws),
                       std::move(cursor_ptr_io)));

    ui_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&HostDrmDevice::OnGpuServiceLaunchedCompositor,
                       host_drm_device_, std::move(drm_device_ptr_ui)));

  } else {
    ui::ozone::mojom::DrmDevicePtr drm_device_ptr_ui;
    ui::ozone::mojom::DeviceCursorPtr cursor_ptr_ui, cursor_ptr_io;

    BindInterfaceDrmDevice(&drm_device_ptr_ui);
    BindInterfaceDeviceCursor(&cursor_ptr_ui);
    BindInterfaceDeviceCursor(&cursor_ptr_io);

    ui_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&HostDrmDevice::OnGpuServiceLaunched, host_drm_device_,
                       std::move(drm_device_ptr_ui), std::move(cursor_ptr_ui),
                       std::move(cursor_ptr_io)));

    ui_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&HostDrmDevice::OnGpuServiceLaunchedCompositor,
                       host_drm_device_, std::move(drm_device_ptr_ui)));
  }
}

void DrmDeviceConnector::OnMessageReceived(const IPC::Message& message) {
  NOTREACHED() << "This class should only be used with mojo transport but here "
                  "we're wrongly getting invoked to handle IPC communication.";
}

void DrmDeviceConnector::BindInterfaceDrmDevice(
    ui::ozone::mojom::DrmDevicePtr* drm_device_ptr) const {
  if (connector_) {
    connector_->BindInterface(ui::mojom::kServiceName, drm_device_ptr);
  } else {
    auto request = mojo::MakeRequest(drm_device_ptr);
    BindInterfaceInGpuProcess(std::move(request), binder_callback_);
  }
}

void DrmDeviceConnector::BindInterfaceDeviceCursor(
    ui::ozone::mojom::DeviceCursorPtr* cursor_ptr) const {
  if (connector_) {
    connector_->BindInterface(ui::mojom::kServiceName, cursor_ptr);
  } else {
    auto request = mojo::MakeRequest(cursor_ptr);
    BindInterfaceInGpuProcess(std::move(request), binder_callback_);
  }
}

}  // namespace ui
