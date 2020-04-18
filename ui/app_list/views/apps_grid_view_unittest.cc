// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/apps_grid_view.h"

#include <stddef.h>

#include <memory>
#include <string>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/icu_test_util.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/test/app_list_test_model.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/test/test_search_result.h"
#include "ui/app_list/views/app_list_folder_view.h"
#include "ui/app_list/views/app_list_item_view.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/apps_grid_view_folder_delegate.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/expand_arrow_view.h"
#include "ui/app_list/views/folder_background_view.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/search_result_tile_item_view.h"
#include "ui/app_list/views/suggestions_container_view.h"
#include "ui/app_list/views/test/apps_grid_view_test_api.h"
#include "ui/aura/window.h"
#include "ui/events/event_utils.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/views_test_base.h"

namespace app_list {
namespace test {

namespace {

constexpr int kNumOfSuggestedApps = 3;

class PageFlipWaiter : public PaginationModelObserver {
 public:
  explicit PageFlipWaiter(PaginationModel* model) : model_(model) {
    model_->AddObserver(this);
  }

  ~PageFlipWaiter() override { model_->RemoveObserver(this); }

  void Wait() {
    DCHECK(!wait_);
    wait_ = true;

    ui_run_loop_.reset(new base::RunLoop);
    ui_run_loop_->Run();
    wait_ = false;
  }

  void Reset() { selected_pages_.clear(); }

  const std::string& selected_pages() const { return selected_pages_; }

 private:
  // PaginationModelObserver overrides:
  void TotalPagesChanged() override {}
  void SelectedPageChanged(int old_selected, int new_selected) override {
    if (!selected_pages_.empty())
      selected_pages_ += ',';
    selected_pages_ += base::IntToString(new_selected);

    if (wait_)
      ui_run_loop_->QuitWhenIdle();
  }
  void TransitionStarted() override {}
  void TransitionChanged() override {}
  void TransitionEnded() override {}

  std::unique_ptr<base::RunLoop> ui_run_loop_;
  PaginationModel* model_ = nullptr;
  bool wait_ = false;
  std::string selected_pages_;

  DISALLOW_COPY_AND_ASSIGN(PageFlipWaiter);
};

class TestSuggestedSearchResult : public TestSearchResult {
 public:
  TestSuggestedSearchResult() {
    set_display_type(ash::SearchResultDisplayType::kRecommendation);
  }
  ~TestSuggestedSearchResult() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestSuggestedSearchResult);
};

}  // namespace

class AppsGridViewTest : public views::ViewsTestBase,
                         public testing::WithParamInterface<bool> {
 public:
  AppsGridViewTest() = default;
  ~AppsGridViewTest() override = default;

  // testing::Test overrides:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    if (testing::UnitTest::GetInstance()->current_test_info()->value_param()) {
      is_rtl_ = GetParam();
      if (is_rtl_)
        base::i18n::SetICUDefaultLocale("he");
    }
    gfx::NativeView parent = GetContext();
    // Ensure that parent is big enough to show the full AppListView.
    parent->SetBounds(gfx::Rect(gfx::Point(0, 0), gfx::Size(1024, 768)));
    delegate_.reset(new AppListTestViewDelegate);
    app_list_view_ = new AppListView(delegate_.get());
    app_list_view_->set_short_animation_for_testing();
    AppListView::InitParams params;
    params.parent = parent;
    app_list_view_->Initialize(params);
    contents_view_ = app_list_view_->app_list_main_view()->contents_view();
    apps_grid_view_ = contents_view_->GetAppsContainerView()->apps_grid_view();
    app_list_view_->GetWidget()->Show();

    model_ = delegate_->GetTestModel();
    search_model_ = delegate_->GetSearchModel();
    suggestions_container_ = apps_grid_view_->suggestions_container_for_test();
    expand_arrow_view_ = apps_grid_view_->expand_arrow_view_for_test();
    for (size_t i = 0; i < kNumOfSuggestedApps; ++i) {
      search_model_->results()->Add(
          std::make_unique<TestSuggestedSearchResult>());
    }
    // Needed to update suggestions from |model_|.
    apps_grid_view_->ResetForShowApps();
    app_list_view_->SetState(AppListViewState::FULLSCREEN_ALL_APPS);
    app_list_view_->Layout();

