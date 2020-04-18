// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_data_offer.h"

#include <fcntl.h>
#include <algorithm>

#include "base/logging.h"

namespace ui {

namespace {

const char kString[] = "STRING";
const char kText[] = "TEXT";
const char kTextPlain[] = "text/plain";
const char kTextPlainUtf8[] = "text/plain;charset=utf-8";
const char kUtf8String[] = "UTF8_STRING";

void CreatePipe(base::ScopedFD* read_pipe, base::ScopedFD* write_pipe) {
  int raw_pipe[2];
  PCHECK(0 == pipe(raw_pipe));
  read_pipe->reset(raw_pipe[0]);
  write_pipe->reset(raw_pipe[1]);
}

}  // namespace

WaylandDataOffer::WaylandDataOffer(wl_data_offer* data_offer)
    : data_offer_(data_offer) {
  static const struct wl_data_offer_listener kDataOfferListener = {
      WaylandDataOffer::OnOffer};
  wl_data_offer_add_listener(data_offer, &kDataOfferListener, this);
}

WaylandDataOffer::~WaylandDataOffer() {
  data_offer_.reset();
}

void WaylandDataOffer::EnsureTextMimeTypeIfNeeded() {
  if (std::find(mime_types_.begin(), mime_types_.end(), kTextPlain) !=
      mime_types_.end())
    return;

  if (std::any_of(mime_types_.begin(), mime_types_.end(),
                  [](const std::string& mime_type) {
                    return mime_type == kString || mime_type == kText ||
                           mime_type == kTextPlainUtf8 ||
                           mime_type == kUtf8String;
                  })) {
    mime_types_.push_back(kTextPlain);
    text_plain_mime_type_inserted_ = true;
  }
}

base::ScopedFD WaylandDataOffer::Receive(const std::string& mime_type) {
  if (std::find(mime_types_.begin(), mime_types_.end(), mime_type) ==
      mime_types_.end())
    return base::ScopedFD();

  base::ScopedFD read_fd;
  base::ScopedFD write_fd;
  CreatePipe(&read_fd, &write_fd);

  // If we needed to forcibly write "text/plain" as an available
  // mimetype, then it is safer to "read" the clipboard data with
  // a mimetype mime_type known to be available.
  std::string effective_mime_type = mime_type;
  if (mime_type == kTextPlain && text_plain_mime_type_inserted_) {
    effective_mime_type = kTextPlainUtf8;
  }

  wl_data_offer_receive(data_offer_.get(), effective_mime_type.data(),
                        write_fd.get());
  return read_fd;
}

// static
void WaylandDataOffer::OnOffer(void* data,
                               wl_data_offer* data_offer,
                               const char* mime_type) {
  auto* self = static_cast<WaylandDataOffer*>(data);
  self->mime_types_.push_back(mime_type);
}

}  // namespace ui
