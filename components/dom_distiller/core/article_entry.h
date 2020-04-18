// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CORE_ARTICLE_ENTRY_H_
#define COMPONENTS_DOM_DISTILLER_CORE_ARTICLE_ENTRY_H_

#include <string>

#include "components/dom_distiller/core/proto/distilled_article.pb.h"
#include "components/sync/model/sync_data.h"
#include "components/sync/protocol/article_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {
class SyncChange;
}

namespace dom_distiller {

typedef sync_pb::ArticleSpecifics ArticleEntry;
typedef sync_pb::ArticlePage ArticleEntryPage;

// A valid entry has an entry_id and all its pages have a URL.
bool IsEntryValid(const ArticleEntry& entry);

bool AreEntriesEqual(const ArticleEntry& left, const ArticleEntry& right);

sync_pb::EntitySpecifics SpecificsFromEntry(const ArticleEntry& entry);
ArticleEntry EntryFromSpecifics(const sync_pb::EntitySpecifics& specifics);

ArticleEntry GetEntryFromChange(const syncer::SyncChange& change);
std::string GetEntryIdFromSyncData(const syncer::SyncData& data);
syncer::SyncData CreateLocalData(const ArticleEntry& entry);

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CORE_ARTICLE_ENTRY_H_
