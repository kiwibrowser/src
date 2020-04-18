// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_data_source.h"

#include "base/files/file_util.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"

namespace ui {

constexpr char kTextMimeTypeUtf8[] = "text/plain;charset=utf-8";

WaylandDataSource::WaylandDataSource(wl_data_source* data_source)
    : data_source_(data_source) {
  static const struct wl_data_source_listener kDataSourceListener = {
      WaylandDataSource::OnTarget, WaylandDataSource::OnSend,
      WaylandDataSource::OnCancel};
  wl_data_source_add_listener(data_source, &kDataSourceListener, this);
}

WaylandDataSource::~WaylandDataSource() = default;

void WaylandDataSource::WriteToClipboard(
    const ClipboardDelegate::DataMap& data_map) {
  for (const auto& data : data_map) {
    wl_data_source_offer(data_source_.get(), data.first.c_str());
    if (strcmp(data.first.c_str(), "text/plain") == 0)
      wl_data_source_offer(data_source_.get(), kTextMimeTypeUtf8);
  }
  wl_data_device_set_selection(connection_->data_device(), data_source_.get(),
                               connection_->serial());

  wl_display_flush(connection_->display());
}

void WaylandDataSource::UpdataDataMap(
    const ClipboardDelegate::DataMap& data_map) {
  data_map_ = data_map;
}

// static
void WaylandDataSource::OnTarget(void* data,
                                 wl_data_source* source,
                                 const char* mime_type) {
  NOTIMPLEMENTED();
}

// static
void WaylandDataSource::OnSend(void* data,
                               wl_data_source* source,
                               const char* mime_type,
                               int32_t fd) {
  WaylandDataSource* self = static_cast<WaylandDataSource*>(data);
  base::Optional<std::vector<uint8_t>> mime_data;
  self->GetClipboardData(mime_type, &mime_data);
  if (!mime_data.has_value() && strcmp(mime_type, kTextMimeTypeUtf8) == 0)
    self->GetClipboardData("text/plain", &mime_data);

  std::string contents(mime_data->begin(), mime_data->end());
  bool result =
      base::WriteFileDescriptor(fd, contents.data(), contents.length());
  DCHECK(result);
  close(fd);
}

// static
void WaylandDataSource::OnCancel(void* data, wl_data_source* source) {
  WaylandDataSource* self = static_cast<WaylandDataSource*>(data);
  self->connection_->DataSourceCancelled();
}

void WaylandDataSource::GetClipboardData(
    const std::string& mime_type,
    base::Optional<std::vector<uint8_t>>* data) {
  auto it = data_map_.find(mime_type);
  if (it != data_map_.end()) {
    data->emplace(it->second);
    // TODO: return here?
    return;
  }
}

}  // namespace ui
