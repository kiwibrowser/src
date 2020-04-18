// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/request_coordinator_event_logger.h"

namespace offline_pages {

namespace {

static std::string OfflinerRequestStatusToString(
    Offliner::RequestStatus request_status) {
  switch (request_status) {
    case Offliner::UNKNOWN:
      return "UNKNOWN";
    case Offliner::LOADED:
      return "LOADED";
    case Offliner::SAVED:
      return "SAVED";
    case Offliner::REQUEST_COORDINATOR_CANCELED:
      return "REQUEST_COORDINATOR_CANCELED";
    case Offliner::LOADING_CANCELED:
      return "LOADING_CANCELED";
    case Offliner::LOADING_FAILED:
      return "LOADING_FAILED";
    case Offliner::SAVE_FAILED:
      return "SAVE_FAILED";
    case Offliner::FOREGROUND_CANCELED:
      return "FOREGROUND_CANCELED";
    case Offliner::REQUEST_COORDINATOR_TIMED_OUT:
      return "REQUEST_COORDINATOR_TIMED_OUT";
    case Offliner::DEPRECATED_LOADING_NOT_STARTED:
      return "DEPRECATED_LOADING_NOT_STARTED";
    case Offliner::LOADING_FAILED_NO_RETRY:
      return "LOADING_FAILED_NO_RETRY";
    case Offliner::LOADING_FAILED_NO_NEXT:
      return "LOADING_FAILED_NO_NEXT";
    case Offliner::LOADING_NOT_ACCEPTED:
      return "LOADING_NOT_ACCEPTED";
    case Offliner::QUEUE_UPDATE_FAILED:
      return "QUEUE_UPDATE_FAILED";
    case Offliner::BACKGROUND_SCHEDULER_CANCELED:
      return "BACKGROUND_SCHEDULER_CANCELED";
    case Offliner::SAVED_ON_LAST_RETRY:
      return "SAVED_ON_LAST_RETRY";
    case Offliner::BROWSER_KILLED:
      return "BROWSER_KILLED";
    case Offliner::LOADING_FAILED_DOWNLOAD:
      return "LOADING_FAILED_DOWNLOAD";
    case Offliner::DOWNLOAD_THROTTLED:
      return "DOWNLOAD_THROTTLED";
    case Offliner::LOADING_FAILED_NET_ERROR:
      return "LOADING_FAILED_NET_ERROR";
    case Offliner::LOADING_FAILED_HTTP_ERROR:
      return "LOADING_FAILED_HTTP_ERROR";
    default:
      NOTREACHED();
      return std::to_string(static_cast<int>(request_status));
  }
}

static std::string BackgroundSavePageResultToString(
    RequestNotifier::BackgroundSavePageResult result) {
  switch (result) {
    case RequestNotifier::BackgroundSavePageResult::SUCCESS:
      return "SUCCESS";
    case RequestNotifier::BackgroundSavePageResult::LOADING_FAILURE:
      return "LOADING_FAILURE";
    case RequestNotifier::BackgroundSavePageResult::LOADING_CANCELED:
      return "LOADING_CANCELED";
    case RequestNotifier::BackgroundSavePageResult::FOREGROUND_CANCELED:
      return "FOREGROUND_CANCELED";
    case RequestNotifier::BackgroundSavePageResult::SAVE_FAILED:
      return "SAVE_FAILED";
    case RequestNotifier::BackgroundSavePageResult::EXPIRED:
      return "EXPIRED";
    case RequestNotifier::BackgroundSavePageResult::RETRY_COUNT_EXCEEDED:
      return "RETRY_COUNT_EXCEEDED";
    case RequestNotifier::BackgroundSavePageResult::START_COUNT_EXCEEDED:
      return "START_COUNT_EXCEEDED";
    case RequestNotifier::BackgroundSavePageResult::USER_CANCELED:
      return "REMOVED";
    case RequestNotifier::BackgroundSavePageResult::DOWNLOAD_THROTTLED:
      return "DOWNLOAD_THROTTLED";
    default:
      NOTREACHED();
      return std::to_string(static_cast<int>(result));
  }
}

static std::string UpdateRequestResultToString(UpdateRequestResult result) {
  switch (result) {
    case UpdateRequestResult::SUCCESS:
      return "SUCCESS";
    case UpdateRequestResult::STORE_FAILURE:
      return "STORE_FAILURE";
    case UpdateRequestResult::REQUEST_DOES_NOT_EXIST:
      return "REQUEST_DOES_NOT_EXIST";
    default:
      NOTREACHED();
      return std::to_string(static_cast<int>(result));
  }
}

}  // namespace

void RequestCoordinatorEventLogger::RecordOfflinerResult(
    const std::string& name_space,
    Offliner::RequestStatus new_status,
    int64_t request_id) {
  std::string request_id_str = std::to_string(request_id);
  RecordActivity("Background save attempt for " + name_space + ":" +
                 request_id_str + " - " +
                 OfflinerRequestStatusToString(new_status));
}

void RequestCoordinatorEventLogger::RecordDroppedSavePageRequest(
    const std::string& name_space,
    RequestNotifier::BackgroundSavePageResult result,
    int64_t request_id) {
  std::string request_id_str = std::to_string(request_id);
  RecordActivity("Background save request removed " + name_space + ":" +
                 request_id_str + " - " +
                 BackgroundSavePageResultToString(result));
}

void RequestCoordinatorEventLogger::RecordUpdateRequestFailed(
    const std::string& name_space,
    UpdateRequestResult result) {
  RecordActivity("Updating queued request for " + name_space + " failed - " +
                 UpdateRequestResultToString(result));
}

}  // namespace offline_pages