    test_api_.reset(new AppsGridViewTestApi(apps_grid_view_));
  }
  void TearDown() override {
    app_list_view_->GetWidget()->Close();
    views::ViewsTestBase::TearDown();
  }

 protected:
  AppListItemView* GetItemViewAt(int index) const {
    return static_cast<AppListItemView*>(test_api_->GetViewAtModelIndex(index));
  }

  AppListItemView* GetItemViewForPoint(const gfx::Point& point) const {
    for (size_t i = 0; i < model_->top_level_item_list()->item_count(); ++i) {
      AppListItemView* view = GetItemViewAt(i);
      if (view->bounds().Contains(point))
        return view;
    }
    return nullptr;
  }

  gfx::Rect GetItemRectOnCurrentPageAt(int row, int col) const {
    DCHECK_GT(model_->top_level_item_list()->item_count(), 0u);
    return test_api_->GetItemTileRectOnCurrentPageAt(row, col);
  }

  int GetTilesPerPage(int page) const { return test_api_->TilesPerPage(page); }

  PaginationModel* GetPaginationModel() const {
    return apps_grid_view_->pagination_model();
  }

  AppListFolderView* app_list_folder_view() const {
    return contents_view_->GetAppsContainerView()->app_list_folder_view();
  }

  // Points are in |apps_grid_view_|'s coordinates, and fixed for RTL.
  AppListItemView* SimulateDrag(AppsGridView::Pointer pointer,
                                const gfx::Point& from,
                                const gfx::Point& to) {
    AppListItemView* view = GetItemViewForPoint(from);
    DCHECK(view);

    gfx::NativeWindow window = app_list_view_->GetWidget()->GetNativeWindow();
    gfx::Point root_from(from);
    views::View::ConvertPointToWidget(apps_grid_view_, &root_from);
    aura::Window::ConvertPointToTarget(window, window->GetRootWindow(),
                                       &root_from);
    // Ensure that the |root_from| point is correct if RTL.
    root_from.set_x(apps_grid_view_->GetMirroredXInView(root_from.x()));
    gfx::Point root_to(to);
    views::View::ConvertPointToWidget(apps_grid_view_, &root_to);
    aura::Window::ConvertPointToTarget(window, window->GetRootWindow(),
                                       &root_to);
    // Ensure that the |root_to| point is correct if RTL.
    root_to.set_x(apps_grid_view_->GetMirroredXInView(root_to.x()));
    apps_grid_view_->InitiateDrag(view, pointer, from, root_from);

    ui::MouseEvent drag_event(ui::ET_MOUSE_DRAGGED, to, root_to,
                              ui::EventTimeForNow(), 0, 0);
    apps_grid_view_->UpdateDragFromItem(pointer, drag_event);
    return view;
  }

  void SimulateKeyPress(ui::KeyboardCode key_code) {
    SimulateKeyPress(key_code, ui::EF_NONE);
  }

  void SimulateKeyPress(ui::KeyboardCode key_code, int flags) {
    ui::KeyEvent key_event(ui::ET_KEY_PRESSED, key_code, flags);
    apps_grid_view_->OnKeyPressed(key_event);
  }

  // Tests that the order of item views in the AppsGridView is in accordance
  // with the order in the view model.
  void TestAppListItemViewIndice() {
    const views::ViewModelT<AppListItemView>* view_model =
        apps_grid_view_->view_model_for_test();
    DCHECK_GT(view_model->view_size(), 0);
    const int initial_index =
        apps_grid_view_->GetIndexOf(view_model->view_at(0));
    DCHECK_NE(-1, initial_index);
    for (int i = 0; i < view_model->view_size(); ++i) {
      EXPECT_EQ(view_model->view_at(i),
                apps_grid_view_->child_at(i + initial_index));
    }
  }

  AppListView* app_list_view_ = nullptr;    // Owned by native widget.
  AppsGridView* apps_grid_view_ = nullptr;  // Owned by |app_list_view_|.
  ContentsView* contents_view_ = nullptr;   // Owned by |app_list_view_|.
  SuggestionsContainerView* suggestions_container_ =
      nullptr;                                    // Owned by |apps_grid_view_|.
  ExpandArrowView* expand_arrow_view_ = nullptr;  // Owned by |apps_grid_view_|.
  std::unique_ptr<AppListTestViewDelegate> delegate_;
  AppListTestModel* model_ = nullptr;  // Owned by |delegate_|.
  SearchModel* search_model_ = nullptr;  // Owned by |delegate_|.
  std::unique_ptr<AppsGridViewTestApi> test_api_;
  bool is_rtl_ = false;
  base::test::ScopedFeatureList scoped_feature_list_;
  bool test_with_fullscreen_ = true;

 private:
  // Restores the locale to default when destructor is called.
  base::test::ScopedRestoreICUDefaultLocale restore_locale_;

  DISALLOW_COPY_AND_ASSIGN(AppsGridViewTest);
};

// Instantiate the Boolean which is used to toggle RTL in
// the parameterized tests.
INSTANTIATE_TEST_CASE_P(, AppsGridViewTest, testing::Bool());

class TestAppsGridViewFolderDelegate : public AppsGridViewFolderDelegate {
 public:
  TestAppsGridViewFolderDelegate() = default;
  ~TestAppsGridViewFolderDelegate() override = default;

  void ReparentItem(AppListItemView* original_drag_view,
                    const gfx::Point& drag_point_in_folder_grid,
                    bool has_native_drag) override {}

  void DispatchDragEventForReparent(
      AppsGridView::Pointer pointer,
      const gfx::Point& drag_point_in_folder_grid) override {}

  void DispatchEndDragEventForReparent(bool events_forwarded_to_drag_drop_host,
                                       bool cancel_drag) override {}

  bool IsPointOutsideOfFolderBoundary(const gfx::Point& point) override {
    return false;
  }

  bool IsOEMFolder() const override { return false; }

  void SetRootLevelDragViewVisible(bool visible) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAppsGridViewFolderDelegate);
};

