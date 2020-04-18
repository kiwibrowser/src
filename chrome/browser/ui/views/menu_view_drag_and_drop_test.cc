// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/ui/views/menu_test_base.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/menu/submenu_view.h"
#include "ui/views/view.h"

namespace {

const char kTestNestedDragData[] = "test_nested_drag_data";
const char kTestTopLevelDragData[] = "test_top_level_drag_data";

// A simple view which can be dragged.
class TestDragView : public views::View {
 public:
  TestDragView();
  ~TestDragView() override;

 private:
  // views::View:
  int GetDragOperations(const gfx::Point& point) override;
  void WriteDragData(const gfx::Point& point,
                     ui::OSExchangeData* data) override;

  DISALLOW_COPY_AND_ASSIGN(TestDragView);
};

TestDragView::TestDragView() {
}

TestDragView::~TestDragView() {
}

int TestDragView::GetDragOperations(const gfx::Point& point) {
  return ui::DragDropTypes::DRAG_MOVE;
}

void TestDragView::WriteDragData(const gfx::Point& point,
                                 ui::OSExchangeData* data) {
  data->SetString(base::ASCIIToUTF16(kTestNestedDragData));
}

// A simple view to serve as a drop target.
class TestTargetView : public views::View {
 public:
  TestTargetView();
  ~TestTargetView() override;

  // Initializes this view to have the same bounds as |parent| and two draggable
  // child views.
  void Init(views::View* parent);
  bool dragging() const { return dragging_; }
  bool dropped() const { return dropped_; }

 private:
  // views::View:
  bool GetDropFormats(
      int* formats,
      std::set<ui::Clipboard::FormatType>* format_types) override;
  bool AreDropTypesRequired() override;
  bool CanDrop(const OSExchangeData& data) override;
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  int OnPerformDrop(const ui::DropTargetEvent& event) override;
  void OnDragExited() override;

  // Whether or not we are currently dragging.
  bool dragging_;

  // Whether or not a drop has been performed on the view.
  bool dropped_;

  DISALLOW_COPY_AND_ASSIGN(TestTargetView);
};

TestTargetView::TestTargetView() : dragging_(false), dropped_(false) {
}

void TestTargetView::Init(views::View* parent) {
  // First, match the parent's size.
  SetSize(parent->size());

  // Then add two draggable views, each 10x2.
  views::View* first = new TestDragView();
  AddChildView(first);
  first->SetBounds(2, 2, 10, 2);

  views::View* second = new TestDragView();
  AddChildView(second);
  second->SetBounds(15, 2, 10, 2);
}

TestTargetView::~TestTargetView() {
}

bool TestTargetView::GetDropFormats(
    int* formats,
    std::set<ui::Clipboard::FormatType>* format_types) {
  *formats = ui::OSExchangeData::STRING;
  return true;
}

bool TestTargetView::AreDropTypesRequired() {
  return true;
}

bool TestTargetView::CanDrop(const OSExchangeData& data) {
  base::string16 contents;
  return data.GetString(&contents) &&
         contents == base::ASCIIToUTF16(kTestNestedDragData);
}

void TestTargetView::OnDragEntered(const ui::DropTargetEvent& event) {
  dragging_ = true;
}

int TestTargetView::OnDragUpdated(const ui::DropTargetEvent& event) {
  return ui::DragDropTypes::DRAG_MOVE;
}

int TestTargetView::OnPerformDrop(const ui::DropTargetEvent& event) {
  dragging_ = false;
  dropped_ = true;
  return ui::DragDropTypes::DRAG_MOVE;
}

void TestTargetView::OnDragExited() {
  dragging_ = false;
}

}  // namespace

class MenuViewDragAndDropTest : public MenuTestBase {
 public:
  MenuViewDragAndDropTest();
  ~MenuViewDragAndDropTest() override;

 protected:
  TestTargetView* target_view() { return target_view_; }
  bool asked_to_close() const { return asked_to_close_; }
  bool performed_in_menu_drop() const { return performed_in_menu_drop_; }

 private:
  // MenuTestBase:
  void BuildMenu(views::MenuItemView* menu) override;

  // views::MenuDelegate:
  bool GetDropFormats(
      views::MenuItemView* menu,
      int* formats,
      std::set<ui::Clipboard::FormatType>* format_types) override;
  bool AreDropTypesRequired(views::MenuItemView* menu) override;
  bool CanDrop(views::MenuItemView* menu,
               const ui::OSExchangeData& data) override;
  int GetDropOperation(views::MenuItemView* item,
                       const ui::DropTargetEvent& event,
                       DropPosition* position) override;
  int OnPerformDrop(views::MenuItemView* menu,
                    DropPosition position,
                    const ui::DropTargetEvent& event) override;
  bool CanDrag(views::MenuItemView* menu) override;
  void WriteDragData(views::MenuItemView* sender,
                     ui::OSExchangeData* data) override;
  int GetDragOperations(views::MenuItemView* sender) override;
  bool ShouldCloseOnDragComplete() override;

