// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/apply_control_data_updates.h"

#include <stdint.h>

#include <vector>

#include "base/metrics/histogram_macros.h"
#include "components/sync/base/cryptographer.h"
#include "components/sync/engine_impl/conflict_resolver.h"
#include "components/sync/engine_impl/conflict_util.h"
#include "components/sync/engine_impl/syncer_util.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/mutable_entry.h"
#include "components/sync/syncable/nigori_handler.h"
#include "components/sync/syncable/nigori_util.h"
#include "components/sync/syncable/syncable_write_transaction.h"

namespace syncer {

void ApplyControlDataUpdates(syncable::Directory* dir) {
  syncable::WriteTransaction trans(FROM_HERE, syncable::SYNCER, dir);

  std::vector<int64_t> handles;
  dir->GetUnappliedUpdateMetaHandles(&trans, ToFullModelTypeSet(ControlTypes()),
                                     &handles);

  // First, go through and manually apply any new top level datatype nodes (so
  // that we don't have to worry about hitting a CONFLICT_HIERARCHY with an
  // entry because we haven't applied its parent yet).
  // TODO(sync): if at some point we support control datatypes with actual
  // hierarchies we'll need to revisit this logic.
  ModelTypeSet control_types = ControlTypes();
  for (ModelTypeSet::Iterator iter = control_types.First(); iter.Good();
       iter.Inc()) {
    ModelType type = iter.Get();
    syncable::MutableEntry entry(&trans, syncable::GET_TYPE_ROOT, type);
    if (!entry.good())
      continue;

    if (!entry.GetIsUnappliedUpdate()) {
      // If this is a type with client generated root, the root node has been
      // created locally and might never be updated by the server. In that case
      // it has to be marked as having the initial download completed (which is
      // done by changing the root's base version to a value other than
      // CHANGES_VERSION). This does nothing if the root's base version is
      // already other than CHANGES_VERSION.
      if (IsTypeWithClientGeneratedRoot(type)) {
        dir->MarkInitialSyncEndedForType(&trans, type);
      }
      continue;
    }

    DCHECK_EQ(type, entry.GetServerModelType());
    if (type == NIGORI) {
      // Nigori node applications never fail.
      ApplyNigoriUpdate(&trans, &entry, dir->GetCryptographer(&trans));
    } else {
      ApplyControlUpdate(&trans, &entry, dir->GetCryptographer(&trans));
    }
  }

  // Go through the rest of the unapplied control updates, skipping over any
  // top level folders.
  for (std::vector<int64_t>::const_iterator iter = handles.begin();
       iter != handles.end(); ++iter) {
    syncable::MutableEntry entry(&trans, syncable::GET_BY_HANDLE, *iter);
    DCHECK(entry.good());
    ModelType type = entry.GetServerModelType();
    DCHECK(ControlTypes().Has(type));
    if (!entry.GetUniqueServerTag().empty()) {
      // We should have already applied all top level control nodes.
      DCHECK(!entry.GetIsUnappliedUpdate());
      continue;
    }

    ApplyControlUpdate(&trans, &entry, dir->GetCryptographer(&trans));
  }
}

// Update the nigori handler with the server's nigori node.
//
// If we have a locally modified nigori node, we merge them manually. This
// handles the case where two clients both set a different passphrase. The
// second client to attempt to commit will go into a state of having pending
// keys, unioned the set of encrypted types, and eventually re-encrypt
// everything with the passphrase of the first client and commit the set of
// merged encryption keys. Until the second client provides the pending
// passphrase, the cryptographer will preserve the encryption keys based on the
// local passphrase, while the nigori node will preserve the server encryption
// keys.
void ApplyNigoriUpdate(syncable::WriteTransaction* const trans,
                       syncable::MutableEntry* const entry,
                       Cryptographer* cryptographer) {
  DCHECK(entry->GetIsUnappliedUpdate());

  // We apply the nigori update regardless of whether there's a conflict or
  // not in order to preserve any new encrypted types or encryption keys.
  // TODO(zea): consider having this return a bool reflecting whether it was a
  // valid update or not, and in the case of invalid updates not overwrite the
  // local data.
  const sync_pb::NigoriSpecifics& nigori = entry->GetServerSpecifics().nigori();
  trans->directory()->GetNigoriHandler()->ApplyNigoriUpdate(nigori, trans);

  // Make sure any unsynced changes are properly encrypted as necessary.
  // We only perform this if the cryptographer is ready. If not, these are
  // re-encrypted at SetDecryptionPassphrase time (via ReEncryptEverything).
  // This logic covers the case where the nigori update marked new datatypes
  // for encryption, but didn't change the passphrase.
  if (cryptographer->is_ready()) {
    // Note that we don't bother to encrypt any data for which IS_UNSYNCED
    // == false here. The machine that turned on encryption should know about
    // and re-encrypt all synced data. It's possible it could get interrupted
    // during this process, but we currently reencrypt everything at startup
    // as well, so as soon as a client is restarted with this datatype marked
    // for encryption, all the data should be updated as necessary.

    // If this fails, something is wrong with the cryptographer, but there's
    // nothing we can do about it here.
    DVLOG(1) << "Received new nigori, encrypting unsynced changes.";
    syncable::ProcessUnsyncedChangesForEncryption(trans);
  }

  if (!entry->GetIsUnsynced()) {  // Update only.
    UpdateLocalDataFromServerData(trans, entry);
  } else {  // Conflict.
    const sync_pb::EntitySpecifics& server_specifics =
        entry->GetServerSpecifics();
    const sync_pb::NigoriSpecifics& server_nigori = server_specifics.nigori();
    const sync_pb::EntitySpecifics& local_specifics = entry->GetSpecifics();
    const sync_pb::NigoriSpecifics& local_nigori = local_specifics.nigori();

    // We initialize the new nigori with the server state, and will override
    // it as necessary below.
    sync_pb::EntitySpecifics new_specifics = entry->GetServerSpecifics();
    sync_pb::NigoriSpecifics* new_nigori = new_specifics.mutable_nigori();

    // If the cryptographer is not ready, another client set a new encryption
    // passphrase. If we had migrated locally, we will re-migrate when the
    // pending keys are provided. If we had set a new custom passphrase locally
    // the user will have another chance to set a custom passphrase later
    // (assuming they hadn't set a custom passphrase on the other client).
    // Therefore, we only attempt to merge the nigori nodes if the cryptographer
    // is ready.
    // Note: we only update the encryption keybag if we're sure that we aren't
    // invalidating the keystore_decryptor_token (i.e. we're either
    // not migrated or we copying over all local state).
    if (cryptographer->is_ready()) {
      if (local_nigori.has_passphrase_type() &&
          server_nigori.has_passphrase_type()) {
        // They're both migrated, preserve the local nigori if the passphrase
        // type is more conservative.
        if (server_nigori.passphrase_type() ==
                sync_pb::NigoriSpecifics::KEYSTORE_PASSPHRASE &&
            local_nigori.passphrase_type() !=
                sync_pb::NigoriSpecifics::KEYSTORE_PASSPHRASE) {
          DCHECK(local_nigori.passphrase_type() ==
                     sync_pb::NigoriSpecifics::FROZEN_IMPLICIT_PASSPHRASE ||
                 local_nigori.passphrase_type() ==
                     sync_pb::NigoriSpecifics::CUSTOM_PASSPHRASE);
          new_nigori->CopyFrom(local_nigori);
          cryptographer->GetKeys(new_nigori->mutable_encryption_keybag());
        }
      } else if (!local_nigori.has_passphrase_type() &&
                 !server_nigori.has_passphrase_type()) {
        // Set the explicit passphrase based on the local state. If the server
        // had set an explict passphrase, we should have pending keys, so
        // should not reach this code.
        // Because neither side is migrated, we don't have to worry about the
        // keystore decryptor token.
        new_nigori->set_keybag_is_frozen(local_nigori.keybag_is_frozen());
        cryptographer->GetKeys(new_nigori->mutable_encryption_keybag());
      } else if (local_nigori.has_passphrase_type()) {
        // Local is migrated but server is not. Copy over the local migrated
        // data.
        new_nigori->CopyFrom(local_nigori);
        cryptographer->GetKeys(new_nigori->mutable_encryption_keybag());
      }  // else leave the new nigori with the server state.
    }

    // Always update to the safest set of encrypted types.
    trans->directory()->GetNigoriHandler()->UpdateNigoriFromEncryptedTypes(
        new_nigori, trans);

    entry->PutSpecifics(new_specifics);
    DVLOG(1) << "Resolving simple conflict, merging nigori nodes: " << entry;

    conflict_util::OverwriteServerChanges(entry);

    UMA_HISTOGRAM_ENUMERATION("Sync.ResolveSimpleConflict",
                              ConflictResolver::NIGORI_MERGE,
                              ConflictResolver::CONFLICT_RESOLUTION_SIZE);
  }
}

void ApplyControlUpdate(syncable::WriteTransaction* const trans,
                        syncable::MutableEntry* const entry,
                        Cryptographer* cryptographer) {
  DCHECK_NE(entry->GetServerModelType(), NIGORI);
  DCHECK(entry->GetIsUnappliedUpdate());
  if (entry->GetIsUnsynced()) {
    // We just let the server win all conflicts with control types.
    DVLOG(1) << "Ignoring local changes for control update.";
    conflict_util::IgnoreLocalChanges(entry);
    UMA_HISTOGRAM_ENUMERATION("Sync.ResolveSimpleConflict",
                              ConflictResolver::OVERWRITE_LOCAL,
                              ConflictResolver::CONFLICT_RESOLUTION_SIZE);
  }

  UpdateAttemptResponse response =
      AttemptToUpdateEntry(trans, entry, cryptographer);
  DCHECK_EQ(SUCCESS, response);
}

}  // namespace syncer
