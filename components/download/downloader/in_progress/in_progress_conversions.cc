// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/in_progress_conversions.h"

#include <utility>
#include "base/logging.h"
#include "base/pickle.h"

namespace download {

DownloadEntry InProgressConversions::DownloadEntryFromProto(
    const metadata_pb::DownloadEntry& proto) {
  DownloadEntry entry;
  entry.guid = proto.guid();
  entry.request_origin = proto.request_origin();
  entry.download_source = DownloadSourceFromProto(proto.download_source());
  entry.ukm_download_id = proto.ukm_download_id();
  entry.bytes_wasted = proto.bytes_wasted();
  entry.fetch_error_body = proto.fetch_error_body();
  for (const auto& header : proto.request_headers()) {
    entry.request_headers.emplace_back(HttpRequestHeaderFromProto(header));
  }
  return entry;
}

metadata_pb::DownloadEntry InProgressConversions::DownloadEntryToProto(
    const DownloadEntry& entry) {
  metadata_pb::DownloadEntry proto;
  proto.set_guid(entry.guid);
  proto.set_request_origin(entry.request_origin);
  proto.set_download_source(DownloadSourceToProto(entry.download_source));
  proto.set_ukm_download_id(entry.ukm_download_id);
  proto.set_bytes_wasted(entry.bytes_wasted);
  proto.set_fetch_error_body(entry.fetch_error_body);
  for (const auto& header : entry.request_headers) {
    auto* proto_header = proto.add_request_headers();
    *proto_header = HttpRequestHeaderToProto(header);
  }
  return proto;
}

// static
DownloadSource InProgressConversions::DownloadSourceFromProto(
    metadata_pb::DownloadSource download_source) {
  switch (download_source) {
    case metadata_pb::DownloadSource::UNKNOWN:
      return DownloadSource::UNKNOWN;
    case metadata_pb::DownloadSource::NAVIGATION:
      return DownloadSource::NAVIGATION;
    case metadata_pb::DownloadSource::DRAG_AND_DROP:
      return DownloadSource::DRAG_AND_DROP;
    case metadata_pb::DownloadSource::FROM_RENDERER:
      return DownloadSource::FROM_RENDERER;
    case metadata_pb::DownloadSource::EXTENSION_API:
      return DownloadSource::EXTENSION_API;
    case metadata_pb::DownloadSource::EXTENSION_INSTALLER:
      return DownloadSource::EXTENSION_INSTALLER;
    case metadata_pb::DownloadSource::INTERNAL_API:
      return DownloadSource::INTERNAL_API;
    case metadata_pb::DownloadSource::WEB_CONTENTS_API:
      return DownloadSource::WEB_CONTENTS_API;
    case metadata_pb::DownloadSource::OFFLINE_PAGE:
      return DownloadSource::OFFLINE_PAGE;
    case metadata_pb::DownloadSource::CONTEXT_MENU:
      return DownloadSource::CONTEXT_MENU;
  }
  NOTREACHED();
  return DownloadSource::UNKNOWN;
}

// static
metadata_pb::DownloadSource InProgressConversions::DownloadSourceToProto(
    DownloadSource download_source) {
  switch (download_source) {
    case DownloadSource::UNKNOWN:
      return metadata_pb::DownloadSource::UNKNOWN;
    case DownloadSource::NAVIGATION:
      return metadata_pb::DownloadSource::NAVIGATION;
    case DownloadSource::DRAG_AND_DROP:
      return metadata_pb::DownloadSource::DRAG_AND_DROP;
    case DownloadSource::FROM_RENDERER:
      return metadata_pb::DownloadSource::FROM_RENDERER;
    case DownloadSource::EXTENSION_API:
      return metadata_pb::DownloadSource::EXTENSION_API;
    case DownloadSource::EXTENSION_INSTALLER:
      return metadata_pb::DownloadSource::EXTENSION_INSTALLER;
    case DownloadSource::INTERNAL_API:
      return metadata_pb::DownloadSource::INTERNAL_API;
    case DownloadSource::WEB_CONTENTS_API:
      return metadata_pb::DownloadSource::WEB_CONTENTS_API;
    case DownloadSource::OFFLINE_PAGE:
      return metadata_pb::DownloadSource::OFFLINE_PAGE;
    case DownloadSource::CONTEXT_MENU:
      return metadata_pb::DownloadSource::CONTEXT_MENU;
  }
  NOTREACHED();
  return metadata_pb::DownloadSource::UNKNOWN;
}

std::vector<DownloadEntry> InProgressConversions::DownloadEntriesFromProto(
    const metadata_pb::DownloadEntries& proto) {
  std::vector<DownloadEntry> entries;
  for (int i = 0; i < proto.entries_size(); i++)
    entries.push_back(DownloadEntryFromProto(proto.entries(i)));
  return entries;
}

metadata_pb::DownloadEntries InProgressConversions::DownloadEntriesToProto(
    const std::vector<DownloadEntry>& entries) {
  metadata_pb::DownloadEntries proto;
  for (size_t i = 0; i < entries.size(); i++) {
    metadata_pb::DownloadEntry* proto_entry = proto.add_entries();
    *proto_entry = DownloadEntryToProto(entries[i]);
  }
  return proto;
}

// static
metadata_pb::HttpRequestHeader InProgressConversions::HttpRequestHeaderToProto(
    const std::pair<std::string, std::string>& header) {
  metadata_pb::HttpRequestHeader proto;
  if (header.first.empty())
    return proto;

  proto.set_key(header.first);
  proto.set_value(header.second);
  return proto;
}

// static
std::pair<std::string, std::string>
InProgressConversions::HttpRequestHeaderFromProto(
    const metadata_pb::HttpRequestHeader& proto) {
  if (proto.key().empty())
    return std::pair<std::string, std::string>();

  return std::make_pair(proto.key(), proto.value());
}

// static
metadata_pb::InProgressInfo InProgressConversions::InProgressInfoToProto(
    const InProgressInfo& in_progress_info) {
  metadata_pb::InProgressInfo proto;
  for (size_t i = 0; i < in_progress_info.url_chain.size(); ++i)
    proto.add_url_chain(in_progress_info.url_chain[i].spec());
  proto.set_fetch_error_body(in_progress_info.fetch_error_body);
  for (const auto& header : in_progress_info.request_headers) {
    auto* proto_header = proto.add_request_headers();
    *proto_header = HttpRequestHeaderToProto(header);
  }
  proto.set_etag(in_progress_info.etag);
  proto.set_last_modified(in_progress_info.last_modified);
  proto.set_total_bytes(in_progress_info.total_bytes);
  base::Pickle current_path;
  in_progress_info.current_path.WriteToPickle(&current_path);
  proto.set_current_path(current_path.data(), current_path.size());
  base::Pickle target_path;
  in_progress_info.target_path.WriteToPickle(&target_path);
  proto.set_target_path(target_path.data(), target_path.size());
  proto.set_received_bytes(in_progress_info.received_bytes);
  proto.set_end_time(
      in_progress_info.end_time.ToDeltaSinceWindowsEpoch().InMilliseconds());
  for (size_t i = 0; i < in_progress_info.received_slices.size(); ++i) {
    metadata_pb::ReceivedSlice* slice = proto.add_received_slices();
    slice->set_received_bytes(
        in_progress_info.received_slices[i].received_bytes);
    slice->set_offset(in_progress_info.received_slices[i].offset);
    slice->set_finished(in_progress_info.received_slices[i].finished);
  }
  proto.set_hash(in_progress_info.hash);
  proto.set_transient(in_progress_info.transient);
  proto.set_state(in_progress_info.state);
  proto.set_danger_type(in_progress_info.danger_type);
  proto.set_interrupt_reason(in_progress_info.interrupt_reason);
  proto.set_paused(in_progress_info.paused);
  proto.set_metered(in_progress_info.metered);
  proto.set_request_origin(in_progress_info.request_origin);
  proto.set_bytes_wasted(in_progress_info.bytes_wasted);
  return proto;
}

// static
InProgressInfo InProgressConversions::InProgressInfoFromProto(
    const metadata_pb::InProgressInfo& proto) {
  InProgressInfo info;
  for (const auto& url : proto.url_chain())
    info.url_chain.emplace_back(url);
  info.fetch_error_body = proto.fetch_error_body();
  for (const auto& header : proto.request_headers())
    info.request_headers.emplace_back(HttpRequestHeaderFromProto(header));
  info.etag = proto.etag();
  info.last_modified = proto.last_modified();
  info.total_bytes = proto.total_bytes();
  base::PickleIterator current_path(
      base::Pickle(proto.current_path().data(), proto.current_path().size()));
  info.current_path.ReadFromPickle(&current_path);
  base::PickleIterator target_path(
      base::Pickle(proto.target_path().data(), proto.target_path().size()));
  info.target_path.ReadFromPickle(&target_path);
  info.received_bytes = proto.received_bytes();
  info.end_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::TimeDelta::FromMilliseconds(proto.end_time()));

