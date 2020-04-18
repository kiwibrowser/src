// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/download_media_parser.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "chrome/browser/android/download/local_media_data_source_factory.h"
#include "content/public/common/service_manager_connection.h"

namespace {

void OnParseMetadataDone(
    std::unique_ptr<SafeMediaMetadataParser> parser_keep_alive,
    SafeMediaMetadataParser::DoneCallback done_callback,
    bool parse_success,
    chrome::mojom::MediaMetadataPtr metadata,
    std::unique_ptr<std::vector<metadata::AttachedImage>> attached_images) {
  // Call done callback on main thread.
  std::move(done_callback)
      .Run(parse_success, std::move(metadata), std::move(attached_images));
}

}  // namespace

DownloadMediaParser::DownloadMediaParser(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner)
    : file_task_runner_(file_task_runner) {}

DownloadMediaParser::~DownloadMediaParser() = default;

void DownloadMediaParser::ParseMediaFile(
    int64_t size,
    const std::string& mime_type,
    const base::FilePath& file_path,
    SafeMediaMetadataParser::DoneCallback callback) {
  auto media_data_source_factory =
      std::make_unique<LocalMediaDataSourceFactory>(file_path,
                                                    file_task_runner_);
  auto parser = std::make_unique<SafeMediaMetadataParser>(
      size, mime_type, true /* get_attached_images */,
      std::move(media_data_source_factory));
  SafeMediaMetadataParser* parser_ptr = parser.get();
  parser_ptr->Start(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      base::BindOnce(&OnParseMetadataDone, std::move(parser),
                     std::move(callback)));
}
