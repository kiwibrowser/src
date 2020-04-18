// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/window_size_autosaver.h"

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class WindowSizeAutosaverTest : public CocoaProfileTest {
  void SetUp() override {
    CocoaProfileTest::SetUp();
    path_ = "WindowSizeAutosaverTest";
    window_ =
        [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 101, 150, 151)
                                    styleMask:NSTitledWindowMask|
                                              NSResizableWindowMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    // TODO(joi): Do all registration up front.
    static_cast<user_prefs::PrefRegistrySyncable*>(
        profile()->GetPrefs()->DeprecatedGetPrefRegistry())
        ->RegisterDictionaryPref(path_);
  }

  void TearDown() override {
    [window_ close];
    CocoaProfileTest::TearDown();
  }

 public:
  NSWindow* window_;
  const char* path_;
};

TEST_F(WindowSizeAutosaverTest, RestoresAndSavesPos) {
  PrefService* pref = profile()->GetPrefs();
  ASSERT_TRUE(pref != NULL);

  // Check to make sure there is no existing pref for window placement.
  const base::DictionaryValue* placement = pref->GetDictionary(path_);
  ASSERT_TRUE(placement);
  EXPECT_TRUE(placement->empty());

  // Replace the window with one that doesn't have resize controls.
  [window_ close];
  window_ =
      [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 101, 150, 151)
                                  styleMask:NSTitledWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO];

  // Ask the window to save its position, then check that a preference
  // exists.  We're technically passing in a pointer to the user prefs
  // and not the local state prefs, but a PrefService* is a
  // PrefService*, and this is a unittest.

  {
    NSRect frame = [window_ frame];
    // Empty state, shouldn't restore:
    base::scoped_nsobject<WindowSizeAutosaver> sizeSaver(
        [[WindowSizeAutosaver alloc] initWithWindow:window_
                                        prefService:pref
                                               path:path_]);
    EXPECT_EQ(NSMinX(frame), NSMinX([window_ frame]));
    EXPECT_EQ(NSMinY(frame), NSMinY([window_ frame]));
    EXPECT_EQ(NSWidth(frame), NSWidth([window_ frame]));
    EXPECT_EQ(NSHeight(frame), NSHeight([window_ frame]));

    // Move and resize window, should store position but not size.
    [window_ setFrame:NSMakeRect(300, 310, 250, 252) display:NO];
  }

  // Another window movement -- shouldn't be recorded.
  [window_ setFrame:NSMakeRect(400, 420, 160, 162) display:NO];

  {
    // Should restore last stored position, but not size.
    base::scoped_nsobject<WindowSizeAutosaver> sizeSaver(
        [[WindowSizeAutosaver alloc] initWithWindow:window_
                                        prefService:pref
                                               path:path_]);
    EXPECT_EQ(300, NSMinX([window_ frame]));
    EXPECT_EQ(310, NSMinY([window_ frame]));
    EXPECT_EQ(160, NSWidth([window_ frame]));
    EXPECT_EQ(162, NSHeight([window_ frame]));
  }

  // ...and it should be in the profile, too.
  EXPECT_TRUE(pref->GetDictionary(path_) != NULL);
  int x, y;
  const base::DictionaryValue* windowPref = pref->GetDictionary(path_);
  EXPECT_FALSE(windowPref->GetInteger("left", &x));
  EXPECT_FALSE(windowPref->GetInteger("right", &x));
  EXPECT_FALSE(windowPref->GetInteger("top", &x));
  EXPECT_FALSE(windowPref->GetInteger("bottom", &x));
  ASSERT_TRUE(windowPref->GetInteger("x", &x));
  ASSERT_TRUE(windowPref->GetInteger("y", &y));
  EXPECT_EQ(300, x);
  EXPECT_EQ(310, y);
}

