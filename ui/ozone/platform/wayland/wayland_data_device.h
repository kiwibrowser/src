// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_DEVICE_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_DEVICE_H_

#include <wayland-client.h>
#include <string>

#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "ui/ozone/platform/wayland/wayland_data_offer.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

namespace ui {

class WaylandDataOffer;
class WaylandConnection;

// This class provides access to inter-client data transfer mechanisms
// such as copy-and-paste and drag-and-drop mechanisms.
//
// TODO(tonikitoo,msisov): Add drag&drop support.
class WaylandDataDevice {
 public:
  WaylandDataDevice(WaylandConnection* connection, wl_data_device* data_device);
  ~WaylandDataDevice();

  void RequestSelectionData(const std::string& mime_type);

  wl_data_device* data_device() { return data_device_.get(); }

  std::vector<std::string> GetAvailableMimeTypes();

 private:
  void ReadClipboardDataFromFD(base::ScopedFD fd, const std::string& mime_type);

  // wl_data_device_listener callbacks
  static void OnDataOffer(void* data,
                          wl_data_device* data_device,
                          wl_data_offer* id);
  // Called by the compositor when the window gets pointer or keyboard focus,
  // or clipboard content changes behind the scenes.
  //
  // https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_data_device
  static void OnSelection(void* data,
                          wl_data_device* data_device,
                          wl_data_offer* id);

  static void SyncCallback(void* data, struct wl_callback* cb, uint32_t time);

  // The wl_data_device wrapped by this WaylandDataDevice.
  wl::Object<wl_data_device> data_device_;

  // Used to call out to WaylandConnection once clipboard data
  // has been successfully read.
  WaylandConnection* connection_ = nullptr;

  // There are two separate data offers at a time, the drag offer and the
  // selection offer, each with independent lifetimes. When we receive a new
  // offer, it is not immediately possible to know whether the new offer is the
  // drag offer or the selection offer. This variable is used to store ownership
  // of new data offers temporarily until its identity becomes known.
  std::unique_ptr<WaylandDataOffer> new_offer_;

  // Offer that holds the most-recent clipboard selection, or null if no
  // clipboard data is available.
  std::unique_ptr<WaylandDataOffer> selection_offer_;

  // Make sure server has written data on the pipe, before block on read().
  static const wl_callback_listener callback_listener_;
  base::OnceClosure read_from_fd_closure_;
  wl::Object<wl_callback> sync_callback_;

  DISALLOW_COPY_AND_ASSIGN(WaylandDataDevice);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_DEVICE_H_