TEST_F(AppsGridViewTest, CreatePage) {
  // Fully populates a page.
  const int kPages = 1;

  EXPECT_EQ(kNumOfSuggestedApps, suggestions_container_->num_results());
  int kExpectedTilesOnFirstPage =
      apps_grid_view_->cols() * (apps_grid_view_->rows_per_page() - 1);
  EXPECT_EQ(kExpectedTilesOnFirstPage, GetTilesPerPage(kPages - 1));

  model_->PopulateApps(kPages * GetTilesPerPage(kPages - 1));
  EXPECT_EQ(kPages, GetPaginationModel()->total_pages());

  // Adds one more and gets a new page created.
  model_->CreateAndAddItem("Extra");
  EXPECT_EQ(kPages + 1, GetPaginationModel()->total_pages());
}

TEST_F(AppsGridViewTest, EnsureHighlightedVisible) {
  const int kPages = 3;
  model_->PopulateApps(GetTilesPerPage(0) + (kPages - 1) * GetTilesPerPage(1));
  EXPECT_EQ(kPages, GetPaginationModel()->total_pages());
  EXPECT_EQ(0, GetPaginationModel()->selected_page());

  // Highlight first one and last one one first page and first page should be
  // selected.
  model_->HighlightItemAt(0);
  EXPECT_EQ(0, GetPaginationModel()->selected_page());
  model_->HighlightItemAt(GetTilesPerPage(0) - 1);
  EXPECT_EQ(0, GetPaginationModel()->selected_page());

  // Highlight first one on 2nd page and 2nd page should be selected.
  model_->HighlightItemAt(GetTilesPerPage(1) + 1);
  EXPECT_EQ(1, GetPaginationModel()->selected_page());

  // Highlight last one in the model and last page should be selected.
  model_->HighlightItemAt(model_->top_level_item_list()->item_count() - 1);
  EXPECT_EQ(kPages - 1, GetPaginationModel()->selected_page());
}

TEST_F(AppsGridViewTest, RemoveSelectedLastApp) {
  const int kTotalItems = 2;
  const int kLastItemIndex = kTotalItems - 1;

  model_->PopulateApps(kTotalItems);

  AppListItemView* last_view = GetItemViewAt(kLastItemIndex);
  apps_grid_view_->SetSelectedView(last_view);
  model_->DeleteItem(model_->GetItemName(kLastItemIndex));

  EXPECT_FALSE(apps_grid_view_->IsSelectedView(last_view));

  // No crash happens.
  AppListItemView* view = GetItemViewAt(0);
  apps_grid_view_->SetSelectedView(view);
  EXPECT_TRUE(apps_grid_view_->IsSelectedView(view));
}

TEST_F(AppsGridViewTest, MouseDragItemIntoFolder) {
  size_t kTotalItems = 3;
  model_->PopulateApps(kTotalItems);
  EXPECT_EQ(model_->top_level_item_list()->item_count(), kTotalItems);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2"), model_->GetModelContent());

  gfx::Point from = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();
  gfx::Point to = GetItemRectOnCurrentPageAt(0, 0).CenterPoint();

  // Dragging item_1 over item_0 creates a folder.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(kTotalItems - 1, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());
  AppListFolderItem* folder_item = static_cast<AppListFolderItem*>(
      model_->top_level_item_list()->item_at(0));
  EXPECT_EQ(2u, folder_item->ChildItemCount());
  AppListItem* item_0 = model_->FindItem("Item 0");
  EXPECT_TRUE(item_0->IsInFolder());
  EXPECT_EQ(folder_item->id(), item_0->folder_id());
  AppListItem* item_1 = model_->FindItem("Item 1");
  EXPECT_TRUE(item_1->IsInFolder());
  EXPECT_EQ(folder_item->id(), item_1->folder_id());
  std::string expected_items = folder_item->id() + ",Item 2";
  EXPECT_EQ(expected_items, model_->GetModelContent());
  test_api_->LayoutToIdealBounds();

  // Dragging item_2 to the folder adds item_2 to the folder.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  apps_grid_view_->EndDrag(false);

  EXPECT_EQ(kTotalItems - 2, model_->top_level_item_list()->item_count());
  EXPECT_EQ(folder_item->id(), model_->GetModelContent());
  EXPECT_EQ(3u, folder_item->ChildItemCount());
  item_0 = model_->FindItem("Item 0");
  EXPECT_TRUE(item_0->IsInFolder());
  EXPECT_EQ(folder_item->id(), item_0->folder_id());
  item_1 = model_->FindItem("Item 1");
  EXPECT_TRUE(item_1->IsInFolder());
  EXPECT_EQ(folder_item->id(), item_1->folder_id());
  AppListItem* item_2 = model_->FindItem("Item 2");
  EXPECT_TRUE(item_2->IsInFolder());
  EXPECT_EQ(folder_item->id(), item_2->folder_id());
  test_api_->LayoutToIdealBounds();
}