  for (int i = 0; i < proto.received_slices_size(); ++i) {
    info.received_slices.emplace_back(proto.received_slices(i).offset(),
                                      proto.received_slices(i).received_bytes(),
                                      proto.received_slices(i).finished());
  }
  info.hash = proto.hash();
  info.transient = proto.transient();
  info.state = static_cast<DownloadItem::DownloadState>(proto.state());
  info.danger_type = static_cast<DownloadDangerType>(proto.danger_type());
  info.interrupt_reason =
      static_cast<DownloadInterruptReason>(proto.interrupt_reason());
  info.paused = proto.paused();
  info.metered = proto.metered();
  info.request_origin = proto.request_origin();
  info.bytes_wasted = proto.bytes_wasted();
  return info;
}

UkmInfo InProgressConversions::UkmInfoFromProto(
    const metadata_pb::UkmInfo& proto) {
  UkmInfo info;
  info.download_source = DownloadSourceFromProto(proto.download_source());
  info.ukm_download_id = proto.ukm_download_id();
  return info;
}

metadata_pb::UkmInfo InProgressConversions::UkmInfoToProto(
    const UkmInfo& info) {
  metadata_pb::UkmInfo proto;
  proto.set_download_source(DownloadSourceToProto(info.download_source));
  proto.set_ukm_download_id(info.ukm_download_id);
  return proto;
}

