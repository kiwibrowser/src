// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_BATTOR_ERROR_H_
#define TOOLS_BATTOR_AGENT_BATTOR_ERROR_H_

#include <string>

namespace battor {

// A BattOrError is an error that occurs when communicating with a BattOr.
enum BattOrError {
  BATTOR_ERROR_NONE,
  BATTOR_ERROR_CONNECTION_FAILED,
  BATTOR_ERROR_TIMEOUT,
  BATTOR_ERROR_SEND_ERROR,
  BATTOR_ERROR_RECEIVE_ERROR,
  BATTOR_ERROR_UNEXPECTED_MESSAGE,
  BATTOR_ERROR_TOO_MANY_COMMAND_RETRIES,
  BATTOR_ERROR_TOO_MANY_FRAME_RETRIES,
  BATTOR_ERROR_FILE_NOT_FOUND,
};

std::string BattOrErrorToString(BattOrError error);

}

#endif  // TOOLS_BATTOR_AGENT_BATTOR_ERROR_H_
