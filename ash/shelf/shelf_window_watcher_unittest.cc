// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_window_watcher.h"

#include <memory>

#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shelf_item.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/transient_window_controller.h"

namespace ash {
namespace {

// Create a test 1x1 icon image with a given |color|.
gfx::ImageSkia CreateImageSkiaIcon(SkColor color) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(1, 1);
  bitmap.eraseColor(color);
  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
}

class ShelfWindowWatcherTest : public AshTestBase {
 public:
  ShelfWindowWatcherTest() : model_(nullptr) {}
  ~ShelfWindowWatcherTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    model_ = Shell::Get()->shelf_model();
    // ShelfModel creates an app list item and back button.
    ASSERT_EQ(2, model_->item_count());
  }

  void TearDown() override {
    model_ = nullptr;
    AshTestBase::TearDown();
  }

  static ShelfID CreateShelfItem(aura::Window* window) {
    static int id = 0;
    ShelfID shelf_id(std::to_string(id++));
    window->SetProperty(kShelfIDKey, new std::string(shelf_id.Serialize()));
    window->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_DIALOG));
    return shelf_id;
  }

 protected:
  ShelfModel* model_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfWindowWatcherTest);
};

// Ensure shelf items are added and removed as windows are opened and closed.
TEST_F(ShelfWindowWatcherTest, OpenAndClose) {
  // Windows with valid ShelfItemType and ShelfID properties get shelf items.
  std::unique_ptr<views::Widget> widget1 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  CreateShelfItem(widget1->GetNativeWindow());
  EXPECT_EQ(3, model_->item_count());
  std::unique_ptr<views::Widget> widget2 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  CreateShelfItem(widget2->GetNativeWindow());
  EXPECT_EQ(4, model_->item_count());

  // Each ShelfItem is removed when the associated window is destroyed.
  widget1.reset();
  EXPECT_EQ(3, model_->item_count());
  widget2.reset();
  EXPECT_EQ(2, model_->item_count());
}

// Ensure shelf items are added and removed for some unknown windows in mash.
TEST_F(ShelfWindowWatcherTest, OpenAndCloseMash) {
  if (Shell::GetAshConfig() != Config::MASH)
    return;

  // Windows with no valid ShelfItemType and ShelfID properties get shelf items.
  std::unique_ptr<views::Widget> widget1 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  EXPECT_EQ(3, model_->item_count());
  std::unique_ptr<views::Widget> widget2 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  EXPECT_EQ(4, model_->item_count());

  // Each ShelfItem is removed when the associated window is destroyed.
  widget1.reset();
  EXPECT_EQ(3, model_->item_count());
  widget2.reset();
  EXPECT_EQ(2, model_->item_count());

  // Windows with type WINDOW_TYPE_NORMAL get shelf items, others do not.
  aura::client::WindowType no_item_types[] = {
      aura::client::WINDOW_TYPE_UNKNOWN, aura::client::WINDOW_TYPE_NORMAL,
      aura::client::WINDOW_TYPE_POPUP,   aura::client::WINDOW_TYPE_CONTROL,
      aura::client::WINDOW_TYPE_PANEL,   aura::client::WINDOW_TYPE_MENU,
      aura::client::WINDOW_TYPE_TOOLTIP};
  for (aura::client::WindowType type : no_item_types) {
    std::unique_ptr<aura::Window> window =
        std::make_unique<aura::Window>(nullptr, type);
    window->Init(ui::LAYER_NOT_DRAWN);
    Shell::GetPrimaryRootWindow()
        ->GetChildById(kShellWindowId_DefaultContainer)
        ->AddChild(window.get());
    window->Show();
    EXPECT_EQ(type == aura::client::WINDOW_TYPE_NORMAL ? 3 : 2,
              model_->item_count());
  }

  // Windows with WindowState::ignored_by_shelf set do not get shelf items.
  widget1 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  wm::GetWindowState(widget1->GetNativeWindow())->set_ignored_by_shelf(true);
  // TODO(msw): Make the flag a window property and remove this workaround.
  widget1->GetNativeWindow()->SetProperty(aura::client::kDrawAttentionKey,
                                          true);
  EXPECT_EQ(2, model_->item_count());
}