DownloadInfo InProgressConversions::DownloadInfoFromProto(
    const metadata_pb::DownloadInfo& proto) {
  DownloadInfo info;
  info.guid = proto.guid();
  if (proto.has_ukm_info())
    info.ukm_info = UkmInfoFromProto(proto.ukm_info());
  if (proto.has_in_progress_info())
    info.in_progress_info = InProgressInfoFromProto(proto.in_progress_info());
  return info;
}

metadata_pb::DownloadInfo InProgressConversions::DownloadInfoToProto(
    const DownloadInfo& info) {
  metadata_pb::DownloadInfo proto;
  proto.set_guid(info.guid);
  if (info.ukm_info.has_value()) {
    auto ukm_info = std::make_unique<metadata_pb::UkmInfo>(
        UkmInfoToProto(info.ukm_info.value()));
    proto.set_allocated_ukm_info(ukm_info.release());
  }
  if (info.in_progress_info.has_value()) {
    auto in_progress_info = std::make_unique<metadata_pb::InProgressInfo>(
        InProgressInfoToProto(info.in_progress_info.value()));
    proto.set_allocated_in_progress_info(in_progress_info.release());
  }
  return proto;
}

DownloadDBEntry InProgressConversions::DownloadDBEntryFromProto(
    const metadata_pb::DownloadDBEntry& proto) {
  DownloadDBEntry entry;
  entry.id = proto.id();
  if (proto.has_download_info())
    entry.download_info = DownloadInfoFromProto(proto.download_info());
  return entry;
}

metadata_pb::DownloadDBEntry InProgressConversions::DownloadDBEntryToProto(
    const DownloadDBEntry& info) {
  metadata_pb::DownloadDBEntry proto;
  proto.set_id(info.id);
  if (info.download_info.has_value()) {
    auto download_info = std::make_unique<metadata_pb::DownloadInfo>(
        DownloadInfoToProto(info.download_info.value()));
    proto.set_allocated_download_info(download_info.release());
  }
  return proto;
}

}  // namespace download
