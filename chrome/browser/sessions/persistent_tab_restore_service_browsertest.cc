// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(jamescook): Why does this test run on all Aura platforms, instead of
// only Chrome OS or Ash?
#if defined(USE_AURA)

#include "components/sessions/core/persistent_tab_restore_service.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"

using Window = sessions::TabRestoreService::Window;

using PersistentTabRestoreServiceBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(PersistentTabRestoreServiceBrowserTest, RestoreApp) {
  Profile* profile = browser()->profile();
  sessions::TabRestoreService* trs =
      TabRestoreServiceFactory::GetForProfile(profile);
  const char* app_name = "TestApp";

  Browser* app_browser = CreateBrowserForApp(app_name, profile);
  CloseBrowserSynchronously(app_browser);

  // One entry should be created.
  ASSERT_EQ(1U, trs->entries().size());
  const sessions::TabRestoreService::Entry* restored_entry =
      trs->entries().front().get();

  // It should be a window with an app.
  ASSERT_EQ(sessions::TabRestoreService::WINDOW, restored_entry->type);
  const Window* restored_window = static_cast<const Window*>(restored_entry);
  EXPECT_EQ(app_name, restored_window->app_name);
}

#endif  // defined(USE_AURA)
