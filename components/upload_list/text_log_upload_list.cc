// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/upload_list/text_log_upload_list.h"

#include <string>

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

TextLogUploadList::TextLogUploadList(const base::FilePath& upload_log_path)
    : UploadList(), upload_log_path_(upload_log_path) {}

TextLogUploadList::~TextLogUploadList() = default;

base::TaskTraits TextLogUploadList::LoadingTaskTraits() {
  return {base::MayBlock(), base::TaskPriority::BACKGROUND,
          base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN};
}

std::vector<UploadList::UploadInfo> TextLogUploadList::LoadUploadList() {
  std::vector<UploadInfo> uploads;

  if (base::PathExists(upload_log_path_)) {
    std::string contents;
    base::ReadFileToString(upload_log_path_, &contents);
    std::vector<std::string> log_entries =
        base::SplitString(contents, base::kWhitespaceASCII,
                          base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    ParseLogEntries(log_entries, &uploads);
  }

  return uploads;
}

void TextLogUploadList::ParseLogEntries(
    const std::vector<std::string>& log_entries,
    std::vector<UploadInfo>* uploads) {
  std::vector<std::string>::const_reverse_iterator i;
  for (i = log_entries.rbegin(); i != log_entries.rend(); ++i) {
    std::vector<std::string> components =
        base::SplitString(*i, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    // Skip any blank (or corrupted) lines.
    if (components.size() < 2 || components.size() > 5)
      continue;
    base::Time upload_time;
    double seconds_since_epoch;
    if (!components[0].empty()) {
      if (!base::StringToDouble(components[0], &seconds_since_epoch))
        continue;
      upload_time = base::Time::FromDoubleT(seconds_since_epoch);
    }
    UploadInfo info(components[1], upload_time);

    // Add local ID if present.
    if (components.size() > 2)
      info.local_id = components[2];

    // Add capture time if present.
    if (components.size() > 3 && !components[3].empty() &&
        base::StringToDouble(components[3], &seconds_since_epoch)) {
      info.capture_time = base::Time::FromDoubleT(seconds_since_epoch);
    }

    int state;
    if (components.size() > 4 && !components[4].empty() &&
        base::StringToInt(components[4], &state)) {
      info.state = static_cast<UploadInfo::State>(state);
    }

    uploads->push_back(info);
  }
}
