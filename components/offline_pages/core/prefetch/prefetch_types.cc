// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

namespace {
std::string PrefetchEnumToString(PrefetchBackgroundTaskRescheduleType value) {
  switch (value) {
    case PrefetchBackgroundTaskRescheduleType::NO_RESCHEDULE:
      return "NO_RESCHEDULE";
    case PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITHOUT_BACKOFF:
      return "RESCHEDULE_WITHOUT_BACKOFF";
    case PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITH_BACKOFF:
      return "RESCHEDULE_WITH_BACKOFF";
    case PrefetchBackgroundTaskRescheduleType::RESCHEDULE_DUE_TO_SYSTEM:
      return "RESCHEDULE_DUE_TO_SYSTEM";
    case PrefetchBackgroundTaskRescheduleType::SUSPEND:
      return "SUSPEND";
  }
  CHECK(false) << static_cast<int>(value) << " not valid enum value";
}

std::string PrefetchEnumToString(PrefetchRequestStatus value) {
  switch (value) {
    case PrefetchRequestStatus::SUCCESS:
      return "SUCCESS";
    case PrefetchRequestStatus::SHOULD_RETRY_WITHOUT_BACKOFF:
      return "SHOULD_RETRY_WITHOUT_BACKOFF";
    case PrefetchRequestStatus::SHOULD_RETRY_WITH_BACKOFF:
      return "SHOULD_RETRY_WITH_BACKOFF";
    case PrefetchRequestStatus::SHOULD_SUSPEND:
      return "SHOULD_SUSPEND";
    case PrefetchRequestStatus::COUNT:
      return "COUNT";
  }
  CHECK(false) << static_cast<int>(value) << " not valid enum value";
}

std::string PrefetchEnumToString(RenderStatus value) {
  switch (value) {
    case RenderStatus::RENDERED:
      return "RENDERED";
    case RenderStatus::PENDING:
      return "PENDING";
    case RenderStatus::FAILED:
      return "FAILED";
    case RenderStatus::EXCEEDED_LIMIT:
      return "EXCEEDED_LIMIT";
  }
  CHECK(false) << static_cast<int>(value) << " not valid enum value";
}

std::string PrefetchEnumToString(PrefetchItemState value) {
  switch (value) {
    case PrefetchItemState::NEW_REQUEST:
      return "NEW_REQUEST";
    case PrefetchItemState::SENT_GENERATE_PAGE_BUNDLE:
      return "SENT_GENERATE_PAGE_BUNDLE";
    case PrefetchItemState::AWAITING_GCM:
      return "AWAITING_GCM";
    case PrefetchItemState::RECEIVED_GCM:
      return "RECEIVED_GCM";
    case PrefetchItemState::SENT_GET_OPERATION:
      return "SENT_GET_OPERATION";
    case PrefetchItemState::RECEIVED_BUNDLE:
      return "RECEIVED_BUNDLE";
    case PrefetchItemState::DOWNLOADING:
      return "DOWNLOADING";
    case PrefetchItemState::DOWNLOADED:
      return "DOWNLOADED";
    case PrefetchItemState::IMPORTING:
      return "IMPORTING";
    case PrefetchItemState::FINISHED:
      return "FINISHED";
    case PrefetchItemState::ZOMBIE:
      return "ZOMBIE";
  }
  CHECK(false) << static_cast<int>(value) << " not valid enum value";
}

std::string PrefetchEnumToString(PrefetchItemErrorCode value) {
  switch (value) {
    case PrefetchItemErrorCode::SUCCESS:
      return "SUCCESS";
    case PrefetchItemErrorCode::TOO_MANY_NEW_URLS:
      return "TOO_MANY_NEW_URLS";
    case PrefetchItemErrorCode::DOWNLOAD_ERROR:
      return "DOWNLOAD_ERROR";
    case PrefetchItemErrorCode::IMPORT_ERROR:
      return "IMPORT_ERROR";
    case PrefetchItemErrorCode::ARCHIVING_FAILED:
      return "ARCHIVING_FAILED";
    case PrefetchItemErrorCode::ARCHIVING_LIMIT_EXCEEDED:
      return "ARCHIVING_LIMIT_EXCEEDED";
    case PrefetchItemErrorCode::STALE_AT_NEW_REQUEST:
      return "STALE_AT_NEW_REQUEST";
    case PrefetchItemErrorCode::STALE_AT_AWAITING_GCM:
      return "STALE_AT_AWAITING_GCM";
    case PrefetchItemErrorCode::STALE_AT_RECEIVED_GCM:
      return "STALE_AT_RECEIVED_GCM";
    case PrefetchItemErrorCode::STALE_AT_RECEIVED_BUNDLE:
      return "STALE_AT_RECEIVED_BUNDLE";
    case PrefetchItemErrorCode::STALE_AT_DOWNLOADING:
      return "STALE_AT_DOWNLOADING";
    case PrefetchItemErrorCode::STALE_AT_IMPORTING:
      return "STALE_AT_IMPORTING";
    case PrefetchItemErrorCode::STALE_AT_UNKNOWN:
      return "STALE_AT_UNKNOWN";
    case PrefetchItemErrorCode::GET_OPERATION_MAX_ATTEMPTS_REACHED:
      return "GET_OPERATION_MAX_ATTEMPTS_REACHED";
    case PrefetchItemErrorCode::
        GENERATE_PAGE_BUNDLE_REQUEST_MAX_ATTEMPTS_REACHED:
      return "GENERATE_PAGE_BUNDLE_REQUEST_MAX_ATTEMPTS_REACHED";
    case PrefetchItemErrorCode::DOWNLOAD_MAX_ATTEMPTS_REACHED:
      return "DOWNLOAD_MAX_ATTEMPTS_REACHED";
    case PrefetchItemErrorCode::MAXIMUM_CLOCK_BACKWARD_SKEW_EXCEEDED:
      return "MAXIMUM_CLOCK_BACKWARD_SKEW_EXCEEDED";
    case PrefetchItemErrorCode::IMPORT_LOST:
      return "IMPORT_LOST";
    case PrefetchItemErrorCode::SUGGESTION_INVALIDATED:
      return "SUGGESTION_INVALIDATED";
  }
  CHECK(false) << static_cast<int>(value) << " not valid enum value";
}
}  // namespace