TEST_P(AppsGridViewTest, MouseDragMaxItemsInFolder) {
  // Create and add a folder with |kMaxFolderItemsFullscreen - 1| items.
  const size_t kMaxItems = kMaxFolderItemsPerPage * kMaxFolderPages;
  const size_t kTotalItems = kMaxItems - 1;
  model_->CreateAndPopulateFolderWithApps(kTotalItems);
  EXPECT_EQ(1u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());
  AppListFolderItem* folder_item = static_cast<AppListFolderItem*>(
      model_->top_level_item_list()->item_at(0));
  EXPECT_EQ(kTotalItems, folder_item->ChildItemCount());

  // Create and add another 2 items.
  model_->PopulateAppWithId(kTotalItems);
  model_->PopulateAppWithId(kTotalItems + 1);
  EXPECT_EQ(3u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(folder_item->id(), model_->top_level_item_list()->item_at(0)->id());
  EXPECT_EQ(model_->GetItemName(kMaxItems - 1),
            model_->top_level_item_list()->item_at(1)->id());
  EXPECT_EQ(model_->GetItemName(kMaxItems),
            model_->top_level_item_list()->item_at(2)->id());

  gfx::Point from = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();
  gfx::Point to = GetItemRectOnCurrentPageAt(0, 0).CenterPoint();

  // Dragging one item into the folder, the folder should accept the item.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(2u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(folder_item->id(), model_->top_level_item_list()->item_at(0)->id());
  EXPECT_EQ(kMaxItems, folder_item->ChildItemCount());
  EXPECT_EQ(model_->GetItemName(kMaxItems),
            model_->top_level_item_list()->item_at(1)->id());
  test_api_->LayoutToIdealBounds();

  // Dragging the last item over the folder, the folder won't accept the new
  // item.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(2u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(kMaxItems, folder_item->ChildItemCount());
  test_api_->LayoutToIdealBounds();
}

// Check that moving items around doesn't allow a drop to happen into a full
// folder.
TEST_P(AppsGridViewTest, MouseDragMaxItemsInFolderWithMovement) {
  // Create and add a folder with |kMaxFolderItemsFullscreen| in it.
  const size_t kMaxItems = kMaxFolderItemsPerPage * kMaxFolderPages;
  size_t kTotalItems = kMaxItems;
  model_->CreateAndPopulateFolderWithApps(kMaxItems);
  EXPECT_EQ(1u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());
  AppListFolderItem* folder_item = static_cast<AppListFolderItem*>(
      model_->top_level_item_list()->item_at(0));
  EXPECT_EQ(kTotalItems, folder_item->ChildItemCount());

  // Create and add another item.
  model_->PopulateAppWithId(kTotalItems);
  EXPECT_EQ(2u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(folder_item->id(), model_->top_level_item_list()->item_at(0)->id());
  EXPECT_EQ(model_->GetItemName(kMaxItems),
            model_->top_level_item_list()->item_at(1)->id());

  AppListItemView* folder_view =
      GetItemViewForPoint(GetItemRectOnCurrentPageAt(0, 0).CenterPoint());

  // Drag the new item to the left so that the grid reorders.
  gfx::Point from = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();
  gfx::Point to = GetItemRectOnCurrentPageAt(0, 0).bottom_left();
  to.Offset(0, -1);  // Get a point inside the rect.
  AppListItemView* dragged_view = SimulateDrag(AppsGridView::MOUSE, from, to);
  test_api_->LayoutToIdealBounds();

  // The grid now looks like | blank | folder |.
  EXPECT_EQ(nullptr, GetItemViewForPoint(
                         GetItemRectOnCurrentPageAt(0, 0).CenterPoint()));
  EXPECT_EQ(folder_view, GetItemViewForPoint(
                             GetItemRectOnCurrentPageAt(0, 1).CenterPoint()));

  // Move onto the folder and end the drag.
  to = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();
  gfx::Point translated_to =
      gfx::PointAtOffsetFromOrigin(to - dragged_view->origin());
  ui::MouseEvent drag_event(ui::ET_MOUSE_DRAGGED, translated_to, to,
                            ui::EventTimeForNow(), 0, 0);
  apps_grid_view_->UpdateDragFromItem(AppsGridView::MOUSE, drag_event);
  apps_grid_view_->EndDrag(false);

  // The item should not have moved into the folder.
  EXPECT_EQ(2u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(kMaxItems, folder_item->ChildItemCount());
  test_api_->LayoutToIdealBounds();
}

TEST_F(AppsGridViewTest, MouseDragItemReorder) {
  // Using a simulated 2x2 layout for the test. If fullscreen app list is
  // enabled, rows_per_page passed should be 3 as the first row is occupied by
  // suggested apps.
  apps_grid_view_->SetLayout(2, 3);
  model_->PopulateApps(4);
  EXPECT_EQ(4u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());

  // Dragging an item towards its neighbours should not reorder until the drag
  // is past the folder drop point.
  gfx::Point top_right = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();
  gfx::Vector2d drag_vector;
  int half_tile_width = (GetItemRectOnCurrentPageAt(0, 1).x() -
                         GetItemRectOnCurrentPageAt(0, 0).x()) /
                        2;
  int tile_height = GetItemRectOnCurrentPageAt(1, 0).y() -
                    GetItemRectOnCurrentPageAt(0, 0).y();

  // Drag left but stop before the folder dropping circle.
  drag_vector.set_x(-half_tile_width - 4);
  SimulateDrag(AppsGridView::MOUSE, top_right, top_right + drag_vector);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());
  TestAppListItemViewIndice();

  // Drag left, past the folder dropping circle.
  gfx::Vector2d last_drag_vector(drag_vector);
  drag_vector.set_x(-3 * half_tile_width + 4);
  SimulateDrag(AppsGridView::MOUSE, top_right + last_drag_vector,
               top_right + drag_vector);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 1,Item 0,Item 2,Item 3"),
            model_->GetModelContent());
  TestAppListItemViewIndice();

  // Drag down, between apps 2 and 3. The gap should open up, making space for
  // app 1 in the bottom left.
  last_drag_vector = drag_vector;
  drag_vector.set_x(-half_tile_width);
  drag_vector.set_y(tile_height);
  SimulateDrag(AppsGridView::MOUSE, top_right + last_drag_vector,
               top_right + drag_vector);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 0,Item 2,Item 1,Item 3"),
            model_->GetModelContent());
  TestAppListItemViewIndice();

  // Drag up, between apps 0 and 2. The gap should open up, making space for app
  // 1 in the top right.
  last_drag_vector = drag_vector;
  drag_vector.set_x(-half_tile_width);
  drag_vector.set_y(0);
  SimulateDrag(AppsGridView::MOUSE, top_right + last_drag_vector,
               top_right + drag_vector);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());
  TestAppListItemViewIndice();

  // Dragging down past the last app should reorder to the last position.
  last_drag_vector = drag_vector;
  drag_vector.set_x(half_tile_width);
  drag_vector.set_y(2 * tile_height);
  SimulateDrag(AppsGridView::MOUSE, top_right + last_drag_vector,
               top_right + drag_vector);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 0,Item 2,Item 3,Item 1"),
            model_->GetModelContent());
  TestAppListItemViewIndice();
}

