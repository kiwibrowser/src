// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/components/shortcut_viewer/views/keyboard_shortcut_view.h"

#include <algorithm>

#include "ash/components/shortcut_viewer/keyboard_shortcut_viewer_metadata.h"
#include "ash/components/shortcut_viewer/vector_icons/vector_icons.h"
#include "ash/components/shortcut_viewer/views/keyboard_shortcut_item_list_view.h"
#include "ash/components/shortcut_viewer/views/keyboard_shortcut_item_view.h"
#include "ash/components/shortcut_viewer/views/ksv_search_box_view.h"
#include "ash/components/strings/grit/ash_components_strings.h"
#include "ash/public/cpp/app_list/internal_app_id_constants.h"
#include "ash/public/cpp/resources/grit/ash_public_unscaled_resources.h"
#include "ash/public/cpp/shelf_item.h"
#include "ash/public/cpp/window_properties.h"
#include "base/bind.h"
#include "base/i18n/string_search.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/string_number_conversions.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/default_style.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/chromeos/search_box/search_box_view_base.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

namespace keyboard_shortcut_viewer {

namespace {

KeyboardShortcutView* g_ksv_view = nullptr;

// Setups the illustration views for search states, including an icon and a
// descriptive text.
void SetupSearchIllustrationView(views::View* illustration_view,
                                 const gfx::VectorIcon& icon,
                                 int message_id) {
  constexpr int kSearchIllustrationIconSize = 150;
  constexpr SkColor kSearchIllustrationIconColor =
      SkColorSetARGB(0xFF, 0xDA, 0xDC, 0xE0);

  illustration_view->set_owned_by_client();
  constexpr int kTopPadding = 98;
  views::BoxLayout* layout =
      illustration_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::kVertical, gfx::Insets(kTopPadding, 0, 0, 0)));
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
  views::ImageView* image_view = new views::ImageView();
  image_view->SetImage(
      gfx::CreateVectorIcon(icon, kSearchIllustrationIconColor));
  image_view->SetImageSize(
      gfx::Size(kSearchIllustrationIconSize, kSearchIllustrationIconSize));
  illustration_view->AddChildView(image_view);

  constexpr SkColor kSearchIllustrationTextColor =
      SkColorSetARGB(0xFF, 0x20, 0x21, 0x24);
  views::Label* text = new views::Label(l10n_util::GetStringUTF16(message_id));
  text->SetEnabledColor(kSearchIllustrationTextColor);
  constexpr int kLabelFontSizeDelta = 1;
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  text->SetFontList(rb.GetFontListWithDelta(
      kLabelFontSizeDelta, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
  illustration_view->AddChildView(text);
}

views::ScrollView* CreateScrollView() {
  views::ScrollView* const scroller = new views::ScrollView();
  scroller->set_draw_overflow_indicator(false);
  scroller->ClipHeightTo(0, 0);
  return scroller;
}

}  // namespace

KeyboardShortcutView::~KeyboardShortcutView() {
  DCHECK_EQ(g_ksv_view, this);
  g_ksv_view = nullptr;
}

// static
views::Widget* KeyboardShortcutView::Toggle(gfx::NativeWindow context) {
  if (g_ksv_view) {
    if (g_ksv_view->GetWidget()->IsActive())
      g_ksv_view->GetWidget()->Close();
    else
      g_ksv_view->GetWidget()->Activate();
  } else {
    base::RecordAction(
        base::UserMetricsAction("KeyboardShortcutViewer.CreateWindow"));

    constexpr gfx::Size kKSVWindowSize(800, 512);
    gfx::Rect window_bounds(kKSVWindowSize);
    if (context) {
      window_bounds = display::Screen::GetScreen()
                          ->GetDisplayNearestWindow(context->GetRootWindow())
                          .work_area();
      window_bounds.ClampToCenteredSize(kKSVWindowSize);
    }
    views::Widget::CreateWindowWithContextAndBounds(new KeyboardShortcutView(),
                                                    context, window_bounds);

    // Set frame view Active and Inactive colors, both are SK_ColorWHITE.
    aura::Window* window = g_ksv_view->GetWidget()->GetNativeWindow();
    window->SetProperty(ash::kFrameActiveColorKey, SK_ColorWHITE);
    window->SetProperty(ash::kFrameInactiveColorKey, SK_ColorWHITE);

    // Set shelf icon.
    const ash::ShelfID shelf_id(app_list::kInternalAppIdKeyboardShortcutViewer);
    window->SetProperty(ash::kShelfIDKey,
                        new std::string(shelf_id.Serialize()));
    window->SetProperty<int>(ash::kShelfItemTypeKey, ash::TYPE_APP);

    // We don't want the KSV window to have a title (per design), however the
    // shelf uses the window title to set the shelf item's tooltip text. The
    // shelf observes changes to the |kWindowIconKey| property and handles that
    // by initializing the shelf item including its tooltip text.
    // TODO(wutao): we can remove resource id IDS_KSV_TITLE after implementing
    // internal app shelf launcher.
    window->SetTitle(l10n_util::GetStringUTF16(IDS_KSV_TITLE));
    gfx::ImageSkia* icon =
        ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
            IDR_SHORTCUT_VIEWER_LOGO_192);
    // The new gfx::ImageSkia instance is owned by the window itself.
    window->SetProperty(aura::client::kWindowIconKey,
                        new gfx::ImageSkia(*icon));

    g_ksv_view->AddAccelerator(
        ui::Accelerator(ui::VKEY_W, ui::EF_CONTROL_DOWN));

    g_ksv_view->GetWidget()->Show();
    g_ksv_view->search_box_view_->search_box()->RequestFocus();
  }
  return g_ksv_view->GetWidget();
}

