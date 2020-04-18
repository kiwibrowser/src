// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_thread_proxy.h"

#include "base/bind.h"
#include "ui/ozone/platform/drm/gpu/drm_thread_message_proxy.h"
#include "ui/ozone/platform/drm/gpu/drm_window_proxy.h"
#include "ui/ozone/platform/drm/gpu/gbm_buffer.h"
#include "ui/ozone/platform/drm/gpu/proxy_helpers.h"

namespace ui {

DrmThreadProxy::DrmThreadProxy() {}

DrmThreadProxy::~DrmThreadProxy() {}

// Used only with the paramtraits implementation.
void DrmThreadProxy::BindThreadIntoMessagingProxy(
    InterThreadMessagingProxy* messaging_proxy) {
  messaging_proxy->SetDrmThread(&drm_thread_);
}

// Used only for the mojo implementation.
void DrmThreadProxy::StartDrmThread(base::OnceClosure binding_drainer) {
  drm_thread_.Start(std::move(binding_drainer));
}

std::unique_ptr<DrmWindowProxy> DrmThreadProxy::CreateDrmWindowProxy(
    gfx::AcceleratedWidget widget) {
  return std::make_unique<DrmWindowProxy>(widget, &drm_thread_);
}

scoped_refptr<GbmBuffer> DrmThreadProxy::CreateBuffer(
    gfx::AcceleratedWidget widget,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  DCHECK(drm_thread_.task_runner())
      << "no task runner! in DrmThreadProxy::CreateBuffer";
  scoped_refptr<GbmBuffer> buffer;

  PostSyncTask(
      drm_thread_.task_runner(),
      base::BindOnce(&DrmThread::CreateBuffer, base::Unretained(&drm_thread_),
                     widget, size, format, usage, &buffer));
  return buffer;
}

scoped_refptr<GbmBuffer> DrmThreadProxy::CreateBufferFromFds(
    gfx::AcceleratedWidget widget,
    const gfx::Size& size,
    gfx::BufferFormat format,
    std::vector<base::ScopedFD>&& fds,
    const std::vector<gfx::NativePixmapPlane>& planes) {
  scoped_refptr<GbmBuffer> buffer;
  PostSyncTask(
      drm_thread_.task_runner(),
      base::BindOnce(&DrmThread::CreateBufferFromFds,
                     base::Unretained(&drm_thread_), widget, size, format,
                     base::Passed(std::move(fds)), planes, &buffer));
  return buffer;
}

void DrmThreadProxy::GetScanoutFormats(
    gfx::AcceleratedWidget widget,
    std::vector<gfx::BufferFormat>* scanout_formats) {
  PostSyncTask(
      drm_thread_.task_runner(),
      base::BindOnce(&DrmThread::GetScanoutFormats,
                     base::Unretained(&drm_thread_), widget, scanout_formats));
}

void DrmThreadProxy::AddBindingCursorDevice(
    ozone::mojom::DeviceCursorRequest request) {
  drm_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DrmThread::AddBindingCursorDevice,
                     base::Unretained(&drm_thread_), std::move(request)));
}

void DrmThreadProxy::AddBindingDrmDevice(
    ozone::mojom::DrmDeviceRequest request) {
  DCHECK(drm_thread_.task_runner()) << "DrmThreadProxy::AddBindingDrmDevice "
                                       "drm_thread_ task runner missing";

  drm_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DrmThread::AddBindingDrmDevice,
                     base::Unretained(&drm_thread_), std::move(request)));
}

}  // namespace ui
