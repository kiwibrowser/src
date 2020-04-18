// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_data_device.h"

#include "base/bind.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"

namespace ui {

// static
const wl_callback_listener WaylandDataDevice::callback_listener_ = {
    WaylandDataDevice::SyncCallback,
};

WaylandDataDevice::WaylandDataDevice(WaylandConnection* connection,
                                     wl_data_device* data_device)
    : data_device_(data_device), connection_(connection) {
  static const struct wl_data_device_listener kDataDeviceListener = {
      WaylandDataDevice::OnDataOffer,
      nullptr /*OnEnter*/,
      nullptr /*OnLeave*/,
      nullptr /*OnMotion*/,
      nullptr /*OnDrop*/,
      WaylandDataDevice::OnSelection};
  wl_data_device_add_listener(data_device_.get(), &kDataDeviceListener, this);
}

WaylandDataDevice::~WaylandDataDevice() {}

void WaylandDataDevice::RequestSelectionData(const std::string& mime_type) {
  base::ScopedFD fd = selection_offer_->Receive(mime_type);
  if (!fd.is_valid()) {
    LOG(ERROR) << "Failed to open file descriptor.";
    return;
  }

  // Ensure there is not pending operation to be performed by the compositor,
  // otherwise read(..) can block awaiting data to be sent to pipe.
  read_from_fd_closure_ =
      base::BindOnce(&WaylandDataDevice::ReadClipboardDataFromFD,
                     base::Unretained(this), std::move(fd), mime_type);
  sync_callback_.reset(wl_display_sync(connection_->display()));
  wl_callback_add_listener(sync_callback_.get(), &callback_listener_, this);
  wl_display_flush(connection_->display());
}

void WaylandDataDevice::ReadClipboardDataFromFD(base::ScopedFD fd,
                                                const std::string& mime_type) {
  std::string contents;
  char buffer[1 << 10];  // 1 kB in bytes.
  ssize_t length;
  while ((length = read(fd.get(), buffer, sizeof(buffer))) > 0)
    contents.append(buffer, length);

  connection_->SetClipboardData(contents, mime_type);
}

std::vector<std::string> WaylandDataDevice::GetAvailableMimeTypes() {
  if (selection_offer_)
    return selection_offer_->GetAvailableMimeTypes();

  return std::vector<std::string>();
}

// static
void WaylandDataDevice::OnDataOffer(void* data,
                                    wl_data_device* data_device,
                                    wl_data_offer* offer) {
  auto* self = static_cast<WaylandDataDevice*>(data);

  DCHECK(!self->new_offer_);
  self->new_offer_.reset(new WaylandDataOffer(offer));
}

// static
void WaylandDataDevice::OnSelection(void* data,
                                    wl_data_device* data_device,
                                    wl_data_offer* offer) {
  auto* self = static_cast<WaylandDataDevice*>(data);
  DCHECK(self);

  // 'offer' will be null to indicate that the selection is no longer valid,
  // i.e. there is no longer clipboard data available to paste.
  if (!offer) {
    self->selection_offer_.reset();

    // Clear Clipboard cache.
    self->connection_->SetClipboardData(std::string(), std::string());
    return;
  }

  DCHECK(self->new_offer_);
  self->selection_offer_ = std::move(self->new_offer_);

  self->selection_offer_->EnsureTextMimeTypeIfNeeded();
}

void WaylandDataDevice::SyncCallback(void* data,
                                     struct wl_callback* cb,
                                     uint32_t time) {
  WaylandDataDevice* data_device = static_cast<WaylandDataDevice*>(data);
  DCHECK(data_device);

  std::move(data_device->read_from_fd_closure_).Run();
  DCHECK(data_device->read_from_fd_closure_.is_null());
  data_device->sync_callback_.reset();
}

}  // namespace ui
