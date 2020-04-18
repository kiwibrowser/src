// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/session_hierarchy_match_checker.h"

SessionHierarchyMatchChecker::SessionHierarchyMatchChecker(
    const fake_server::SessionsHierarchy& sessions_hierarchy,
    browser_sync::ProfileSyncService* service,
    fake_server::FakeServer* fake_server)
    : SingleClientStatusChangeChecker(service),
      sessions_hierarchy_(sessions_hierarchy),
      verifier_(fake_server) {}

bool SessionHierarchyMatchChecker::IsExitConditionSatisfied() {
  return verifier_.VerifySessions(sessions_hierarchy_);
}

std::string SessionHierarchyMatchChecker::GetDebugMessage() const {
  return "Waiting for matching sessions hierarchy to be reflected in fake "
         "server";
}
