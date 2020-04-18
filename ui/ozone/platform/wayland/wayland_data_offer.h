// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_OFFER_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_OFFER_H_

#include <wayland-client.h>

#include <string>
#include <vector>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

namespace ui {

// This class represents a piece of data offered for transfer by another
// client, the source client (see WaylandDataSource for more).
// It is used by the copy-and-paste and drag-and-drop mechanisms.
//
// The offer describes the different mime types that the data can be
// converted to and provides the mechanism for transferring the data
// directly from the source client.
//
// TODO(tonikitoo,msisov): Add drag&drop support.
class WaylandDataOffer {
 public:
  // Takes ownership of data_offer.
  explicit WaylandDataOffer(wl_data_offer* data_offer);
  ~WaylandDataOffer();

  const std::vector<std::string>& GetAvailableMimeTypes() const {
    return mime_types_;
  }

  // Some X11 applications on Gnome/Wayland (running through XWayland)
  // do not send the "text/plain" mime type that Chrome relies on, but
  // instead they send mime types like "text/plain;charset=utf-8".
  // When it happens, this method forcibly injects "text/plain" to the
  // list of provided mime types so that Chrome clipboard's machinery
  // works fine.
  void EnsureTextMimeTypeIfNeeded();

  // Creates a pipe (read & write FDs), passing the write-end of to pipe
  // to the compositor (via wl_data_offer_receive) and returning the
  // read-end to the pipe.
  base::ScopedFD Receive(const std::string& mime_type);

 private:
  // wl_data_offer_listener callbacks.
  static void OnOffer(void* data,
                      wl_data_offer* data_offer,
                      const char* mime_type);

  wl::Object<wl_data_offer> data_offer_;
  std::vector<std::string> mime_types_;

  bool text_plain_mime_type_inserted_ = false;

  DISALLOW_COPY_AND_ASSIGN(WaylandDataOffer);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_DATA_OFFER_H_