bool KeyboardShortcutView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  DCHECK_EQ(ui::VKEY_W, accelerator.key_code());
  DCHECK_EQ(ui::EF_CONTROL_DOWN, accelerator.modifiers());
  GetWidget()->Close();
  return true;
}

void KeyboardShortcutView::Layout() {
  gfx::Rect content_bounds(GetContentsBounds());
  if (content_bounds.IsEmpty())
    return;

  constexpr int kSearchBoxTopPadding = 8;
  constexpr int kSearchBoxBottomPadding = 16;
  constexpr int kSearchBoxHorizontalPadding = 30;
  const int left = content_bounds.x();
  const int top = content_bounds.y();
  gfx::Rect search_box_bounds(search_box_view_->GetPreferredSize());
  search_box_bounds.set_width(
      std::min(search_box_bounds.width(),
               content_bounds.width() - 2 * kSearchBoxHorizontalPadding));
  search_box_bounds.set_x(
      left + (content_bounds.width() - search_box_bounds.width()) / 2);
  search_box_bounds.set_y(top + kSearchBoxTopPadding);
  search_box_view_->SetBoundsRect(search_box_bounds);

  views::View* content_view = categories_tabbed_pane_->visible()
                                  ? categories_tabbed_pane_
                                  : search_results_container_;
  const int search_box_used_height = search_box_bounds.height() +
                                     kSearchBoxTopPadding +
                                     kSearchBoxBottomPadding;
  content_view->SetBounds(left, top + search_box_used_height,
                          content_bounds.width(),
                          content_bounds.height() - search_box_used_height);
}

void KeyboardShortcutView::QueryChanged(search_box::SearchBoxViewBase* sender) {
  const bool query_empty = sender->IsSearchBoxTrimmedQueryEmpty();
  if (is_search_box_empty_ != query_empty) {
    is_search_box_empty_ = query_empty;
    UpdateViewsLayout(/*is_search_box_active=*/true);
  }

  debounce_timer_.Stop();
  // If search box is empty, do not show |search_results_container_|.
  if (query_empty)
    return;

  // TODO(wutao): This timeout value is chosen based on subjective search
  // latency tests on Minnie. Objective method or UMA is desired.
  constexpr base::TimeDelta kTimeOut(base::TimeDelta::FromMilliseconds(250));
  debounce_timer_.Start(
      FROM_HERE, kTimeOut,
      base::Bind(&KeyboardShortcutView::ShowSearchResults,
                 base::Unretained(this), sender->search_box()->text()));
}

void KeyboardShortcutView::BackButtonPressed() {
  search_box_view_->ClearSearch();
  search_box_view_->SetSearchBoxActive(false);
}

void KeyboardShortcutView::ActiveChanged(
    search_box::SearchBoxViewBase* sender) {
  const bool is_search_box_active = sender->is_search_box_active();
  is_search_box_empty_ = sender->IsSearchBoxTrimmedQueryEmpty();
  sender->ShowBackOrGoogleIcon(is_search_box_active);
  if (is_search_box_active) {
    base::RecordAction(
        base::UserMetricsAction("KeyboardShortcutViewer.Search"));
  }
  UpdateViewsLayout(is_search_box_active);
}

KeyboardShortcutView::KeyboardShortcutView() {
  DCHECK_EQ(g_ksv_view, nullptr);
  g_ksv_view = this;

  // Default background is transparent.
  SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
  InitViews();
}