TEST_F(WindowSizeAutosaverTest, RestoresAndSavesRect) {
  PrefService* pref = profile()->GetPrefs();
  ASSERT_TRUE(pref != NULL);

  // Check to make sure there is no existing pref for window placement.
  const base::DictionaryValue* placement = pref->GetDictionary(path_);
  ASSERT_TRUE(placement);
  EXPECT_TRUE(placement->empty());

  // Ask the window to save its position, then check that a preference
  // exists.  We're technically passing in a pointer to the user prefs
  // and not the local state prefs, but a PrefService* is a
  // PrefService*, and this is a unittest.

  {
    NSRect frame = [window_ frame];
    // Empty state, shouldn't restore:
    base::scoped_nsobject<WindowSizeAutosaver> sizeSaver(
        [[WindowSizeAutosaver alloc] initWithWindow:window_
                                        prefService:pref
                                               path:path_]);
    EXPECT_EQ(NSMinX(frame), NSMinX([window_ frame]));
    EXPECT_EQ(NSMinY(frame), NSMinY([window_ frame]));
    EXPECT_EQ(NSWidth(frame), NSWidth([window_ frame]));
    EXPECT_EQ(NSHeight(frame), NSHeight([window_ frame]));

    // Move and resize window, should store
    [window_ setFrame:NSMakeRect(300, 310, 250, 252) display:NO];
  }

  // Another window movement -- shouldn't be recorded.
  [window_ setFrame:NSMakeRect(400, 420, 160, 162) display:NO];

  {
    // Should restore last stored size
    base::scoped_nsobject<WindowSizeAutosaver> sizeSaver(
        [[WindowSizeAutosaver alloc] initWithWindow:window_
                                        prefService:pref
                                               path:path_]);
    EXPECT_EQ(300, NSMinX([window_ frame]));
    EXPECT_EQ(310, NSMinY([window_ frame]));
    EXPECT_EQ(250, NSWidth([window_ frame]));
    EXPECT_EQ(252, NSHeight([window_ frame]));
  }

  // ...and it should be in the profile, too.
  EXPECT_TRUE(pref->GetDictionary(path_) != NULL);
  int x1, y1, x2, y2;
  const base::DictionaryValue* windowPref = pref->GetDictionary(path_);
  EXPECT_FALSE(windowPref->GetInteger("x", &x1));
  EXPECT_FALSE(windowPref->GetInteger("y", &x1));
  ASSERT_TRUE(windowPref->GetInteger("left", &x1));
  ASSERT_TRUE(windowPref->GetInteger("right", &x2));
  ASSERT_TRUE(windowPref->GetInteger("top", &y1));
  ASSERT_TRUE(windowPref->GetInteger("bottom", &y2));
  EXPECT_EQ(300, x1);
  EXPECT_EQ(310, y1);
  EXPECT_EQ(300 + 250, x2);
  EXPECT_EQ(310 + 252, y2);
}

// http://crbug.com/39625
TEST_F(WindowSizeAutosaverTest, DoesNotRestoreButClearsEmptyRect) {
  PrefService* pref = profile()->GetPrefs();
  ASSERT_TRUE(pref != NULL);

  DictionaryPrefUpdate update(pref, path_);
  base::DictionaryValue* windowPref = update.Get();
  windowPref->SetInteger("left", 50);
  windowPref->SetInteger("right", 50);
  windowPref->SetInteger("top", 60);
  windowPref->SetInteger("bottom", 60);

  {
    // Window rect shouldn't change...
    NSRect frame = [window_ frame];
    base::scoped_nsobject<WindowSizeAutosaver> sizeSaver(
        [[WindowSizeAutosaver alloc] initWithWindow:window_
                                        prefService:pref
                                               path:path_]);
    EXPECT_EQ(NSMinX(frame), NSMinX([window_ frame]));
    EXPECT_EQ(NSMinY(frame), NSMinY([window_ frame]));
    EXPECT_EQ(NSWidth(frame), NSWidth([window_ frame]));
    EXPECT_EQ(NSHeight(frame), NSHeight([window_ frame]));
  }

  // ...and it should be gone from the profile, too.
  EXPECT_TRUE(pref->GetDictionary(path_) != NULL);
  int x1, y1, x2, y2;
  EXPECT_FALSE(windowPref->GetInteger("x", &x1));
  EXPECT_FALSE(windowPref->GetInteger("y", &x1));
  ASSERT_FALSE(windowPref->GetInteger("left", &x1));
  ASSERT_FALSE(windowPref->GetInteger("right", &x2));
  ASSERT_FALSE(windowPref->GetInteger("top", &y1));
  ASSERT_FALSE(windowPref->GetInteger("bottom", &y2));
}

}  // namespace
