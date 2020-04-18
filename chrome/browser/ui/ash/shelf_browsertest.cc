// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_prefs.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/public/interfaces/shelf_test_api.mojom.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/status_bubble.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/user_names.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/aura/test/mus/change_completion_waiter.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

class ShelfBrowserTest : public InProcessBrowserTest {
 public:
  ShelfBrowserTest() = default;
  ~ShelfBrowserTest() override = default;

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Connect to the ash test interface for the shelf.
    content::ServiceManagerConnection::GetForProcess()
        ->GetConnector()
        ->BindInterface(ash::mojom::kServiceName, &shelf_test_api_);
  }

 protected:
  ash::mojom::ShelfTestApiPtr shelf_test_api_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfBrowserTest);
};

// Confirm that a status bubble doesn't cause the shelf to darken.
IN_PROC_BROWSER_TEST_F(ShelfBrowserTest, StatusBubble) {
  ash::mojom::ShelfTestApiAsyncWaiter shelf(shelf_test_api_.get());
  bool shelf_visible = false;
  shelf.IsVisible(&shelf_visible);
  EXPECT_TRUE(shelf_visible);

  // Ensure that the browser abuts the shelf.
  const int shelf_top =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area().bottom();
  gfx::Rect bounds = browser()->window()->GetBounds();
  bounds.set_height(shelf_top - bounds.y());
  browser()->window()->SetBounds(bounds);
  aura::test::WaitForAllChangesToComplete();

  // Browser does not overlap shelf.
  bool has_overlapping_window = false;
  shelf.HasOverlappingWindow(&has_overlapping_window);
  EXPECT_FALSE(has_overlapping_window);

  // Show status, which may overlap the shelf by a pixel.
  browser()->window()->GetStatusBubble()->SetStatus(
      base::UTF8ToUTF16("Dummy Status Text"));
  aura::test::WaitForAllChangesToComplete();
  shelf.UpdateVisibility();

  // Ensure that status doesn't cause overlap.
  shelf.HasOverlappingWindow(&has_overlapping_window);
  EXPECT_FALSE(has_overlapping_window);

  // Ensure that moving the browser slightly down does cause overlap.
  bounds.Offset(0, 1);
  browser()->window()->SetBounds(bounds);
  aura::test::WaitForAllChangesToComplete();
  shelf.HasOverlappingWindow(&has_overlapping_window);
  EXPECT_TRUE(has_overlapping_window);
}

class ShelfGuestSessionBrowserTest : public ShelfBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(chromeos::switches::kGuestSession);
    command_line->AppendSwitch(::switches::kIncognito);
    command_line->AppendSwitchASCII(chromeos::switches::kLoginProfile, "hash");
    command_line->AppendSwitchASCII(
        chromeos::switches::kLoginUser,
        user_manager::GuestAccountId().GetUserEmail());
  }
};

// Tests that in guest session, shelf alignment could be initialized to bottom
// aligned, instead of bottom locked (crbug.com/699661).
IN_PROC_BROWSER_TEST_F(ShelfGuestSessionBrowserTest, ShelfAlignment) {
  // Check the alignment pref for the primary display.
  ash::ShelfAlignment alignment = ash::GetShelfAlignmentPref(
      browser()->profile()->GetPrefs(),
      display::Screen::GetScreen()->GetPrimaryDisplay().id());
  EXPECT_EQ(ash::SHELF_ALIGNMENT_BOTTOM, alignment);

  // Check the locked state, which is not exposed via prefs.
  ash::mojom::ShelfTestApiAsyncWaiter shelf(shelf_test_api_.get());
  bool shelf_bottom_locked = false;
  shelf.IsAlignmentBottomLocked(&shelf_bottom_locked);
  EXPECT_FALSE(shelf_bottom_locked);
}
