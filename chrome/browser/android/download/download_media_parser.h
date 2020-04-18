// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_MEDIA_PARSER_H_
#define CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_MEDIA_PARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequenced_task_runner.h"
#include "chrome/common/media_galleries/metadata_types.h"
#include "chrome/services/media_gallery_util/public/cpp/safe_media_metadata_parser.h"
#include "chrome/services/media_gallery_util/public/mojom/media_parser.mojom.h"

// Local media files parser is used to process local media files. This object
// lives on main thread in browser process.
class DownloadMediaParser {
 public:
  explicit DownloadMediaParser(
      scoped_refptr<base::SequencedTaskRunner> file_task_runner);
  ~DownloadMediaParser();

  // Parse media metadata in a local file. All file IO will run on
  // |file_task_runner|. The metadata is parsed in an utility process safely.
  // However, the result is still comes from user-defined input, thus should be
  // used with caution.
  void ParseMediaFile(int64_t size,
                      const std::string& mime_type,
                      const base::FilePath& file_path,
                      SafeMediaMetadataParser::DoneCallback callback);

 private:
  // The task runner to do blocking disk IO.
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DownloadMediaParser);
};

#endif  // CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_MEDIA_PARSER_H_
