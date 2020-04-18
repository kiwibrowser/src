// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/write_transaction.h"

#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/mutable_entry.h"
#include "components/sync/syncable/syncable_write_transaction.h"

namespace syncer {

//////////////////////////////////////////////////////////////////////////
// WriteTransaction member definitions
WriteTransaction::WriteTransaction(const base::Location& from_here,
                                   UserShare* share)
    : BaseTransaction(share), transaction_(nullptr) {
  transaction_ = new syncable::WriteTransaction(from_here, syncable::SYNCAPI,
                                                share->directory.get());
}

WriteTransaction::WriteTransaction(const base::Location& from_here,
                                   UserShare* share,
                                   int64_t* new_model_version)
    : BaseTransaction(share), transaction_(nullptr) {
  transaction_ = new syncable::WriteTransaction(
      from_here, share->directory.get(), new_model_version);
}

WriteTransaction::~WriteTransaction() {
  delete transaction_;
}

syncable::BaseTransaction* WriteTransaction::GetWrappedTrans() const {
  return transaction_;
}

void WriteTransaction::SetDataTypeContext(
    ModelType type,
    SyncChangeProcessor::ContextRefreshStatus refresh_status,
    const std::string& context) {
  DCHECK(ProtocolTypes().Has(type));
  int field_number = GetSpecificsFieldNumberFromModelType(type);
  sync_pb::DataTypeContext local_context;
  GetDirectory()->GetDataTypeContext(transaction_, type, &local_context);
  if (local_context.context() == context)
    return;

  if (!local_context.has_data_type_id())
    local_context.set_data_type_id(field_number);

  DCHECK_EQ(field_number, local_context.data_type_id());
  DCHECK_GE(local_context.version(), 0);
  local_context.set_version(local_context.version() + 1);
  local_context.set_context(context);
  GetDirectory()->SetDataTypeContext(transaction_, type, local_context);
  if (refresh_status == SyncChangeProcessor::REFRESH_NEEDED) {
    DVLOG(1) << "Forcing refresh of type " << ModelTypeToString(type);
    // Clear the progress token from the progress markers. Preserve all other
    // state, in case a GC directive was present.
    sync_pb::DataTypeProgressMarker progress_marker;
    GetDirectory()->GetDownloadProgress(type, &progress_marker);
    progress_marker.clear_token();
    GetDirectory()->SetDownloadProgress(type, progress_marker);

    // Go through and reset the versions for all the synced entities of this
    // data type.
    GetDirectory()->ResetVersionsForType(transaction_, type);
  }

  // Note that it's possible for a GetUpdatesResponse that arrives immediately
  // after the context update to override the cleared progress markers.
  // TODO(zea): add a flag in the directory to prevent this from happening.
  // See crbug.com/360280
}

}  // namespace syncer