TEST_F(ShelfWindowWatcherTest, CreateAndRemoveShelfItemProperties) {
  // Creating windows without a valid ShelfItemType only adds items in mash.
  std::unique_ptr<views::Widget> widget1 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  std::unique_ptr<views::Widget> widget2 =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  const bool is_mash = Shell::GetAshConfig() == Config::MASH;
  EXPECT_EQ(is_mash ? 4 : 2, model_->item_count());

  // Create a ShelfItem for the first window.
  ShelfID id_w1 = CreateShelfItem(widget1->GetNativeWindow());
  EXPECT_EQ(is_mash ? 4 : 3, model_->item_count());

  int index_w1 = model_->ItemIndexByID(id_w1);
  EXPECT_EQ(STATUS_RUNNING, model_->items()[index_w1].status);

  // Create a ShelfItem for the second window.
  ShelfID id_w2 = CreateShelfItem(widget2->GetNativeWindow());
  EXPECT_EQ(4, model_->item_count());

  int index_w2 = model_->ItemIndexByID(id_w2);
  EXPECT_EQ(STATUS_RUNNING, model_->items()[index_w2].status);

  // ShelfItem is removed when the type property is cleared in classic ash.
  widget1->GetNativeWindow()->SetProperty(kShelfItemTypeKey,
                                          static_cast<int32_t>(TYPE_UNDEFINED));
  EXPECT_EQ(is_mash ? 4 : 3, model_->item_count());
  widget2->GetNativeWindow()->SetProperty(kShelfItemTypeKey,
                                          static_cast<int32_t>(TYPE_UNDEFINED));
  EXPECT_EQ(is_mash ? 4 : 2, model_->item_count());
  // Clearing twice doesn't do anything.
  widget2->GetNativeWindow()->SetProperty(kShelfItemTypeKey,
                                          static_cast<int32_t>(TYPE_UNDEFINED));
  EXPECT_EQ(is_mash ? 4 : 2, model_->item_count());

  // Closing the windows will remove the items in mash.
  widget1->CloseNow();
  EXPECT_EQ(is_mash ? 3 : 2, model_->item_count());
  widget2->CloseNow();
  EXPECT_EQ(2, model_->item_count());
}

TEST_F(ShelfWindowWatcherTest, UpdateWindowProperty) {
  // Create a ShelfItem for a new window.
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  ShelfID id = CreateShelfItem(widget->GetNativeWindow());
  EXPECT_EQ(3, model_->item_count());

  int index = model_->ItemIndexByID(id);
  EXPECT_EQ(STATUS_RUNNING, model_->items()[index].status);

  // Update the window's ShelfItemType.
  widget->GetNativeWindow()->SetProperty(kShelfItemTypeKey,
                                         static_cast<int32_t>(TYPE_APP_PANEL));
  // No new item is created after updating a launcher item.
  EXPECT_EQ(3, model_->item_count());
  // index and id are not changed after updating a launcher item.
  EXPECT_EQ(index, model_->ItemIndexByID(id));
  EXPECT_EQ(id, model_->items()[index].id);
}

TEST_F(ShelfWindowWatcherTest, MaximizeAndRestoreWindow) {
  // Create a ShelfItem for a new window.
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  ShelfID id = CreateShelfItem(widget->GetNativeWindow());
  EXPECT_EQ(3, model_->item_count());

  int index = model_->ItemIndexByID(id);
  EXPECT_EQ(STATUS_RUNNING, model_->items()[index].status);

  // Maximize the window.
  wm::WindowState* window_state = wm::GetWindowState(widget->GetNativeWindow());
  EXPECT_FALSE(window_state->IsMaximized());
  window_state->Maximize();
  EXPECT_TRUE(window_state->IsMaximized());
  // No new item is created after maximizing the window.
  EXPECT_EQ(3, model_->item_count());
  // index and id are not changed after maximizing the window.
  EXPECT_EQ(index, model_->ItemIndexByID(id));
  EXPECT_EQ(id, model_->items()[index].id);

  // Restore the window.
  window_state->Restore();
  EXPECT_FALSE(window_state->IsMaximized());
  // No new item is created after restoring the window.
  EXPECT_EQ(3, model_->item_count());
  // Index and id are not changed after maximizing the window.
  EXPECT_EQ(index, model_->ItemIndexByID(id));
  EXPECT_EQ(id, model_->items()[index].id);
}

