// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/util.h"

#include "base/logging.h"
#include "chromeos/binder/binder_driver_api.h"

namespace binder {

const char* CommandToString(uint32_t command) {
  switch (command) {
    case BR_ERROR:
      return "BR_ERROR";
    case BR_OK:
      return "BR_OK";
    case BR_TRANSACTION:
      return "BR_TRANSACTION";
    case BR_REPLY:
      return "BR_REPLY";
    case BR_ACQUIRE_RESULT:
      return "BR_ACQUIRE_RESULT";
    case BR_DEAD_REPLY:
      return "BR_DEAD_REPLY";
    case BR_TRANSACTION_COMPLETE:
      return "BR_TRANSACTION_COMPLETE";
    case BR_INCREFS:
      return "BR_INCREFS";
    case BR_ACQUIRE:
      return "BR_ACQUIRE";
    case BR_RELEASE:
      return "BR_RELEASE";
    case BR_DECREFS:
      return "BR_DECREFS";
    case BR_ATTEMPT_ACQUIRE:
      return "BR_ATTEMPT_ACQUIRE";
    case BR_NOOP:
      return "BR_NOOP";
    case BR_SPAWN_LOOPER:
      return "BR_SPAWN_LOOPER";
    case BR_FINISHED:
      return "BR_FINISHED";
    case BR_DEAD_BINDER:
      return "BR_DEAD_BINDER";
    case BR_CLEAR_DEATH_NOTIFICATION_DONE:
      return "BR_CLEAR_DEATH_NOTIFICATION_DONE";
    case BR_FAILED_REPLY:
      return "BR_FAILED_REPLY";
    case BC_TRANSACTION:
      return "BC_TRANSACTION";
    case BC_REPLY:
      return "BC_REPLY";
    case BC_ACQUIRE_RESULT:
      return "BC_ACQUIRE_RESULT";
    case BC_FREE_BUFFER:
      return "BC_FREE_BUFFER";
    case BC_INCREFS:
      return "BC_INCREFS";
    case BC_ACQUIRE:
      return "BC_ACQUIRE";
    case BC_RELEASE:
      return "BC_RELEASE";
    case BC_DECREFS:
      return "BC_DECREFS";
    case BC_INCREFS_DONE:
      return "BC_INCREFS_DONE";
    case BC_ACQUIRE_DONE:
      return "BC_ACQUIRE_DONE";
    case BC_ATTEMPT_ACQUIRE:
      return "BC_ATTEMPT_ACQUIRE";
    case BC_REGISTER_LOOPER:
      return "BC_REGISTER_LOOPER";
    case BC_ENTER_LOOPER:
      return "BC_ENTER_LOOPER";
    case BC_EXIT_LOOPER:
      return "BC_EXIT_LOOPER";
    case BC_REQUEST_DEATH_NOTIFICATION:
      return "BC_REQUEST_DEATH_NOTIFICATION";
    case BC_CLEAR_DEATH_NOTIFICATION:
      return "BC_CLEAR_DEATH_NOTIFICATION";
    case BC_DEAD_BINDER_DONE:
      return "BC_DEAD_BINDER_DONE";
  }
  LOG(ERROR) << "Unknown command: " << command;
  return "UNKNOWN";
}

}  // namespace binder
