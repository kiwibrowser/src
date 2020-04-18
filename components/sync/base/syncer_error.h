// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_SYNCER_ERROR_H_
#define COMPONENTS_SYNC_BASE_SYNCER_ERROR_H_

namespace syncer {

// This enum describes all the possible results of a sync cycle.
enum SyncerError {
  UNSET = 0,       // Default value.
  CANNOT_DO_WORK,  // A model worker could not process a work item.

  NETWORK_CONNECTION_UNAVAILABLE,  // Connectivity failure.
  NETWORK_IO_ERROR,                // Response buffer read error.
  SYNC_SERVER_ERROR,               // Non auth HTTP error.
  SYNC_AUTH_ERROR,                 // HTTP auth error.

  // Based on values returned by server.  Most are defined in sync.proto.
  SERVER_RETURN_INVALID_CREDENTIAL,
  SERVER_RETURN_UNKNOWN_ERROR,
  SERVER_RETURN_THROTTLED,
  SERVER_RETURN_TRANSIENT_ERROR,
  SERVER_RETURN_MIGRATION_DONE,
  SERVER_RETURN_CLEAR_PENDING,
  SERVER_RETURN_NOT_MY_BIRTHDAY,
  SERVER_RETURN_CONFLICT,
  SERVER_RESPONSE_VALIDATION_FAILED,
  SERVER_RETURN_DISABLED_BY_ADMIN,
  SERVER_RETURN_USER_ROLLBACK,
  SERVER_RETURN_PARTIAL_FAILURE,
  SERVER_RETURN_CLIENT_DATA_OBSOLETE,

  // A datatype decided the sync cycle needed to be performed again.
  DATATYPE_TRIGGERED_RETRY,

  SERVER_MORE_TO_DOWNLOAD,

  SYNCER_OK
};

const char* GetSyncerErrorString(SyncerError);

// Helper to check that |error| is set to something (not UNSET) and is not
// SYNCER_OK.
bool SyncerErrorIsError(SyncerError error);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_SYNCER_ERROR_H_