  // The special view in the menu, which supports its own drag and drop.
  TestTargetView* target_view_;

  // Whether or not we have been asked to close on drag complete.
  bool asked_to_close_;

  // Whether or not a drop was performed in-menu (i.e., not including drops
  // in separate child views).
  bool performed_in_menu_drop_;

  DISALLOW_COPY_AND_ASSIGN(MenuViewDragAndDropTest);
};

MenuViewDragAndDropTest::MenuViewDragAndDropTest()
    : target_view_(NULL),
      asked_to_close_(false),
      performed_in_menu_drop_(false) {
}

MenuViewDragAndDropTest::~MenuViewDragAndDropTest() {
}

void MenuViewDragAndDropTest::BuildMenu(views::MenuItemView* menu) {
  // Build a menu item that has a nested view that supports its own drag and
  // drop...
  views::MenuItemView* menu_item_view =
      menu->AppendMenuItem(1,
                           base::ASCIIToUTF16("item 1"),
                           views::MenuItemView::NORMAL);
  target_view_ = new TestTargetView();
  menu_item_view->AddChildView(target_view_);
  // ... as well as two other, normal items.
  menu->AppendMenuItemWithLabel(2, base::ASCIIToUTF16("item 2"));
  menu->AppendMenuItemWithLabel(3, base::ASCIIToUTF16("item 3"));
}

bool MenuViewDragAndDropTest::GetDropFormats(
    views::MenuItemView* menu,
    int* formats,
    std::set<ui::Clipboard::FormatType>* format_types) {
  *formats = ui::OSExchangeData::STRING;
  return true;
}

bool MenuViewDragAndDropTest::AreDropTypesRequired(views::MenuItemView* menu) {
  return true;
}

bool MenuViewDragAndDropTest::CanDrop(views::MenuItemView* menu,
                                      const ui::OSExchangeData& data) {
  base::string16 contents;
  return data.GetString(&contents) &&
         contents == base::ASCIIToUTF16(kTestTopLevelDragData);
}

int MenuViewDragAndDropTest::GetDropOperation(views::MenuItemView* item,
                                              const ui::DropTargetEvent& event,
                                              DropPosition* position) {
  return ui::DragDropTypes::DRAG_MOVE;
}


int MenuViewDragAndDropTest::OnPerformDrop(views::MenuItemView* menu,
                                           DropPosition position,
                                           const ui::DropTargetEvent& event) {
  performed_in_menu_drop_ = true;
  return ui::DragDropTypes::DRAG_MOVE;
}

bool MenuViewDragAndDropTest::CanDrag(views::MenuItemView* menu) {
  return true;
}

void MenuViewDragAndDropTest::WriteDragData(
    views::MenuItemView* sender, ui::OSExchangeData* data) {
  data->SetString(base::ASCIIToUTF16(kTestTopLevelDragData));
}

int MenuViewDragAndDropTest::GetDragOperations(views::MenuItemView* sender) {
  return ui::DragDropTypes::DRAG_MOVE;
}

bool MenuViewDragAndDropTest::ShouldCloseOnDragComplete() {
  asked_to_close_ = true;
  return false;
}

class MenuViewDragAndDropTestTestInMenuDrag : public MenuViewDragAndDropTest {
 public:
  MenuViewDragAndDropTestTestInMenuDrag() {}
  ~MenuViewDragAndDropTestTestInMenuDrag() override {}

 private:
  // MenuViewDragAndDropTest:
  void DoTestWithMenuOpen() override;

  void Step2();
  void Step3();
  void Step4();
};

void MenuViewDragAndDropTestTestInMenuDrag::DoTestWithMenuOpen() {
  // A few sanity checks to make sure the menu built correctly.
  views::SubmenuView* submenu = menu()->GetSubmenu();
  ASSERT_TRUE(submenu);
  ASSERT_TRUE(submenu->IsShowing());
  ASSERT_EQ(3, submenu->GetMenuItemCount());

  // We do this here (instead of in BuildMenu()) so that the menu is already
  // built and the bounds are correct.
  target_view()->Init(submenu->GetMenuItemAt(0));

  // We're going to drag the second menu element.
  views::MenuItemView* drag_view = submenu->GetMenuItemAt(1);
  ASSERT_TRUE(drag_view != NULL);

  // Move mouse to center of menu and press button.
  ui_test_utils::MoveMouseToCenterAndPress(
      drag_view,
      ui_controls::LEFT,
      ui_controls::DOWN,
      CreateEventTask(this, &MenuViewDragAndDropTestTestInMenuDrag::Step2));
}

