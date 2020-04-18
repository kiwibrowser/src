// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/in_memory_tab_restore_service.h"

#include <utility>
#include <vector>

#include "base/compiler_specific.h"

namespace sessions {

InMemoryTabRestoreService::InMemoryTabRestoreService(
    std::unique_ptr<TabRestoreServiceClient> client,
    TabRestoreService::TimeFactory* time_factory)
    : client_(std::move(client)),
      helper_(this, NULL, client_.get(), time_factory) {}

InMemoryTabRestoreService::~InMemoryTabRestoreService() {}

void InMemoryTabRestoreService::AddObserver(
    TabRestoreServiceObserver* observer) {
  helper_.AddObserver(observer);
}

void InMemoryTabRestoreService::RemoveObserver(
    TabRestoreServiceObserver* observer) {
  helper_.RemoveObserver(observer);
}

void InMemoryTabRestoreService::CreateHistoricalTab(LiveTab* live_tab,
                                                    int index) {
  helper_.CreateHistoricalTab(live_tab, index);
}

void InMemoryTabRestoreService::BrowserClosing(LiveTabContext* context) {
  helper_.BrowserClosing(context);
}

void InMemoryTabRestoreService::BrowserClosed(LiveTabContext* context) {
  helper_.BrowserClosed(context);
}

void InMemoryTabRestoreService::ClearEntries() {
  helper_.ClearEntries();
}

void InMemoryTabRestoreService::DeleteNavigationEntries(
    const DeletionPredicate& predicate) {
  helper_.DeleteNavigationEntries(predicate);
}

const TabRestoreService::Entries& InMemoryTabRestoreService::entries() const {
  return helper_.entries();
}

std::vector<LiveTab*> InMemoryTabRestoreService::RestoreMostRecentEntry(
    LiveTabContext* context) {
  return helper_.RestoreMostRecentEntry(context);
}

std::unique_ptr<TabRestoreService::Tab>
InMemoryTabRestoreService::RemoveTabEntryById(SessionID id) {
  return helper_.RemoveTabEntryById(id);
}

std::vector<LiveTab*> InMemoryTabRestoreService::RestoreEntryById(
    LiveTabContext* context,
    SessionID id,
    WindowOpenDisposition disposition) {
  return helper_.RestoreEntryById(context, id, disposition);
}

void InMemoryTabRestoreService::LoadTabsFromLastSession() {
  // Do nothing. This relies on tab persistence which is implemented in Java on
  // the application side on Android.
}

bool InMemoryTabRestoreService::IsLoaded() const {
  // See comment above.
  return true;
}

void InMemoryTabRestoreService::DeleteLastSession() {
  // See comment above.
}

bool InMemoryTabRestoreService::IsRestoring() const {
  return helper_.IsRestoring();
}

void InMemoryTabRestoreService::Shutdown() {
}

}  // namespace
