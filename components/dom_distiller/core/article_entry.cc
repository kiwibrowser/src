// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/article_entry.h"

#include "base/logging.h"
#include "components/sync/model/sync_change.h"

using sync_pb::EntitySpecifics;
using sync_pb::ArticlePage;
using sync_pb::ArticleSpecifics;

namespace dom_distiller {

bool IsEntryPageValid(const ArticleEntryPage& page) { return page.has_url(); }

bool IsEntryValid(const ArticleEntry& entry) {
  if (!entry.has_entry_id())
    return false;
  for (int i = 0; i < entry.pages_size(); ++i) {
    if (!IsEntryPageValid(entry.pages(i)))
      return false;
  }
  return true;
}

bool AreEntriesEqual(const ArticleEntry& left, const ArticleEntry& right) {
  DCHECK(IsEntryValid(left));
  DCHECK(IsEntryValid(right));
  return left.SerializeAsString() == right.SerializeAsString();
}

ArticleEntry EntryFromSpecifics(const EntitySpecifics& specifics) {
  DCHECK(specifics.has_article());
  const ArticleSpecifics& article_specifics = specifics.article();
  ArticleEntry entry = article_specifics;
  DCHECK(IsEntryValid(entry));
  return entry;
}

EntitySpecifics SpecificsFromEntry(const ArticleEntry& entry) {
  DCHECK(IsEntryValid(entry));
  EntitySpecifics specifics;
  *specifics.mutable_article() = entry;
  return specifics;
}

ArticleEntry GetEntryFromChange(const syncer::SyncChange& change) {
  DCHECK(change.IsValid());
  DCHECK(change.sync_data().IsValid());
  return EntryFromSpecifics(change.sync_data().GetSpecifics());
}

std::string GetEntryIdFromSyncData(const syncer::SyncData& data) {
  const EntitySpecifics& entity = data.GetSpecifics();
  DCHECK(entity.has_article());
  const ArticleSpecifics& specifics = entity.article();
  DCHECK(specifics.has_entry_id());
  return specifics.entry_id();
}

syncer::SyncData CreateLocalData(const ArticleEntry& entry) {
  EntitySpecifics specifics = SpecificsFromEntry(entry);
  const std::string& entry_id = entry.entry_id();
  return syncer::SyncData::CreateLocalData(entry_id, entry_id, specifics);
}

}  // namespace dom_distiller
