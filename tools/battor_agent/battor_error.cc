// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/battor_error.h"

#include "base/logging.h"

namespace battor {

std::string BattOrErrorToString(BattOrError error) {
  switch (error) {
    case BATTOR_ERROR_NONE:
      return "NONE";
    case BATTOR_ERROR_CONNECTION_FAILED:
      return "CONNECTION FAILED";
    case BATTOR_ERROR_TIMEOUT:
      return "TIMEOUT";
    case BATTOR_ERROR_SEND_ERROR:
      return "SEND ERROR";
    case BATTOR_ERROR_RECEIVE_ERROR:
      return "RECEIVE ERROR";
    case BATTOR_ERROR_UNEXPECTED_MESSAGE:
      return "UNEXPECTED MESSAGE";
    case BATTOR_ERROR_TOO_MANY_COMMAND_RETRIES:
      return "TOO MANY COMMAND RETRIES";
    case BATTOR_ERROR_TOO_MANY_FRAME_RETRIES:
      return "TOO MANY FRAME RETRIES";
    case BATTOR_ERROR_FILE_NOT_FOUND:
      return "FILE NOT FOUND";
  }

  NOTREACHED();
  return std::string();
}

}  // namespace battor