void KeyboardShortcutView::InitViews() {
  // Init search box view.
  search_box_view_ = std::make_unique<KSVSearchBoxView>(this);
  search_box_view_->Init();
  AddChildView(search_box_view_.get());

  // Init no search result illustration view.
  search_no_result_view_ = std::make_unique<views::View>();
  SetupSearchIllustrationView(search_no_result_view_.get(),
                              kKsvSearchNoResultIcon, IDS_KSV_SEARCH_NO_RESULT);

  // Init search results container view.
  search_results_container_ = new views::View();
  search_results_container_->SetLayoutManager(
      std::make_unique<views::FillLayout>());
  search_results_container_->SetVisible(false);
  AddChildView(search_results_container_);

  // Init views of KeyboardShortcutItemView.
  // TODO(https://crbug.com/843394): Observe changes in keyboard layout and
  // clear the cache.
  KeyboardShortcutItemView::ClearKeycodeToString16Cache();
  for (const auto& item : GetKeyboardShortcutItemList()) {
    for (auto category : item.categories) {
      shortcut_views_.emplace_back(
          std::make_unique<KeyboardShortcutItemView>(item, category));
      shortcut_views_.back()->set_owned_by_client();
    }
  }
  std::sort(shortcut_views_.begin(), shortcut_views_.end(),
            [](const auto& lhs, const auto& rhs) {
              if (lhs->category() != rhs->category())
                return lhs->category() < rhs->category();
              return lhs->description_label_view()->text() <
                     rhs->description_label_view()->text();
            });

  // Init views of |categories_tabbed_pane_| and KeyboardShortcutItemListViews.
  categories_tabbed_pane_ =
      new views::TabbedPane(views::TabbedPane::Orientation::kVertical,
                            views::TabbedPane::TabStripStyle::kHighlight);
  AddChildView(categories_tabbed_pane_);
  InitCategoriesTabbedPane();
}

void KeyboardShortcutView::InitCategoriesTabbedPane() {
  // If the tab count is 0, |GetSelectedTabIndex()| will return -1, which we do
  // not want to cache.
  active_tab_index_ =
      std::max(0, categories_tabbed_pane_->GetSelectedTabIndex());
  // Although we remove all child views, when the KeyboardShortcutItemView is
  // added back to the |categories_tabbed_pane_|, because there is no width
  // changes, it will not layout the KeyboardShortcutItemView again due to the
  // |MaybeCalculateAndDoLayout()| optimization in KeyboardShortcutItemView.
  // Cannot remove |tab_strip_| and |contents_|, child views of the
  // |categories_tabbed_pane_|, because they are added in the ctor of
  // TabbedPane.
  categories_tabbed_pane_->child_at(0)->RemoveAllChildViews(true);
  categories_tabbed_pane_->child_at(1)->RemoveAllChildViews(true);

  ShortcutCategory current_category = ShortcutCategory::kUnknown;
  KeyboardShortcutItemListView* item_list_view;
  for (const auto& item_view : shortcut_views_) {
    const ShortcutCategory category = item_view->category();
    DCHECK_NE(ShortcutCategory::kUnknown, category);
    if (current_category != category) {
      current_category = category;
      item_list_view = new KeyboardShortcutItemListView();
      views::ScrollView* const scroller = CreateScrollView();
      scroller->SetContents(item_list_view);
      categories_tabbed_pane_->AddTab(GetStringForCategory(current_category),
                                      scroller);
    }
    if (item_list_view->has_children())
      item_list_view->AddHorizontalSeparator();
    views::StyledLabel* description_label_view =
        item_view->description_label_view();
    // Clear any styles used to highlight matched search query in search mode.
    description_label_view->ClearStyleRanges();
    item_list_view->AddChildView(item_view.get());
    // Remove the search query highlight.
    description_label_view->Layout();
  }
}

void KeyboardShortcutView::UpdateViewsLayout(bool is_search_box_active) {
  // 1. Search box is not active: show |categories_tabbed_pane_| and focus on
  //    active tab.
  // 2. Search box is active and empty: show |categories_tabbed_pane_| but focus
  //    on search box.
  // 3. Search box is not empty, show |search_results_container_|. Focus is on
  //    search box.
  const bool should_show_search_results =
      is_search_box_active && !is_search_box_empty_;
  if (!should_show_search_results) {
    // Remove all child views, including horizontal separator lines, to prepare
    // for showing search results next time.
    search_results_container_->RemoveAllChildViews(true);
    if (!categories_tabbed_pane_->visible()) {
      // Repopulate |categories_tabbed_pane_| child views, which were removed
      // when they were added to |search_results_container_|.
      InitCategoriesTabbedPane();
      // Select the category that was active before entering search mode.
      categories_tabbed_pane_->SelectTabAt(active_tab_index_);
    }
  }
  categories_tabbed_pane_->SetVisible(!should_show_search_results);
  search_results_container_->SetVisible(should_show_search_results);
  Layout();
  SchedulePaint();
}