TEST_F(AppsGridViewTest, MouseDragFolderReorder) {
  size_t kTotalItems = 2;
  model_->CreateAndPopulateFolderWithApps(kTotalItems);
  model_->PopulateAppWithId(kTotalItems);
  EXPECT_EQ(2u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());
  AppListFolderItem* folder_item = static_cast<AppListFolderItem*>(
      model_->top_level_item_list()->item_at(0));
  EXPECT_EQ("Item 2", model_->top_level_item_list()->item_at(1)->id());

  gfx::Point from = GetItemRectOnCurrentPageAt(0, 0).CenterPoint();
  gfx::Point to = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();

  // Dragging folder over item_1 should leads to re-ordering these two
  // items.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(2u, model_->top_level_item_list()->item_count());
  EXPECT_EQ("Item 2", model_->top_level_item_list()->item_at(0)->id());
  EXPECT_EQ(folder_item->id(), model_->top_level_item_list()->item_at(1)->id());
  test_api_->LayoutToIdealBounds();
  TestAppListItemViewIndice();
}

TEST_F(AppsGridViewTest, MouseDragWithCancelDeleteAddItem) {
  size_t kTotalItems = 4;
  model_->PopulateApps(kTotalItems);
  EXPECT_EQ(model_->top_level_item_list()->item_count(), kTotalItems);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());

  gfx::Point from = GetItemRectOnCurrentPageAt(0, 0).CenterPoint();
  gfx::Point to = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();

  // Canceling drag should keep existing order.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  apps_grid_view_->EndDrag(true);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());
  test_api_->LayoutToIdealBounds();

  // Deleting an item keeps remaining intact.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  model_->DeleteItem(model_->GetItemName(2));
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 3"), model_->GetModelContent());
  test_api_->LayoutToIdealBounds();

  // Adding a launcher item cancels the drag and respects the order.
  SimulateDrag(AppsGridView::MOUSE, from, to);
  model_->CreateAndAddItem("Extra");
  apps_grid_view_->EndDrag(false);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 3,Extra"),
            model_->GetModelContent());
  test_api_->LayoutToIdealBounds();
}

TEST_F(AppsGridViewTest, MouseDragFlipPage) {
  apps_grid_view_->set_page_flip_delay_in_ms_for_testing(10);
  GetPaginationModel()->SetTransitionDurations(10, 10);

  PageFlipWaiter page_flip_waiter(GetPaginationModel());

  const int kPages = 3;
  model_->PopulateApps(GetTilesPerPage(0) + (kPages - 1) * GetTilesPerPage(1));
  EXPECT_EQ(kPages, GetPaginationModel()->total_pages());
  EXPECT_EQ(0, GetPaginationModel()->selected_page());

  gfx::Point from = GetItemRectOnCurrentPageAt(0, 0).CenterPoint();
  gfx::Point to;
  const gfx::Rect apps_grid_bounds = apps_grid_view_->GetLocalBounds();
  to = gfx::Point(apps_grid_bounds.width() / 2, apps_grid_bounds.bottom());

  // For fullscreen/bubble launcher, drag to the bottom/right of bounds.
  page_flip_waiter.Reset();
  SimulateDrag(AppsGridView::MOUSE, from, to);

  // Page should be flipped after sometime to hit page 1 and 2 then stop.
  while (test_api_->HasPendingPageFlip()) {
    page_flip_waiter.Wait();
  }
  EXPECT_EQ("1,2", page_flip_waiter.selected_pages());
  EXPECT_EQ(2, GetPaginationModel()->selected_page());

  // Cancel drag and put the dragged view back to its ideal position so that
  // the next drag would pick it up.
  apps_grid_view_->EndDrag(true);
  test_api_->LayoutToIdealBounds();

  // Now drag to the top edge, and test the other direction.
  to.set_y(apps_grid_bounds.y());

  page_flip_waiter.Reset();
  SimulateDrag(AppsGridView::MOUSE, from, to);

  while (test_api_->HasPendingPageFlip()) {
    page_flip_waiter.Wait();
  }
  EXPECT_EQ("1,0", page_flip_waiter.selected_pages());
  EXPECT_EQ(0, GetPaginationModel()->selected_page());

  apps_grid_view_->EndDrag(true);
}