void MenuViewDragAndDropTestTestInMenuDrag::Step2() {
  views::MenuItemView* drop_target = menu()->GetSubmenu()->GetMenuItemAt(2);
  gfx::Point loc(1, drop_target->height() - 1);
  views::View::ConvertPointToScreen(drop_target, &loc);

  // Start a drag.
  ui_controls::SendMouseMoveNotifyWhenDone(
      loc.x() + 10,
      loc.y(),
      CreateEventTask(this, &MenuViewDragAndDropTestTestInMenuDrag::Step3));

  ScheduleMouseMoveInBackground(loc.x(), loc.y());
}

void MenuViewDragAndDropTestTestInMenuDrag::Step3() {
  // Drop the item on the target.
  views::MenuItemView* drop_target = menu()->GetSubmenu()->GetMenuItemAt(2);
  gfx::Point loc(1, drop_target->height() - 2);
  views::View::ConvertPointToScreen(drop_target, &loc);
  ui_controls::SendMouseMove(loc.x(), loc.y());

  ui_controls::SendMouseEventsNotifyWhenDone(
      ui_controls::LEFT,
      ui_controls::UP,
      CreateEventTask(this, &MenuViewDragAndDropTestTestInMenuDrag::Step4));
}

void MenuViewDragAndDropTestTestInMenuDrag::Step4() {
  // Verify our state.
  // We should have performed an in-menu drop, and the nested view should not
  // have had a drag and drop. Since the drag happened in menu code, the
  // delegate should not have been asked whether or not to close, and the menu
  // should simply be closed.
  EXPECT_TRUE(performed_in_menu_drop());
  EXPECT_FALSE(target_view()->dropped());
  EXPECT_FALSE(asked_to_close());
  EXPECT_FALSE(menu()->GetSubmenu()->IsShowing());

  Done();
}

// Test that an in-menu (i.e., entirely implemented in the menu code) closes the
// menu automatically once the drag is complete, and does not ask the delegate
// to stay open.
// Disabled for being flaky. Tracked in:
// TODO(erg): Fix DND tests on linux_aura. http://crbug.com/163931.
// TODO(tapted): De-flake and run on Mac. http://crbug.com/449058.
#if defined(OS_WIN)
#define MAYBE_TestInMenuDrag TestInMenuDrag
#else
#define MAYBE_TestInMenuDrag DISABLED_TestInMenuDrag
#endif
VIEW_TEST(MenuViewDragAndDropTestTestInMenuDrag, MAYBE_TestInMenuDrag)

class MenuViewDragAndDropTestNestedDrag : public MenuViewDragAndDropTest {
 public:
  MenuViewDragAndDropTestNestedDrag() {}
  ~MenuViewDragAndDropTestNestedDrag() override {}

 private:
  // MenuViewDragAndDropTest:
  void DoTestWithMenuOpen() override;

  void Step2();
  void Step3();
  void Step4();
};

void MenuViewDragAndDropTestNestedDrag::DoTestWithMenuOpen() {
  // Sanity checks: We should be showing the menu, it should have three
  // children, and the first of those children should have a nested view of the
  // TestTargetView.
  views::SubmenuView* submenu = menu()->GetSubmenu();
  ASSERT_TRUE(submenu);
  ASSERT_TRUE(submenu->IsShowing());
  ASSERT_EQ(3, submenu->GetMenuItemCount());
  views::View* first_view = submenu->GetMenuItemAt(0);
  ASSERT_EQ(1, first_view->child_count());
  views::View* child_view = first_view->child_at(0);
  ASSERT_EQ(child_view, target_view());

  // We do this here (instead of in BuildMenu()) so that the menu is already
  // built and the bounds are correct.
  target_view()->Init(submenu->GetMenuItemAt(0));

  // The target view should now have two children.
  ASSERT_EQ(2, target_view()->child_count());

  views::View* drag_view = target_view()->child_at(0);
  ASSERT_TRUE(drag_view != NULL);

  // Move mouse to center of menu and press button.
  ui_test_utils::MoveMouseToCenterAndPress(
      drag_view,
      ui_controls::LEFT,
      ui_controls::DOWN,
      CreateEventTask(this, &MenuViewDragAndDropTestNestedDrag::Step2));
}

void MenuViewDragAndDropTestNestedDrag::Step2() {
  views::View* drop_target = target_view()->child_at(1);
  gfx::Point loc(2, 0);
  views::View::ConvertPointToScreen(drop_target, &loc);

  // Start a drag.
  ui_controls::SendMouseMoveNotifyWhenDone(
      loc.x() + 3,
      loc.y(),
      CreateEventTask(this, &MenuViewDragAndDropTestNestedDrag::Step3));

  ScheduleMouseMoveInBackground(loc.x(), loc.y());
}

