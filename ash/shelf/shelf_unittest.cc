// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "ash/public/cpp/shelf_model.h"
#include "ash/root_window_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_button.h"
#include "ash/shelf/shelf_controller.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shelf/shelf_view_test_api.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/strings/string_number_conversions.h"
#include "components/session_manager/session_manager_types.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace ash {
namespace {

class ShelfTest : public AshTestBase {
 public:
  ShelfTest() = default;
  ~ShelfTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    ShelfView* shelf_view = GetPrimaryShelf()->GetShelfViewForTesting();
    shelf_model_ = shelf_view->model();

    test_.reset(new ShelfViewTestAPI(shelf_view));
  }

  ShelfModel* shelf_model() { return shelf_model_; }

  ShelfViewTestAPI* test_api() { return test_.get(); }

 protected:
  Shelf* GetSecondaryShelf() {
    return Shell::GetRootWindowControllerWithDisplayId(
               GetSecondaryDisplay().id())
        ->shelf();
  }

 private:
  ShelfModel* shelf_model_ = nullptr;
  std::unique_ptr<ShelfViewTestAPI> test_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTest);
};

// Confirms that ShelfItem reflects the appropriated state.
TEST_F(ShelfTest, StatusReflection) {
  // Initially we have the app list.
  int button_count = test_api()->GetButtonCount();

  // Add a running app.
  ShelfItem item;
  item.id = ShelfID("foo");
  item.type = TYPE_APP;
  item.status = STATUS_RUNNING;
  int index = shelf_model()->Add(item);
  ASSERT_EQ(++button_count, test_api()->GetButtonCount());
  ShelfButton* button = test_api()->GetButton(index);
  EXPECT_EQ(ShelfButton::STATE_RUNNING, button->state());

  // Remove it.
  shelf_model()->RemoveItemAt(index);
  ASSERT_EQ(--button_count, test_api()->GetButtonCount());
}

// Confirm that using the menu will clear the hover attribute. To avoid another
// browser test we check this here.
TEST_F(ShelfTest, CheckHoverAfterMenu) {
  // Initially we have the app list.
  int button_count = test_api()->GetButtonCount();

  // Add a running app.
  ShelfItem item;
  item.id = ShelfID("foo");
  item.type = TYPE_APP;
  item.status = STATUS_RUNNING;
  int index = shelf_model()->Add(item);

  ASSERT_EQ(++button_count, test_api()->GetButtonCount());
  ShelfButton* button = test_api()->GetButton(index);
  button->AddState(ShelfButton::STATE_HOVERED);
  button->ShowContextMenu(gfx::Point(), ui::MENU_SOURCE_MOUSE);
  EXPECT_FALSE(button->state() & ShelfButton::STATE_HOVERED);

  // Remove it.
  shelf_model()->RemoveItemAt(index);
}

TEST_F(ShelfTest, ShowOverflowBubble) {
  ShelfWidget* shelf_widget = GetPrimaryShelf()->shelf_widget();

  // Add app buttons until overflow occurs.
  ShelfItem item;
  item.type = TYPE_APP;
  item.status = STATUS_RUNNING;
  while (!test_api()->IsOverflowButtonVisible()) {
    item.id = ShelfID(base::IntToString(shelf_model()->item_count()));
    shelf_model()->Add(item);
    ASSERT_LT(shelf_model()->item_count(), 10000);
  }

  // Shows overflow bubble.
  test_api()->ShowOverflowBubble();
  EXPECT_TRUE(shelf_widget->IsShowingOverflowBubble());

  // Remove one of the first items in the main shelf view.
  ASSERT_GT(shelf_model()->item_count(), 2);
  shelf_model()->RemoveItemAt(2);

  // Waits for all transitions to finish and there should be no crash.
  test_api()->RunMessageLoopUntilAnimationsDone();
  EXPECT_FALSE(shelf_widget->IsShowingOverflowBubble());
}

// Tests if shelf is hidden on secondary display after the primary display is
// changed.
TEST_F(ShelfTest, ShelfHiddenOnScreenOnSecondaryDisplay) {
  for (const auto& state : {session_manager::SessionState::LOCKED,
                            session_manager::SessionState::LOGIN_PRIMARY}) {
    SCOPED_TRACE(base::StringPrintf("Testing state: %d", state));
    GetSessionControllerClient()->SetSessionState(state);
    UpdateDisplay("800x600,800x600");

    EXPECT_EQ(SHELF_VISIBLE, GetPrimaryShelf()->GetVisibilityState());
    EXPECT_EQ(SHELF_HIDDEN, GetSecondaryShelf()->GetVisibilityState());

    SwapPrimaryDisplay();

    EXPECT_EQ(SHELF_VISIBLE, GetPrimaryShelf()->GetVisibilityState());
    EXPECT_EQ(SHELF_HIDDEN, GetSecondaryShelf()->GetVisibilityState());
  }
}

}  // namespace
}  // namespace ash
