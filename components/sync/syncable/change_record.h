// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_CHANGE_RECORD_H_
#define COMPONENTS_SYNC_SYNCABLE_CHANGE_RECORD_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/memory/linked_ptr.h"
#include "components/sync/base/immutable.h"
#include "components/sync/protocol/password_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace syncer {

// TODO(zea): One day get passwords playing nicely with the rest of encryption
// and get rid of this.
class ExtraPasswordChangeRecordData {
 public:
  ExtraPasswordChangeRecordData();
  explicit ExtraPasswordChangeRecordData(
      const sync_pb::PasswordSpecificsData& data);
  virtual ~ExtraPasswordChangeRecordData();

  virtual std::unique_ptr<base::DictionaryValue> ToValue() const;

  const sync_pb::PasswordSpecificsData& unencrypted() const;

 private:
  sync_pb::PasswordSpecificsData unencrypted_;
};

// ChangeRecord indicates a single item that changed as a result of a sync
// operation.  This gives the sync id of the node that changed, and the type
// of change.  To get the actual property values after an ADD or UPDATE, the
// client should get the node with InitByIdLookup(), using the provided id.
struct ChangeRecord {
  enum Action {
    ACTION_ADD,
    ACTION_DELETE,
    ACTION_UPDATE,
  };
  ChangeRecord();
  ChangeRecord(const ChangeRecord& other);
  ~ChangeRecord();

  std::unique_ptr<base::DictionaryValue> ToValue() const;

  int64_t id;
  Action action;
  sync_pb::EntitySpecifics specifics;
  linked_ptr<ExtraPasswordChangeRecordData> extra;
};

using ChangeRecordList = std::vector<ChangeRecord>;

using ImmutableChangeRecordList = Immutable<ChangeRecordList>;

}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_CHANGE_RECORD_H_
