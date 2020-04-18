// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/syncable_delete_journal.h"

#include <utility>

#include "components/sync/syncable/entry_kernel.h"
#include "components/sync/syncable/syncable_id.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace syncable {

// Tests that when adding entries to JournalIndex key pointer always matches
// value object.
TEST(SyncableDeleteJournal, AddEntryToJournalIndex_KeyMatchesValue) {
  JournalIndex journal_index;
  std::unique_ptr<EntryKernel> entry;

  // Add two entries with the same id. Verify that only one entry is stored in
  // JournalIndex and that entry's key matches the value object.
  entry = std::make_unique<EntryKernel>();
  entry->put(ID, Id::CreateFromServerId("id1"));
  DeleteJournal::AddEntryToJournalIndex(&journal_index, std::move(entry));
  EXPECT_EQ(1U, journal_index.size());

  entry = std::make_unique<EntryKernel>();
  entry->put(ID, Id::CreateFromServerId("id1"));
  DeleteJournal::AddEntryToJournalIndex(&journal_index, std::move(entry));
  EXPECT_EQ(1U, journal_index.size());

  for (auto it = journal_index.begin(); it != journal_index.end(); ++it) {
    EXPECT_TRUE(it->first == it->second.get());
  }
}

}  // namespace syncable
}  // namespace syncer
