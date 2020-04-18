// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager_stub.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "components/account_id/account_id.h"

MultiUserWindowManagerStub::MultiUserWindowManagerStub() {}

MultiUserWindowManagerStub::~MultiUserWindowManagerStub() {}

void MultiUserWindowManagerStub::SetWindowOwner(aura::Window* window,
                                                const AccountId& account_id) {
  NOTIMPLEMENTED();
}

const AccountId& MultiUserWindowManagerStub::GetWindowOwner(
    aura::Window* window) const {
  return EmptyAccountId();
}

void MultiUserWindowManagerStub::ShowWindowForUser(
    aura::Window* window,
    const AccountId& account_id) {
  NOTIMPLEMENTED();
}

bool MultiUserWindowManagerStub::AreWindowsSharedAmongUsers() const {
  return false;
}

void MultiUserWindowManagerStub::GetOwnersOfVisibleWindows(
    std::set<AccountId>* account_ids) const {}

bool MultiUserWindowManagerStub::IsWindowOnDesktopOfUser(
    aura::Window* window,
    const AccountId& account_id) const {
  return true;
}

const AccountId& MultiUserWindowManagerStub::GetUserPresentingWindow(
    aura::Window* window) const {
  return EmptyAccountId();
}

void MultiUserWindowManagerStub::AddUser(content::BrowserContext* context) {
  NOTIMPLEMENTED();
}

void MultiUserWindowManagerStub::AddObserver(Observer* observer) {
  NOTIMPLEMENTED();
}

void MultiUserWindowManagerStub::RemoveObserver(Observer* observer) {
  NOTIMPLEMENTED();
}
