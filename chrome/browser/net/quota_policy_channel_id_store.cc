// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/quota_policy_channel_id_store.h"

#include <list>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "net/cookies/cookie_util.h"
#include "net/extras/sqlite/sqlite_channel_id_store.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "url/gurl.h"

QuotaPolicyChannelIDStore::QuotaPolicyChannelIDStore(
    const base::FilePath& path,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const scoped_refptr<storage::SpecialStoragePolicy>& special_storage_policy)
    : special_storage_policy_(special_storage_policy),
      persistent_store_(
          new net::SQLiteChannelIDStore(path, background_task_runner)) {
  DCHECK(background_task_runner.get());
}

QuotaPolicyChannelIDStore::~QuotaPolicyChannelIDStore() {
  if (!special_storage_policy_.get() ||
      !special_storage_policy_->HasSessionOnlyOrigins()) {
    return;
  }
  std::list<std::string> session_only_server_identifiers;
  for (std::set<std::string>::iterator it = server_identifiers_.begin();
       it != server_identifiers_.end();
       ++it) {
    GURL url(net::cookie_util::CookieOriginToURL(*it, true));
    if (special_storage_policy_->IsStorageSessionOnly(url))
      session_only_server_identifiers.push_back(*it);
  }
  persistent_store_->DeleteAllInList(session_only_server_identifiers);
}

void QuotaPolicyChannelIDStore::Load(const LoadedCallback& loaded_callback) {
  persistent_store_->Load(
      base::Bind(&QuotaPolicyChannelIDStore::OnLoad, this, loaded_callback));
}

void QuotaPolicyChannelIDStore::AddChannelID(
    const net::DefaultChannelIDStore::ChannelID& channel_id) {
  server_identifiers_.insert(channel_id.server_identifier());
  persistent_store_->AddChannelID(channel_id);
}

void QuotaPolicyChannelIDStore::DeleteChannelID(
    const net::DefaultChannelIDStore::ChannelID& channel_id) {
  server_identifiers_.erase(channel_id.server_identifier());
  persistent_store_->DeleteChannelID(channel_id);
}

void QuotaPolicyChannelIDStore::Flush() {
  persistent_store_->Flush();
}

void QuotaPolicyChannelIDStore::SetForceKeepSessionState() {
  special_storage_policy_ = NULL;
}

void QuotaPolicyChannelIDStore::OnLoad(
    const LoadedCallback& loaded_callback,
    std::unique_ptr<ChannelIDVector> channel_ids) {
  for (ChannelIDVector::const_iterator channel_id = channel_ids->begin();
       channel_id != channel_ids->end();
       ++channel_id) {
    server_identifiers_.insert((*channel_id)->server_identifier());
  }
  loaded_callback.Run(std::move(channel_ids));
}