void MenuViewDragAndDropTestNestedDrag::Step3() {
  // The view should be dragging now.
  EXPECT_TRUE(target_view()->dragging());

  // Drop the item so that it's now the second item.
  views::View* drop_target = target_view()->child_at(1);
  gfx::Point loc(5, 0);
  views::View::ConvertPointToScreen(drop_target, &loc);
  ui_controls::SendMouseMove(loc.x(), loc.y());

  ui_controls::SendMouseEventsNotifyWhenDone(
      ui_controls::LEFT,
      ui_controls::UP,
      CreateEventTask(this, &MenuViewDragAndDropTestNestedDrag::Step4));
}

void MenuViewDragAndDropTestNestedDrag::Step4() {
  // Check our state.
  // The target view should have finished its drag, and should have dropped the
  // view. The main menu should not have done any drag, and the delegate should
  // have been asked if it wanted to close. Since the delegate did not want to
  // close, the menu should still be open.
  EXPECT_FALSE(target_view()->dragging());
  EXPECT_TRUE(target_view()->dropped());
  EXPECT_FALSE(performed_in_menu_drop());
  EXPECT_TRUE(asked_to_close());
  EXPECT_TRUE(menu()->GetSubmenu()->IsShowing());

  // Clean up.
  menu()->GetSubmenu()->Close();

  Done();
}

// Test that a nested drag (i.e. one via a child view, and not entirely
// implemented in menu code) will consult the delegate before closing the view
// after the drag.
// Disabled for being flaky. Tracked in:
// TODO(erg): Fix DND tests on linux_aura. http://crbug.com/163931.
// TODO(tapted): De-flake and run on Mac. http://crbug.com/449058.
// TODO(crbug.com/829922): Flaky on Windows.
VIEW_TEST(MenuViewDragAndDropTestNestedDrag,
          DISABLED_MenuViewDragAndDropNestedDrag)

class MenuViewDragAndDropForDropStayOpen : public MenuViewDragAndDropTest {
 public:
  MenuViewDragAndDropForDropStayOpen() {}
  ~MenuViewDragAndDropForDropStayOpen() override {}

 private:
  // MenuViewDragAndDropTest:
  int GetMenuRunnerFlags() override;
  void DoTestWithMenuOpen() override;
};

int MenuViewDragAndDropForDropStayOpen::GetMenuRunnerFlags() {
  return views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::NESTED_DRAG |
         views::MenuRunner::FOR_DROP;
}

void MenuViewDragAndDropForDropStayOpen::DoTestWithMenuOpen() {
  views::SubmenuView* submenu = menu()->GetSubmenu();
  ASSERT_TRUE(submenu);
  ASSERT_TRUE(submenu->IsShowing());

  views::MenuController* controller = menu()->GetMenuController();
  ASSERT_TRUE(controller);
  EXPECT_FALSE(controller->IsCancelAllTimerRunningForTest());

  Done();
}

// Test that if a menu is opened for a drop which is handled by a child view
// that the menu does not immediately try to close.
// If this flakes, disable and log details in http://crbug.com/523255.
VIEW_TEST(MenuViewDragAndDropForDropStayOpen, MenuViewStaysOpenForNestedDrag)

class MenuViewDragAndDropForDropCancel : public MenuViewDragAndDropTest {
 public:
  MenuViewDragAndDropForDropCancel() {}
  ~MenuViewDragAndDropForDropCancel() override {}

 private:
  // MenuViewDragAndDropTest:
  int GetMenuRunnerFlags() override;
  void DoTestWithMenuOpen() override;
};

int MenuViewDragAndDropForDropCancel::GetMenuRunnerFlags() {
  return views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::FOR_DROP;
}

void MenuViewDragAndDropForDropCancel::DoTestWithMenuOpen() {
  views::SubmenuView* submenu = menu()->GetSubmenu();
  ASSERT_TRUE(submenu);
  ASSERT_TRUE(submenu->IsShowing());

  views::MenuController* controller = menu()->GetMenuController();
  ASSERT_TRUE(controller);
  EXPECT_TRUE(controller->IsCancelAllTimerRunningForTest());

  Done();
}

// Test that if a menu is opened for a drop handled entirely by menu code, the
// menu will try to close if it does not receive any drag updates.
// If this flakes, disable and log details in http://crbug.com/523255.
VIEW_TEST(MenuViewDragAndDropForDropCancel, MenuViewCancelsForOwnDrag)