// Check |window|'s item is not changed during the dragging.
// TODO(simonhong): Add a test for removing a Window during the dragging.
TEST_F(ShelfWindowWatcherTest, DragWindow) {
  // Create a ShelfItem for a new window.
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  ShelfID id = CreateShelfItem(widget->GetNativeWindow());
  EXPECT_EQ(3, model_->item_count());

  int index = model_->ItemIndexByID(id);
  EXPECT_EQ(STATUS_RUNNING, model_->items()[index].status);

  // Simulate dragging of the window and check its item is not changed.
  std::unique_ptr<WindowResizer> resizer(
      CreateWindowResizer(widget->GetNativeWindow(), gfx::Point(), HTCAPTION,
                          ::wm::WINDOW_MOVE_SOURCE_MOUSE));
  ASSERT_TRUE(resizer.get());
  resizer->Drag(gfx::Point(50, 50), 0);
  resizer->CompleteDrag();

  // Index and id are not changed after dragging the window.
  EXPECT_EQ(index, model_->ItemIndexByID(id));
  EXPECT_EQ(id, model_->items()[index].id);
}

// Ensure panels and dialogs get shelf items.
TEST_F(ShelfWindowWatcherTest, PanelAndDialogWindows) {
  // An item is created for a dialog window.
  std::unique_ptr<views::Widget> dialog_widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  aura::Window* dialog = dialog_widget->GetNativeWindow();
  dialog->SetProperty(kShelfIDKey, new std::string(ShelfID("a").Serialize()));
  dialog->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_DIALOG));
  EXPECT_EQ(3, model_->item_count());

  // An item is created for a panel window.
  views::Widget panel_widget;
  views::Widget::InitParams panel_params(views::Widget::InitParams::TYPE_PANEL);
  panel_params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  panel_params.parent = Shell::GetPrimaryRootWindow()->GetChildById(
      kShellWindowId_PanelContainer);
  panel_widget.Init(panel_params);
  panel_widget.Show();
  aura::Window* panel = panel_widget.GetNativeWindow();
  panel->SetProperty(kShelfIDKey, new std::string(ShelfID("b").Serialize()));
  panel->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_APP_PANEL));
  EXPECT_EQ(4, model_->item_count());

  // An item is not created for an app window.
  std::unique_ptr<views::Widget> app_widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  aura::Window* app = app_widget->GetNativeWindow();
  app->SetProperty(kShelfIDKey, new std::string(ShelfID("c").Serialize()));
  app->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_APP));
  EXPECT_EQ(4, model_->item_count());
  app_widget.reset();

  // Each ShelfItem is removed when the associated window is destroyed.
  panel_widget.CloseNow();
  EXPECT_EQ(3, model_->item_count());
  dialog_widget.reset();
  EXPECT_EQ(2, model_->item_count());
}

// Ensure items use the app icon and window icon aura::Window properties.
TEST_F(ShelfWindowWatcherTest, ItemIcon) {
  // Create a ShelfItem for a window; it should have a default icon.
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  aura::Window* window = widget->GetNativeWindow();
  ShelfID id = CreateShelfItem(window);
  EXPECT_EQ(3, model_->item_count());
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::Image default_image = rb.GetImageNamed(IDR_DEFAULT_FAVICON_32);
  EXPECT_TRUE(model_->items()[2].image.BackedBySameObjectAs(
      default_image.AsImageSkia()));

  // Setting a window icon should update the item icon.
  const gfx::ImageSkia red = CreateImageSkiaIcon(SK_ColorRED);
  window->SetProperty(aura::client::kWindowIconKey, new gfx::ImageSkia(red));
  EXPECT_EQ(SK_ColorRED, model_->items()[2].image.bitmap()->getColor(0, 0));

  // Setting an app icon should override the window icon.
  const gfx::ImageSkia blue = CreateImageSkiaIcon(SK_ColorBLUE);
  window->SetProperty(aura::client::kAppIconKey, new gfx::ImageSkia(blue));
  EXPECT_EQ(SK_ColorBLUE, model_->items()[2].image.bitmap()->getColor(0, 0));

  // Clearing the app icon should restore the window icon to the shelf item.
  window->ClearProperty(aura::client::kAppIconKey);
  EXPECT_EQ(SK_ColorRED, model_->items()[2].image.bitmap()->getColor(0, 0));
}