RenderPageInfo::RenderPageInfo() = default;

RenderPageInfo::RenderPageInfo(const RenderPageInfo& other) = default;

PrefetchDownloadResult::PrefetchDownloadResult() = default;

PrefetchDownloadResult::PrefetchDownloadResult(const std::string& download_id,
                                               const base::FilePath& file_path,
                                               int64_t file_size)
    : download_id(download_id),
      success(true),
      file_path(file_path),
      file_size(file_size) {}

PrefetchDownloadResult::PrefetchDownloadResult(
    const PrefetchDownloadResult& other) = default;

bool PrefetchDownloadResult::operator==(
    const PrefetchDownloadResult& other) const {
  return download_id == other.download_id && success == other.success &&
         file_path == other.file_path && file_size == other.file_size;
}

PrefetchArchiveInfo::PrefetchArchiveInfo() = default;

PrefetchArchiveInfo::PrefetchArchiveInfo(const PrefetchArchiveInfo& other) =
    default;

PrefetchArchiveInfo::~PrefetchArchiveInfo() = default;

bool PrefetchArchiveInfo::empty() const {
  return offline_id == 0;
}

std::ostream& operator<<(std::ostream& out,
                         PrefetchBackgroundTaskRescheduleType value) {
  return out << PrefetchEnumToString(value);
}
std::ostream& operator<<(std::ostream& out, PrefetchRequestStatus value) {
  return out << PrefetchEnumToString(value);
}
std::ostream& operator<<(std::ostream& out, RenderStatus value) {
  return out << PrefetchEnumToString(value);
}
std::ostream& operator<<(std::ostream& out, const PrefetchItemState& value) {
  return out << PrefetchEnumToString(value);
}
std::ostream& operator<<(std::ostream& out, PrefetchItemErrorCode value) {
  return out << PrefetchEnumToString(value);
}

}  // namespace offline_pages
