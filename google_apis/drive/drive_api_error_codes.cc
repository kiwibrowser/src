// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/drive/drive_api_error_codes.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace google_apis {

std::string DriveApiErrorCodeToString(DriveApiErrorCode error) {
  switch (error) {
    case HTTP_SUCCESS:
      return "HTTP_SUCCESS";

    case HTTP_CREATED:
      return "HTTP_CREATED";

    case HTTP_NO_CONTENT:
      return "HTTP_NO_CONTENT";

    case HTTP_FOUND:
      return "HTTP_FOUND";

    case HTTP_NOT_MODIFIED:
      return "HTTP_NOT_MODIFIED";

    case HTTP_RESUME_INCOMPLETE:
      return "HTTP_RESUME_INCOMPLETE";

    case HTTP_BAD_REQUEST:
      return "HTTP_BAD_REQUEST";

    case HTTP_UNAUTHORIZED:
      return "HTTP_UNAUTHORIZED";

    case HTTP_FORBIDDEN:
      return "HTTP_FORBIDDEN";

    case HTTP_NOT_FOUND:
      return "HTTP_NOT_FOUND";

    case HTTP_CONFLICT:
      return "HTTP_CONFLICT";

    case HTTP_GONE:
      return "HTTP_GONE";

    case HTTP_LENGTH_REQUIRED:
      return "HTTP_LENGTH_REQUIRED";

    case HTTP_PRECONDITION:
      return "HTTP_PRECONDITION";

    case HTTP_INTERNAL_SERVER_ERROR:
      return "HTTP_INTERNAL_SERVER_ERROR";

    case HTTP_NOT_IMPLEMENTED:
      return "HTTP_NOT_IMPLEMENTED";

    case HTTP_BAD_GATEWAY:
      return "HTTP_BAD_GATEWAY";

    case HTTP_SERVICE_UNAVAILABLE:
      return "HTTP_SERVICE_UNAVAILABLE";

    case DRIVE_PARSE_ERROR:
      return "DRIVE_PARSE_ERROR";

    case DRIVE_FILE_ERROR:
      return "DRIVE_FILE_ERROR";

    case DRIVE_CANCELLED:
      return "DRIVE_CANCELLED";

    case DRIVE_OTHER_ERROR:
      return "DRIVE_OTHER_ERROR";

    case DRIVE_NO_CONNECTION:
      return "DRIVE_NO_CONNECTION";

    case DRIVE_NOT_READY:
      return "DRIVE_NOT_READY";

    case DRIVE_NO_SPACE:
      return "DRIVE_NO_SPACE";

    case DRIVE_RESPONSE_TOO_LARGE:
      return "DRIVE_RESPONSE_TOO_LARGE";
  }

  return "UNKNOWN_ERROR_" + base::IntToString(error);
}

bool IsSuccessfulDriveApiErrorCode(DriveApiErrorCode error) {
  return 200 <= error && error <= 299;
}

}  // namespace google_apis
