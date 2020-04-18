// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/base_shell_dialog_win.h"

#include <algorithm>

#include "base/threading/thread.h"
#include "base/win/scoped_com_initializer.h"

namespace ui {

// static
BaseShellDialogImpl::Owners BaseShellDialogImpl::owners_;
int BaseShellDialogImpl::instance_count_ = 0;

BaseShellDialogImpl::BaseShellDialogImpl() {
  ++instance_count_;
}

BaseShellDialogImpl::~BaseShellDialogImpl() {
  // All runs should be complete by the time this is called!
  if (--instance_count_ == 0)
    DCHECK(owners_.empty());
}

BaseShellDialogImpl::RunState BaseShellDialogImpl::BeginRun(HWND owner) {
  // Cannot run a modal shell dialog if one is already running for this owner.
  DCHECK(!IsRunningDialogForOwner(owner));
  // The owner must be a top level window, otherwise we could end up with two
  // entries in our map for the same top level window.
  DCHECK(!owner || owner == GetAncestor(owner, GA_ROOT));
  RunState run_state;
  run_state.dialog_thread = CreateDialogThread();
  run_state.owner = owner;
  if (owner) {
    owners_.insert(owner);
    DisableOwner(owner);
  }
  return run_state;
}

void BaseShellDialogImpl::EndRun(RunState run_state) {
  if (run_state.owner) {
    DCHECK(IsRunningDialogForOwner(run_state.owner));
    EnableOwner(run_state.owner);
    DCHECK(owners_.find(run_state.owner) != owners_.end());
    owners_.erase(run_state.owner);
  }
  DCHECK(run_state.dialog_thread);
  delete run_state.dialog_thread;
}

bool BaseShellDialogImpl::IsRunningDialogForOwner(HWND owner) const {
  return (owner && owners_.find(owner) != owners_.end());
}

void BaseShellDialogImpl::DisableOwner(HWND owner) {
  if (IsWindow(owner))
    EnableWindow(owner, FALSE);
}

// static
base::Thread* BaseShellDialogImpl::CreateDialogThread() {
  base::Thread* thread = new base::Thread("Chrome_ShellDialogThread");
  // Many shell dialogs require a COM Single-Threaded Apartment (STA) to run.
  thread->init_com_with_mta(false);
  bool started = thread->Start();
  DCHECK(started);
  return thread;
}

void BaseShellDialogImpl::EnableOwner(HWND owner) {
  if (IsWindow(owner))
    EnableWindow(owner, TRUE);
}

}  // namespace ui
