// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background/background_mode_manager.h"

#include "base/sequenced_task_runner.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

void BackgroundModeManager::EnableLaunchOnStartup(bool should_launch) {
  NOTREACHED();
}

void BackgroundModeManager::DisplayClientInstalledNotification(
    const base::string16& name) {
  // No need to display anything on ChromeOS because all extensions run all
  // the time anyway.
}

base::string16 BackgroundModeManager::GetPreferencesMenuLabel() {
  return l10n_util::GetStringUTF16(IDS_SETTINGS);
}

// static
scoped_refptr<base::SequencedTaskRunner>
BackgroundModeManager::CreateTaskRunner() {
  return nullptr;
}
