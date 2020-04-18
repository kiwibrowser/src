// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/syncer_error.h"

#include "base/logging.h"

namespace syncer {

#define ENUM_CASE(x) \
  case x:            \
    return #x;       \
    break;
const char* GetSyncerErrorString(SyncerError value) {
  switch (value) {
    ENUM_CASE(UNSET);
    ENUM_CASE(CANNOT_DO_WORK);
    ENUM_CASE(NETWORK_CONNECTION_UNAVAILABLE);
    ENUM_CASE(NETWORK_IO_ERROR);
    ENUM_CASE(SYNC_SERVER_ERROR);
    ENUM_CASE(SYNC_AUTH_ERROR);
    ENUM_CASE(SERVER_RETURN_INVALID_CREDENTIAL);
    ENUM_CASE(SERVER_RETURN_UNKNOWN_ERROR);
    ENUM_CASE(SERVER_RETURN_THROTTLED);
    ENUM_CASE(SERVER_RETURN_TRANSIENT_ERROR);
    ENUM_CASE(SERVER_RETURN_MIGRATION_DONE);
    ENUM_CASE(SERVER_RETURN_CLEAR_PENDING);
    ENUM_CASE(SERVER_RETURN_NOT_MY_BIRTHDAY);
    ENUM_CASE(SERVER_RETURN_CONFLICT);
    ENUM_CASE(SERVER_RESPONSE_VALIDATION_FAILED);
    ENUM_CASE(SERVER_RETURN_DISABLED_BY_ADMIN);
    ENUM_CASE(SERVER_RETURN_USER_ROLLBACK);
    ENUM_CASE(SERVER_RETURN_PARTIAL_FAILURE);
    ENUM_CASE(SERVER_RETURN_CLIENT_DATA_OBSOLETE);
    ENUM_CASE(SERVER_MORE_TO_DOWNLOAD);
    ENUM_CASE(DATATYPE_TRIGGERED_RETRY);
    ENUM_CASE(SYNCER_OK);
  }
  NOTREACHED();
  return "INVALID";
}
#undef ENUM_CASE

bool SyncerErrorIsError(SyncerError error) {
  return error != UNSET && error != SYNCER_OK &&
         error != SERVER_MORE_TO_DOWNLOAD;
}

}  // namespace syncer
