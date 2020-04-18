// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_IN_PROGRESS_CONVERSIONS_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_IN_PROGRESS_CONVERSIONS_H_

#include "base/macros.h"
#include "components/download/downloader/in_progress/download_db_entry.h"
#include "components/download/downloader/in_progress/download_entry.h"
#include "components/download/downloader/in_progress/download_info.h"
#include "components/download/downloader/in_progress/in_progress_info.h"
#include "components/download/downloader/in_progress/proto/download_entry.pb.h"
#include "components/download/downloader/in_progress/proto/download_source.pb.h"
#include "components/download/downloader/in_progress/ukm_info.h"

namespace download {

class InProgressConversions {
 public:
  static DownloadEntry DownloadEntryFromProto(
      const metadata_pb::DownloadEntry& proto);

  static metadata_pb::DownloadEntry DownloadEntryToProto(
      const DownloadEntry& entry);

  static DownloadSource DownloadSourceFromProto(
      metadata_pb::DownloadSource download_source);

  static metadata_pb::DownloadSource DownloadSourceToProto(
      DownloadSource download_source);

  static std::vector<DownloadEntry> DownloadEntriesFromProto(
      const metadata_pb::DownloadEntries& proto);

  static metadata_pb::DownloadEntries DownloadEntriesToProto(
      const std::vector<DownloadEntry>& entries);

  static metadata_pb::HttpRequestHeader HttpRequestHeaderToProto(
      const std::pair<std::string, std::string>& header);

  static std::pair<std::string, std::string> HttpRequestHeaderFromProto(
      const metadata_pb::HttpRequestHeader& proto);

  static metadata_pb::InProgressInfo InProgressInfoToProto(
      const InProgressInfo& in_progress_info);

  static InProgressInfo InProgressInfoFromProto(
      const metadata_pb::InProgressInfo& proto);

  static metadata_pb::UkmInfo UkmInfoToProto(const UkmInfo& ukm_info);

  static UkmInfo UkmInfoFromProto(const metadata_pb::UkmInfo& proto);

  static metadata_pb::DownloadInfo DownloadInfoToProto(
      const DownloadInfo& download_info);

  static DownloadInfo DownloadInfoFromProto(
      const metadata_pb::DownloadInfo& proto);

  static metadata_pb::DownloadDBEntry DownloadDBEntryToProto(
      const DownloadDBEntry& entry);

  static DownloadDBEntry DownloadDBEntryFromProto(
      const metadata_pb::DownloadDBEntry& proto);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_IN_PROGRESS_CONVERSIONS_H_