void KeyboardShortcutView::ShowSearchResults(
    const base::string16& search_query) {
  search_results_container_->RemoveAllChildViews(true);
  auto* search_container_content_view = search_no_result_view_.get();
  auto found_items_list_view = std::make_unique<KeyboardShortcutItemListView>();
  base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents finder(
      search_query);
  ShortcutCategory current_category = ShortcutCategory::kUnknown;
  bool has_category_item = false;
  int number_search_results = 0;
  for (const auto& item_view : shortcut_views_) {
    base::string16 description_text =
        item_view->description_label_view()->text();
    base::string16 shortcut_text = item_view->shortcut_label_view()->text();
    size_t match_index = -1;
    size_t match_length = 0;
    // Only highlight |description_label_view_| in KeyboardShortcutItemView.
    // |shortcut_label_view_| has customized style ranges for bubble views
    // so it may have overlappings with the searched ranges. The highlighted
    // behaviors are not defined so we don't highlight
    // |shortcut_label_view_|.
    if (finder.Search(description_text, &match_index, &match_length) ||
        finder.Search(shortcut_text, nullptr, nullptr)) {
      const ShortcutCategory category = item_view->category();
      if (current_category != category) {
        current_category = category;
        has_category_item = false;
        found_items_list_view->AddCategoryLabel(GetStringForCategory(category));
      }
      if (has_category_item)
        found_items_list_view->AddHorizontalSeparator();
      else
        has_category_item = true;
      // Highlight matched query in |description_label_view_|.
      if (match_length > 0) {
        views::StyledLabel::RangeStyleInfo style;
        views::StyledLabel* description_label_view =
            item_view->description_label_view();
        // Clear previous styles.
        description_label_view->ClearStyleRanges();
        style.custom_font = description_label_view->GetDefaultFontList().Derive(
            0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD);
        description_label_view->AddStyleRange(
            gfx::Range(match_index, match_index + match_length), style);
        // Apply new styles to highlight matched search query.
        description_label_view->Layout();
      }

      found_items_list_view->AddChildView(item_view.get());
      ++number_search_results;
    }
  }

  std::vector<base::string16> replacement_strings;
  if (found_items_list_view->has_children()) {
    replacement_strings.emplace_back(
        base::NumberToString16(number_search_results));

    // To offset the padding between the bottom of the |search_box_view_| and
    // the top of the |search_results_container_|.
    constexpr int kTopPadding = -16;
    constexpr int kHorizontalPadding = 128;
    found_items_list_view->SetBorder(views::CreateEmptyBorder(
        gfx::Insets(kTopPadding, kHorizontalPadding, 0, kHorizontalPadding)));
    views::ScrollView* const scroller = CreateScrollView();
    scroller->SetContents(found_items_list_view.release());
    search_container_content_view = scroller;
  }
  replacement_strings.emplace_back(search_query);
  search_box_view_->SetAccessibleValue(l10n_util::GetStringFUTF16(
      number_search_results == 0
          ? IDS_KSV_SEARCH_BOX_ACCESSIBILITY_VALUE_WITHOUT_RESULTS
          : IDS_KSV_SEARCH_BOX_ACCESSIBILITY_VALUE_WITH_RESULTS,
      replacement_strings, nullptr));
  search_results_container_->AddChildView(search_container_content_view);
  Layout();
  SchedulePaint();
}

bool KeyboardShortcutView::CanMaximize() const {
  return false;
}

bool KeyboardShortcutView::CanMinimize() const {
  return true;
}

bool KeyboardShortcutView::CanResize() const {
  return false;
}

views::ClientView* KeyboardShortcutView::CreateClientView(
    views::Widget* widget) {
  return new views::ClientView(widget, this);
}

KeyboardShortcutView* KeyboardShortcutView::GetInstanceForTesting() {
  return g_ksv_view;
}

int KeyboardShortcutView::GetTabCountForTesting() const {
  return categories_tabbed_pane_->GetTabCount();
}

const std::vector<std::unique_ptr<KeyboardShortcutItemView>>&
KeyboardShortcutView::GetShortcutViewsForTesting() const {
  return shortcut_views_;
}

KSVSearchBoxView* KeyboardShortcutView::GetSearchBoxViewForTesting() {
  return search_box_view_.get();
}

}  // namespace keyboard_shortcut_viewer
