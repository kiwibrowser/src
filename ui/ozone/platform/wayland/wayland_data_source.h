// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_SOURCE_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_SOURCE_H_

#include <wayland-client.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/optional.h"
#include "ui/ozone/platform/wayland/wayland_object.h"
#include "ui/ozone/public/clipboard_delegate.h"

namespace ui {

class WaylandConnection;

// The WaylandDataSource object represents the source side of a
// WaylandDataOffer. It is created by the source client in a data
// transfer and provides a way to describe the offered data
// (wl_data_source_offer) // and a way to respond to requests to
// transfer the data (OnSend listener).
class WaylandDataSource {
 public:
  // Takes ownership of data_source.
  explicit WaylandDataSource(wl_data_source* data_source);
  ~WaylandDataSource();

  void set_connection(WaylandConnection* connection) {
    DCHECK(connection);
    connection_ = connection;
  }

  void WriteToClipboard(const ClipboardDelegate::DataMap& data_map);
  void UpdataDataMap(const ClipboardDelegate::DataMap& data_map);

 private:
  static void OnTarget(void* data,
                       wl_data_source* source,
                       const char* mime_type);
  static void OnSend(void* data,
                     wl_data_source* source,
                     const char* mime_type,
                     int32_t fd);
  static void OnCancel(void* data, wl_data_source* source);

  void GetClipboardData(const std::string& mime_type,
                        base::Optional<std::vector<uint8_t>>* data);

  wl::Object<wl_data_source> data_source_;
  WaylandConnection* connection_ = nullptr;

  ClipboardDelegate::DataMap data_map_;

  DISALLOW_COPY_AND_ASSIGN(WaylandDataSource);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_SOURCE_H_