TEST_F(AppsGridViewTest, UpdateFolderBackgroundOnCancelDrag) {
  const int kTotalItems = 4;
  TestAppsGridViewFolderDelegate folder_delegate;
  apps_grid_view_->set_folder_delegate(&folder_delegate);
  model_->PopulateApps(kTotalItems);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());

  gfx::Point mouse_from = GetItemRectOnCurrentPageAt(0, 0).CenterPoint();
  gfx::Point mouse_to = GetItemRectOnCurrentPageAt(0, 1).CenterPoint();

  // Starts a mouse drag and then cancels it.
  SimulateDrag(AppsGridView::MOUSE, mouse_from, mouse_to);
  apps_grid_view_->EndDrag(true);
  EXPECT_EQ(std::string("Item 0,Item 1,Item 2,Item 3"),
            model_->GetModelContent());
}

// TODO(crbug.com/766807): Remove the test once the new focus model is stable.
TEST_P(AppsGridViewTest, DISABLED_MoveSelectedOnAllAppsTiles) {
  const int kItemsOnSecondPage = 3;
  const int kAllAppsItems = GetTilesPerPage(0) + kItemsOnSecondPage;
  const int kLastIndexOfFirstPage = GetTilesPerPage(0) - 1;
  const int kFirstIndexOfLastRowFirstPage =
      GetTilesPerPage(0) - apps_grid_view_->cols();
  model_->PopulateApps(kAllAppsItems);

  // Tests moving left from the first tile on the first page.
  apps_grid_view_->SetSelectedView(GetItemViewAt(0));
  SimulateKeyPress(is_rtl_ ? ui::VKEY_RIGHT : ui::VKEY_LEFT);
  EXPECT_FALSE(apps_grid_view_->has_selected_view());
  EXPECT_EQ(suggestions_container_->num_results() - 1,
            suggestions_container_->selected_index());
  suggestions_container_->ClearSelectedIndex();

  // Tests moving left from the first tile on the second page.
  apps_grid_view_->SetSelectedView(GetItemViewAt(GetTilesPerPage(0)));
  SimulateKeyPress(is_rtl_ ? ui::VKEY_RIGHT : ui::VKEY_LEFT);
  EXPECT_TRUE(
      apps_grid_view_->IsSelectedView(GetItemViewAt(kLastIndexOfFirstPage)));

  // Tests moving right from the last slot on the first page.
  apps_grid_view_->SetSelectedView(GetItemViewAt(kLastIndexOfFirstPage));
  SimulateKeyPress(is_rtl_ ? ui::VKEY_LEFT : ui::VKEY_RIGHT);
  EXPECT_TRUE(
      apps_grid_view_->IsSelectedView(GetItemViewAt(GetTilesPerPage(0))));

  // Tests moving up from the first row on the first page.
  apps_grid_view_->SetSelectedView(GetItemViewAt(1));
  SimulateKeyPress(ui::VKEY_UP);
  EXPECT_FALSE(apps_grid_view_->has_selected_view());
  EXPECT_EQ(1, suggestions_container_->selected_index());
  suggestions_container_->ClearSelectedIndex();

  // Tests moving up from the first row on the second page.
  apps_grid_view_->SetSelectedView(GetItemViewAt(kAllAppsItems - 1));
  SimulateKeyPress(ui::VKEY_UP);
  int expected_index =
      GetTilesPerPage(0) - 1 - (apps_grid_view_->cols() - kItemsOnSecondPage);
  EXPECT_TRUE(apps_grid_view_->IsSelectedView(GetItemViewAt(expected_index)));

  // Tests moving down from the last row on the first page.
  apps_grid_view_->SetSelectedView(
      GetItemViewAt(kFirstIndexOfLastRowFirstPage));
  SimulateKeyPress(ui::VKEY_DOWN);
  EXPECT_TRUE(
      apps_grid_view_->IsSelectedView(GetItemViewAt(GetTilesPerPage(0))));

  // Tests moving down if no direct tile exists, clamp to the last item.
  apps_grid_view_->SetSelectedView(GetItemViewAt(kLastIndexOfFirstPage));
  SimulateKeyPress(ui::VKEY_DOWN);
  EXPECT_TRUE(
      apps_grid_view_->IsSelectedView(GetItemViewAt(kAllAppsItems - 1)));
}

