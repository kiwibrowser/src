// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_security/screen_tray_item.h"

#include "ash/shell.h"
#include "ash/system/screen_security/screen_capture_tray_item.h"
#include "ash/system/screen_security/screen_share_tray_item.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/test/ash_test_base.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/geometry/point.h"
#include "ui/message_center/message_center.h"
#include "ui/views/view.h"

namespace ash {

// Test with unicode strings.
const char kTestScreenCaptureAppName[] =
    "\xE0\xB2\xA0\x5F\xE0\xB2\xA0 (Screen Capture Test)";
const char kTestScreenShareHelperName[] =
    "\xE5\xAE\x8B\xE8\x85\xBE (Screen Share Test)";

void ClickViewCenter(views::View* view) {
  gfx::Point click_location_in_local =
      gfx::Point(view->width() / 2, view->height() / 2);
  view->OnMousePressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, click_location_in_local, click_location_in_local,
      ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE));
}

class ScreenTrayItemTest : public AshTestBase {
 public:
  ScreenTrayItemTest() {}
  ~ScreenTrayItemTest() override = default;

  ScreenTrayItem* tray_item() { return tray_item_; }
  void set_tray_item(ScreenTrayItem* tray_item) { tray_item_ = tray_item; }

  void reset_stop_callback_hit_count() { stop_callback_hit_count_ = 0; }
  int stop_callback_hit_count() const { return stop_callback_hit_count_; }

  void StartSession() {
    tray_item_->Start(
        base::Bind(&ScreenTrayItemTest::StopCallback, base::Unretained(this)));
  }

  void StopSession() { tray_item_->Stop(); }

  void StopCallback() { stop_callback_hit_count_++; }

 private:
  ScreenTrayItem* tray_item_ = nullptr;
  int stop_callback_hit_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ScreenTrayItemTest);
};

class ScreenCaptureTest : public ScreenTrayItemTest {
 public:
  ScreenCaptureTest() = default;
  ~ScreenCaptureTest() override = default;

  void SetUp() override {
    ScreenTrayItemTest::SetUp();
    // This tray item is owned by its parent system tray view and will
    // be deleted automatically when its parent is destroyed in AshTestBase.
    ScreenTrayItem* item = new ScreenCaptureTrayItem(GetPrimarySystemTray());
    GetPrimarySystemTray()->AddTrayItem(base::WrapUnique(item));
    set_tray_item(item);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenCaptureTest);
};

class ScreenShareTest : public ScreenTrayItemTest {
 public:
  ScreenShareTest() = default;
  ~ScreenShareTest() override = default;

  void SetUp() override {
    ScreenTrayItemTest::SetUp();
    // This tray item is owned by its parent system tray view and will
    // be deleted automatically when its parent is destroyed in AshTestBase.
    ScreenTrayItem* item = new ScreenShareTrayItem(GetPrimarySystemTray());
    GetPrimarySystemTray()->AddTrayItem(base::WrapUnique(item));
    set_tray_item(item);
  }

  DISALLOW_COPY_AND_ASSIGN(ScreenShareTest);
};

void TestStartAndStop(ScreenTrayItemTest* test) {
  ScreenTrayItem* tray_item = test->tray_item();

  EXPECT_FALSE(tray_item->is_started());
  EXPECT_EQ(0, test->stop_callback_hit_count());

  test->StartSession();
  EXPECT_TRUE(tray_item->is_started());

  test->StopSession();
  EXPECT_FALSE(tray_item->is_started());
  EXPECT_EQ(1, test->stop_callback_hit_count());
}

TEST_F(ScreenCaptureTest, StartAndStop) {
  TestStartAndStop(this);
}

TEST_F(ScreenShareTest, StartAndStop) {
  TestStartAndStop(this);
}

void TestNotificationStartAndStop(
    ScreenTrayItemTest* test,
    const std::vector<base::RepeatingClosure>& start_functions,
    const base::RepeatingClosure& stop_function) {
  test->reset_stop_callback_hit_count();
  ScreenTrayItem* tray_item = test->tray_item();
  EXPECT_FALSE(tray_item->is_started());

  for (const auto& start : start_functions) {
    start.Run();
    EXPECT_TRUE(tray_item->is_started());
  }

  // The stop callback is called even if the notification fires.
  stop_function.Run();
  EXPECT_FALSE(tray_item->is_started());
  EXPECT_EQ(static_cast<int>(start_functions.size()),
            test->stop_callback_hit_count());
}

TEST_F(ScreenCaptureTest, NotificationStartAndStop) {
  base::RepeatingClosure start_function = base::BindRepeating(
      &SystemTrayNotifier::NotifyScreenCaptureStart,
      base::Unretained(Shell::Get()->system_tray_notifier()),
      base::Bind(&ScreenTrayItemTest::StopCallback, base::Unretained(this)),
      base::UTF8ToUTF16(kTestScreenCaptureAppName));

  base::RepeatingClosure stop_function = base::BindRepeating(
      &SystemTrayNotifier::NotifyScreenCaptureStop,
      base::Unretained(Shell::Get()->system_tray_notifier()));

  TestNotificationStartAndStop(this, {start_function}, stop_function);
  TestNotificationStartAndStop(this, {start_function, start_function},
                               stop_function);
}

TEST_F(ScreenShareTest, NotificationStartAndStop) {
  base::RepeatingClosure start_function = base::BindRepeating(
      &SystemTrayNotifier::NotifyScreenShareStart,
      base::Unretained(Shell::Get()->system_tray_notifier()),
      base::Bind(&ScreenTrayItemTest::StopCallback, base::Unretained(this)),
      base::UTF8ToUTF16(kTestScreenShareHelperName));

  base::RepeatingClosure stop_function = base::BindRepeating(
      &SystemTrayNotifier::NotifyScreenShareStop,
      base::Unretained(Shell::Get()->system_tray_notifier()));

  TestNotificationStartAndStop(this, {start_function}, stop_function);
  TestNotificationStartAndStop(this, {start_function, start_function},
                               stop_function);
}

void TestSystemTrayInteraction(ScreenTrayItemTest* test) {
  ScreenTrayItem* tray_item = test->tray_item();
  EXPECT_FALSE(tray_item->tray_view()->visible());

  std::vector<SystemTrayItem*> tray_items =
      AshTestBase::GetPrimarySystemTray()->GetTrayItems();
  EXPECT_TRUE(base::ContainsValue(tray_items, tray_item));

  test->StartSession();
  EXPECT_TRUE(tray_item->tray_view()->visible());

  // The default view should be created in a new bubble.
  AshTestBase::GetPrimarySystemTray()->ShowDefaultView(
      BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray_item->default_view());
  AshTestBase::GetPrimarySystemTray()->CloseBubble();
  EXPECT_FALSE(tray_item->default_view());

  test->StopSession();
  EXPECT_FALSE(tray_item->tray_view()->visible());

  // The default view should not be visible because session is stopped.
  AshTestBase::GetPrimarySystemTray()->ShowDefaultView(
      BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_FALSE(tray_item->default_view()->visible());
}

TEST_F(ScreenCaptureTest, SystemTrayInteraction) {
  TestSystemTrayInteraction(this);
}

TEST_F(ScreenShareTest, SystemTrayInteraction) {
  TestSystemTrayInteraction(this);
}

}  // namespace ash
