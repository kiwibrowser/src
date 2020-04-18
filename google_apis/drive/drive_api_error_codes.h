// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_DRIVE_DRIVE_API_ERROR_CODES_H_
#define GOOGLE_APIS_DRIVE_DRIVE_API_ERROR_CODES_H_

#include <string>

namespace google_apis {

// HTTP errors that can be returned by Drive API service. Keep all the values
// positive, as they are used for UMA histograms.
enum DriveApiErrorCode {
  HTTP_SUCCESS               = 200,
  HTTP_CREATED               = 201,
  HTTP_NO_CONTENT            = 204,
  HTTP_FOUND                 = 302,
  HTTP_NOT_MODIFIED          = 304,
  HTTP_RESUME_INCOMPLETE     = 308,
  HTTP_BAD_REQUEST           = 400,
  HTTP_UNAUTHORIZED          = 401,
  HTTP_FORBIDDEN             = 403,
  HTTP_NOT_FOUND             = 404,
  HTTP_CONFLICT              = 409,
  HTTP_GONE                  = 410,
  HTTP_LENGTH_REQUIRED       = 411,
  HTTP_PRECONDITION          = 412,
  HTTP_INTERNAL_SERVER_ERROR = 500,
  HTTP_NOT_IMPLEMENTED       = 501,
  HTTP_BAD_GATEWAY           = 502,
  HTTP_SERVICE_UNAVAILABLE   = 503,
  DRIVE_PARSE_ERROR          = 1000,
  DRIVE_FILE_ERROR           = 1001,
  DRIVE_CANCELLED            = 1002,
  DRIVE_OTHER_ERROR          = 1003,
  DRIVE_NO_CONNECTION        = 1004,
  DRIVE_NOT_READY            = 1005,
  DRIVE_NO_SPACE             = 1006,
  DRIVE_RESPONSE_TOO_LARGE   = 1007
  // If modified, update the enum mapping in histograms.xml.
};

// Returns a string representation of DriveApiErrorCode.
std::string DriveApiErrorCodeToString(DriveApiErrorCode error);

// Checks if the error code represents success.
bool IsSuccessfulDriveApiErrorCode(DriveApiErrorCode error);

}  // namespace google_apis

#endif  // GOOGLE_APIS_DRIVE_DRIVE_API_ERROR_CODES_H_
