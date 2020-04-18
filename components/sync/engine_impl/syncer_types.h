// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_SYNCER_TYPES_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_SYNCER_TYPES_H_

// The intent of this is to keep all shared data types and enums for the syncer
// in a single place without having dependencies between other files.
namespace syncer {

enum UpdateAttemptResponse {
  // Update was applied or safely ignored.
  SUCCESS,

  // The conditions described by the following enum values are not mutually
  // exclusive.  The list has been ordered according to priority in the case of
  // overlap, with highest priority first.
  //
  // For example, in the case where an item had both the IS_UNSYCNED and
  // IS_UNAPPLIED_UPDATE flags set (CONFLICT_SIMPLE), and a SERVER_PARENT_ID
  // that, if applied, would cause a directory loop (CONFLICT_HIERARCHY), and
  // specifics that we can't decrypt right now (CONFLICT_ENCRYPTION), the
  // UpdateApplicator would return CONFLICT_ENCRYPTION when attempting to
  // process the item.
  //
  // We do not attempt to resolve CONFLICT_HIERARCHY or CONFLICT_ENCRYPTION
  // items.  We will leave these updates unapplied and wait for the server
  // to send us newer updates that will resolve the conflict.

  // We were unable to decrypt/encrypt this server data. As such, we can't make
  // forward progress on this node, but because the passphrase may not arrive
  // until later we don't want to get the syncer stuck. See UpdateApplicator
  // for how this is handled.
  CONFLICT_ENCRYPTION,

  // These are some updates that, if applied, would violate the tree's
  // invariants.  Examples of this include the server adding children to locally
  // deleted directories and directory moves that would create loops.
  CONFLICT_HIERARCHY,

  // This indicates that item was modified both remotely (IS_UNAPPLIED_UPDATE)
  // and locally (IS_UNSYNCED).  We use the ConflictResolver to decide which of
  // the changes should take priority, or if we can possibly merge the data.
  CONFLICT_SIMPLE
};

// Different results from the verify phase will yield different methods of
// processing in the ProcessUpdates phase. The SKIP result means the entry
// doesn't go to the ProcessUpdates phase.
enum VerifyResult {
  VERIFY_FAIL,
  VERIFY_SUCCESS,
  VERIFY_UNDELETE,
  VERIFY_SKIP,
  VERIFY_UNDECIDED
};

enum VerifyCommitResult {
  VERIFY_UNSYNCABLE,
  VERIFY_OK,
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_SYNCER_TYPES_H_