// Tests that UMA is properly collected when either a suggested or normal app is
// launched.
TEST_F(AppsGridViewTest, UMATestForLaunchingApps) {
  base::HistogramTester histogram_tester;
  model_->PopulateApps(5);

  // Select the first suggested app and launch it.
  contents_view_->GetAppListMainView()->ActivateApp(GetItemViewAt(0)->item(),
                                                    0);

  // Test that histograms recorded that a regular app launched.
  histogram_tester.ExpectBucketCount("Apps.AppListAppLaunchedFullscreen", 0, 1);
  // Test that histograms did not record that a suggested launched.
  histogram_tester.ExpectBucketCount("Apps.AppListAppLaunchedFullscreen", 1, 0);

  // Launch a suggested app.
  suggestions_container_->child_at(0)->OnKeyPressed(
      ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_RETURN, ui::EF_NONE));

  // Test that histograms recorded that a suggested app launched, and that the
  // count for regular apps launched is unchanged.
  histogram_tester.ExpectBucketCount("Apps.AppListAppLaunchedFullscreen", 0, 1);
  histogram_tester.ExpectBucketCount("Apps.AppListAppLaunchedFullscreen", 1, 1);
}

TEST_F(AppsGridViewTest, ItemLabelShortNameOverride) {
  // If the app's full name and short name differ, the title label's tooltip
  // should always be the full name of the app.
  std::string expected_text("xyz");
  std::string expected_tooltip("tooltip");
  AppListItem* item = model_->CreateAndAddItem("Item with short name");
  model_->SetItemNameAndShortName(item, expected_tooltip, expected_text);

  base::string16 actual_tooltip;
  AppListItemView* item_view = GetItemViewAt(0);
  ASSERT_TRUE(item_view);
  const views::Label* title_label = item_view->title();
  EXPECT_TRUE(item_view->GetTooltipText(title_label->bounds().CenterPoint(),
                                        &actual_tooltip));
  EXPECT_EQ(expected_tooltip, base::UTF16ToUTF8(actual_tooltip));
  EXPECT_EQ(expected_text, base::UTF16ToUTF8(title_label->text()));
}

TEST_F(AppsGridViewTest, ItemLabelNoShortName) {
  // If the app's full name and short name are the same, use the default tooltip
  // behavior of the label (only show a tooltip if the title is truncated).
  std::string title("a");
  AppListItem* item = model_->CreateAndAddItem(title);
  model_->SetItemNameAndShortName(item, title, "");

  base::string16 actual_tooltip;
  AppListItemView* item_view = GetItemViewAt(0);
  ASSERT_TRUE(item_view);
  const views::Label* title_label = item_view->title();
  EXPECT_FALSE(title_label->GetTooltipText(title_label->bounds().CenterPoint(),
                                           &actual_tooltip));
  EXPECT_EQ(title, base::UTF16ToUTF8(title_label->text()));
}

TEST_P(AppsGridViewTest, ScrollSequenceHandledByAppListView) {
  model_->PopulateApps(GetTilesPerPage(0) + 1);
  EXPECT_EQ(2, GetPaginationModel()->total_pages());

  gfx::Point apps_grid_view_origin =
      apps_grid_view_->GetBoundsInScreen().origin();
  ui::GestureEvent scroll_begin(
      apps_grid_view_origin.x(), apps_grid_view_origin.y(), 0,
      base::TimeTicks(),
      ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_BEGIN, 0, 1));
  ui::GestureEvent scroll_update(
      apps_grid_view_origin.x(), apps_grid_view_origin.y(), 0,
      base::TimeTicks(),
      ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_UPDATE, 0, 10));

  // Drag down on the app grid when on page 1, this should move the AppListView
  // and not move the AppsGridView.
  apps_grid_view_->OnGestureEvent(&scroll_begin);
  EXPECT_FALSE(scroll_begin.handled());

  // Simulate redirecting the event to app list view through views hierarchy.
  app_list_view_->OnGestureEvent(&scroll_begin);
  EXPECT_TRUE(scroll_begin.handled());

  // The following scroll update events will be sent to the view that handled
  // the scroll begin event.
  app_list_view_->OnGestureEvent(&scroll_update);
  EXPECT_TRUE(scroll_update.handled());
  ASSERT_TRUE(app_list_view_->is_in_drag());
  ASSERT_EQ(0, GetPaginationModel()->transition().progress);
}

TEST_F(AppsGridViewTest,
       OnGestureEventScrollSequenceHandleByPaginationController) {
  model_->PopulateApps(GetTilesPerPage(0) + 1);
  EXPECT_EQ(2, GetPaginationModel()->total_pages());

  gfx::Point apps_grid_view_origin =
      apps_grid_view_->GetBoundsInScreen().origin();
  ui::GestureEvent scroll_begin(
      apps_grid_view_origin.x(), apps_grid_view_origin.y(), 0,
      base::TimeTicks(),
      ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_BEGIN, 0, -1));
  ui::GestureEvent scroll_update(
      apps_grid_view_origin.x(), apps_grid_view_origin.y(), 0,
      base::TimeTicks(),
      ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_UPDATE, 0, -10));

  // Drag up on the app grid when on page 1, this should move the AppsGridView
  // but not the AppListView.
  apps_grid_view_->OnGestureEvent(&scroll_begin);
  EXPECT_TRUE(scroll_begin.handled());
  apps_grid_view_->OnGestureEvent(&scroll_update);
  EXPECT_TRUE(scroll_update.handled());
  ASSERT_FALSE(app_list_view_->is_in_drag());
  ASSERT_NE(0, GetPaginationModel()->transition().progress);
}