TEST_F(ShelfWindowWatcherTest, DontCreateShelfEntriesForChildWindows) {
  std::unique_ptr<aura::Window> window =
      std::make_unique<aura::Window>(nullptr, aura::client::WINDOW_TYPE_NORMAL);
  window->Init(ui::LAYER_NOT_DRAWN);
  window->SetProperty(kShelfIDKey, new std::string(ShelfID("a").Serialize()));
  window->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_DIALOG));
  Shell::GetPrimaryRootWindow()
      ->GetChildById(kShellWindowId_DefaultContainer)
      ->AddChild(window.get());
  window->Show();
  EXPECT_EQ(3, model_->item_count());

  std::unique_ptr<aura::Window> child =
      std::make_unique<aura::Window>(nullptr, aura::client::WINDOW_TYPE_NORMAL);
  child->Init(ui::LAYER_NOT_DRAWN);
  child->SetProperty(kShelfIDKey, new std::string(ShelfID("b").Serialize()));
  child->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_DIALOG));
  window->AddChild(child.get());
  child->Show();
  // There should not be a new shelf item for |child|.
  EXPECT_EQ(3, model_->item_count());

  child.reset();
  EXPECT_EQ(3, model_->item_count());
  window.reset();
  EXPECT_EQ(2, model_->item_count());
}

TEST_F(ShelfWindowWatcherTest, CreateShelfEntriesForTransientWindows) {
  std::unique_ptr<aura::Window> window =
      std::make_unique<aura::Window>(nullptr, aura::client::WINDOW_TYPE_NORMAL);
  window->Init(ui::LAYER_NOT_DRAWN);
  window->SetProperty(kShelfIDKey, new std::string(ShelfID("a").Serialize()));
  window->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_DIALOG));
  Shell::GetPrimaryRootWindow()
      ->GetChildById(kShellWindowId_DefaultContainer)
      ->AddChild(window.get());
  window->Show();
  EXPECT_EQ(3, model_->item_count());

  std::unique_ptr<aura::Window> transient =
      std::make_unique<aura::Window>(nullptr, aura::client::WINDOW_TYPE_NORMAL);
  transient->Init(ui::LAYER_NOT_DRAWN);
  transient->SetProperty(kShelfIDKey,
                         new std::string(ShelfID("b").Serialize()));
  transient->SetProperty(kShelfItemTypeKey, static_cast<int32_t>(TYPE_DIALOG));
  Shell::GetPrimaryRootWindow()
      ->GetChildById(kShellWindowId_DefaultContainer)
      ->AddChild(transient.get());
  ::wm::TransientWindowController::Get()->AddTransientChild(window.get(),
                                                            transient.get());
  transient->Show();
  // There should be a new shelf item for |transient|.
  EXPECT_EQ(4, model_->item_count());

  transient.reset();
  EXPECT_EQ(3, model_->item_count());
  window.reset();
  EXPECT_EQ(2, model_->item_count());
}

// Ensures ShelfWindowWatcher supports windows opened prior to session start.
using ShelfWindowWatcherSessionStartTest = NoSessionAshTestBase;
TEST_F(ShelfWindowWatcherSessionStartTest, PreExistingWindow) {
  ShelfModel* model = Shell::Get()->shelf_model();
  ASSERT_FALSE(
      Shell::Get()->session_controller()->IsActiveUserSessionStarted());

  // ShelfModel creates an app list item and back button.
  EXPECT_EQ(2, model->item_count());

  // Construct a window that should get a shelf item once the session starts.
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  ShelfWindowWatcherTest::CreateShelfItem(widget->GetNativeWindow());
  EXPECT_EQ(2, model->item_count());

  // Start the test user session; ShelfWindowWatcher will find the open window.
  CreateUserSessions(1);
  EXPECT_EQ(3, model->item_count());
}

}  // namespace
}  // namespace ash