TEST_F(AppsGridViewTest, CloseFolderByClickingBackground) {
  AppsContainerView* apps_container_view =
      contents_view_->GetAppsContainerView();

  const size_t kTotalItems = kMaxFolderItemsPerPage;
  model_->CreateAndPopulateFolderWithApps(kTotalItems);
  EXPECT_EQ(1u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());

  // Open the folder.
  test_api_->PressItemAt(0);
  EXPECT_TRUE(apps_container_view->IsInFolderView());

  // Simulate mouse press on folder background to close the folder.
  ui::MouseEvent event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  apps_container_view->folder_background_view()->OnMouseEvent(&event);
  EXPECT_FALSE(apps_container_view->IsInFolderView());
}

TEST_F(AppsGridViewTest, PageResetAfterOpenFolder) {
  const size_t kTotalItems = kMaxFolderPages * kMaxFolderItemsPerPage;
  model_->CreateAndPopulateFolderWithApps(kTotalItems);
  EXPECT_EQ(1u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());

  // Open the folder. It should be at page 0.
  test_api_->PressItemAt(0);
  PaginationModel* pagination_model =
      app_list_folder_view()->items_grid_view()->pagination_model();
  EXPECT_EQ(3, pagination_model->total_pages());
  EXPECT_EQ(0, pagination_model->selected_page());

  // Select page 2.
  pagination_model->SelectPage(2, false /* animate */);
  EXPECT_EQ(2, pagination_model->selected_page());

  // Close the folder and reopen it. It should be at page 0.
  app_list_folder_view()->CloseFolderPage();
  test_api_->PressItemAt(0);
  EXPECT_EQ(3, pagination_model->total_pages());
  EXPECT_EQ(0, pagination_model->selected_page());
}

TEST_F(AppsGridViewTest, FolderColsAndRows) {
  // Populate folders with different number of apps.
  model_->CreateAndPopulateFolderWithApps(2);
  model_->CreateAndPopulateFolderWithApps(5);
  model_->CreateAndPopulateFolderWithApps(9);
  model_->CreateAndPopulateFolderWithApps(15);
  model_->CreateAndPopulateFolderWithApps(17);

  // Check the number of cols and rows for each opened folder.
  AppsGridView* items_grid_view = app_list_folder_view()->items_grid_view();
  test_api_->PressItemAt(0);
  EXPECT_EQ(2, items_grid_view->view_model_for_test()->view_size());
  EXPECT_EQ(2, items_grid_view->cols());
  EXPECT_EQ(1, items_grid_view->rows_per_page());
  app_list_folder_view()->CloseFolderPage();

  test_api_->PressItemAt(1);
  EXPECT_EQ(5, items_grid_view->view_model_for_test()->view_size());
  EXPECT_EQ(3, items_grid_view->cols());
  EXPECT_EQ(2, items_grid_view->rows_per_page());
  app_list_folder_view()->CloseFolderPage();

  test_api_->PressItemAt(2);
  EXPECT_EQ(9, items_grid_view->view_model_for_test()->view_size());
  EXPECT_EQ(3, items_grid_view->cols());
  EXPECT_EQ(3, items_grid_view->rows_per_page());
  app_list_folder_view()->CloseFolderPage();

  test_api_->PressItemAt(3);
  EXPECT_EQ(15, items_grid_view->view_model_for_test()->view_size());
  EXPECT_EQ(4, items_grid_view->cols());
  EXPECT_EQ(4, items_grid_view->rows_per_page());
  app_list_folder_view()->CloseFolderPage();

  test_api_->PressItemAt(4);
  EXPECT_EQ(17, items_grid_view->view_model_for_test()->view_size());
  EXPECT_EQ(4, items_grid_view->cols());
  EXPECT_EQ(4, items_grid_view->rows_per_page());
  app_list_folder_view()->CloseFolderPage();
}

TEST_P(AppsGridViewTest, ScrollDownShouldNotExitFolder) {
  const size_t kTotalItems = kMaxFolderItemsPerPage;
  model_->CreateAndPopulateFolderWithApps(kTotalItems);
  EXPECT_EQ(1u, model_->top_level_item_list()->item_count());
  EXPECT_EQ(AppListFolderItem::kItemType,
            model_->top_level_item_list()->item_at(0)->GetItemType());

  // Open the folder.
  test_api_->PressItemAt(0);
  EXPECT_TRUE(contents_view_->GetAppsContainerView()->IsInFolderView());

  AppsGridView* items_grid_view = app_list_folder_view()->items_grid_view();
  gfx::Point apps_grid_view_origin =
      items_grid_view->GetBoundsInScreen().origin();
  ui::GestureEvent scroll_begin(
      apps_grid_view_origin.x(), apps_grid_view_origin.y(), 0,
      base::TimeTicks(),
      ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_BEGIN, 0, 1));
  ui::GestureEvent scroll_update(
      apps_grid_view_origin.x(), apps_grid_view_origin.y(), 0,
      base::TimeTicks(),
      ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_UPDATE, 0, 10));

  // Drag down on the items grid, this should be handled by items grid view and
  // the folder should not be closed.
  items_grid_view->OnGestureEvent(&scroll_begin);
  EXPECT_TRUE(scroll_begin.handled());
  EXPECT_TRUE(contents_view_->GetAppsContainerView()->IsInFolderView());
}

}  // namespace test
}  // namespace app_list
