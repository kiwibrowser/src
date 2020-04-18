// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/search_box/search_box_view_base.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/ime/text_input_flags.h"
#include "ui/chromeos/search_box/search_box_view_delegate.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"

namespace search_box {

namespace {

constexpr int kInnerPadding = 16;

// Preferred width of search box.
constexpr int kSearchBoxPreferredWidth = 544;

// The keyboard select colour (6% black).
constexpr SkColor kSelectedColor = SkColorSetARGB(15, 0, 0, 0);

constexpr SkColor kSearchTextColor = SkColorSetRGB(0x33, 0x33, 0x33);

constexpr int kLightVibrantBlendAlpha = 0xE6;

// Color of placeholder text in zero query state.
constexpr SkColor kZeroQuerySearchboxColor =
    SkColorSetARGB(0x8A, 0x00, 0x00, 0x00);

}  // namespace

// A background that paints a solid white rounded rect with a thin grey border.
class SearchBoxBackground : public views::Background {
 public:
  SearchBoxBackground(int corner_radius, SkColor color)
      : corner_radius_(corner_radius), color_(color) {}
  ~SearchBoxBackground() override {}

  void set_corner_radius(int corner_radius) { corner_radius_ = corner_radius; }
  void set_color(SkColor color) { color_ = color; }

 private:
  // views::Background overrides:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    gfx::Rect bounds = view->GetContentsBounds();

    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(color_);
    canvas->DrawRoundRect(bounds, corner_radius_, flags);
  }

  int corner_radius_;
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(SearchBoxBackground);
};

// To paint grey background on mic and back buttons, and close buttons for
// fullscreen launcher.
class SearchBoxImageButton : public views::ImageButton {
 public:
  explicit SearchBoxImageButton(views::ButtonListener* listener)
      : ImageButton(listener) {
    SetFocusBehavior(FocusBehavior::ALWAYS);

    // Avoid drawing default dashed focus and draw customized focus in
    // OnPaintBackground();
    SetFocusPainter(nullptr);
  }
  ~SearchBoxImageButton() override {}

  // views::View overrides:
  bool OnKeyPressed(const ui::KeyEvent& event) override {
    // Disable space key to press the button. The keyboard events received
    // by this view are forwarded from a Textfield (SearchBoxView) and key
    // released events are not forwarded. This leaves the button in pressed
    // state.
    if (event.key_code() == ui::VKEY_SPACE)
      return false;

    return Button::OnKeyPressed(event);
  }

  void OnFocus() override { SchedulePaint(); }

  void OnBlur() override { SchedulePaint(); }

 private:
  // views::View overrides:
  void OnPaintBackground(gfx::Canvas* canvas) override {
    if (state() == STATE_PRESSED || HasFocus()) {
      canvas->FillRect(gfx::Rect(size()), kSelectedColor);
    }
  }

  const char* GetClassName() const override { return "SearchBoxImageButton"; }

  DISALLOW_COPY_AND_ASSIGN(SearchBoxImageButton);
};

// To show context menu of selected view instead of that of focused view which
// is always this view when the user uses keyboard shortcut to open context
// menu.
class SearchBoxTextfield : public views::Textfield {
 public:
  explicit SearchBoxTextfield(SearchBoxViewBase* search_box_view)
      : search_box_view_(search_box_view) {}
  ~SearchBoxTextfield() override = default;

  // Overridden from views::View:
  void ShowContextMenu(const gfx::Point& p,
                       ui::MenuSourceType source_type) override {
    views::View* selected_view =
        search_box_view_->GetSelectedViewInContentsView();
    if (source_type != ui::MENU_SOURCE_KEYBOARD || !selected_view) {
      views::Textfield::ShowContextMenu(p, source_type);
      return;
    }
    selected_view->ShowContextMenu(
        selected_view->GetKeyboardContextMenuLocation(),
        ui::MENU_SOURCE_KEYBOARD);
  }

  void OnFocus() override {
    search_box_view_->OnOnSearchBoxFocusedChanged();
    Textfield::OnFocus();
  }

  void OnBlur() override {
    search_box_view_->OnOnSearchBoxFocusedChanged();
    // Clear selection and set the caret to the end of the text.
    ClearSelection();
    Textfield::OnBlur();
  }

 private:
  SearchBoxViewBase* const search_box_view_;

  DISALLOW_COPY_AND_ASSIGN(SearchBoxTextfield);
};

SearchBoxViewBase::SearchBoxViewBase(SearchBoxViewDelegate* delegate)
    : delegate_(delegate),
      content_container_(new views::View),
      search_box_(new SearchBoxTextfield(this)) {
  DCHECK(delegate_);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(content_container_);

  content_container_->SetBackground(std::make_unique<SearchBoxBackground>(
      kSearchBoxBorderCornerRadius, background_color_));

  box_layout_ =
      content_container_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::kHorizontal, gfx::Insets(0, kPadding),
          kInnerPadding -
              views::LayoutProvider::Get()->GetDistanceMetric(
                  views::DISTANCE_TEXTFIELD_HORIZONTAL_TEXT_PADDING)));
  box_layout_->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  box_layout_->set_minimum_cross_axis_size(kSearchBoxPreferredHeight);

  search_box_->SetBorder(views::NullBorder());
  search_box_->SetTextColor(kSearchTextColor);
  search_box_->SetBackgroundColor(background_color_);
  search_box_->set_controller(this);
  search_box_->SetTextInputType(ui::TEXT_INPUT_TYPE_SEARCH);
  search_box_->SetTextInputFlags(ui::TEXT_INPUT_FLAG_AUTOCORRECT_OFF);

  back_button_ = new SearchBoxImageButton(this);
  content_container_->AddChildView(back_button_);

  search_icon_ = new views::ImageView();
  content_container_->AddChildView(search_icon_);
  search_box_->set_placeholder_text_color(search_box_color_);
  search_box_->set_placeholder_text_draw_flags(gfx::Canvas::TEXT_ALIGN_CENTER);
  search_box_->SetFontList(search_box_->GetFontList().DeriveWithSizeDelta(2));
  search_box_->SetCursorEnabled(is_search_box_active_);

  content_container_->AddChildView(search_box_);
  box_layout_->SetFlexForView(search_box_, 1);

  // An invisible space view to align |search_box_| to center.
  search_box_right_space_ = new views::View();
  search_box_right_space_->SetPreferredSize(gfx::Size(kSearchIconSize, 0));
  content_container_->AddChildView(search_box_right_space_);

  close_button_ = new SearchBoxImageButton(this);
  content_container_->AddChildView(close_button_);
}

SearchBoxViewBase::~SearchBoxViewBase() = default;

void SearchBoxViewBase::Init() {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetMasksToBounds(true);
  UpdateSearchBoxBorder();
  SetupBackButton();
  SetupCloseButton();
  ModelChanged();
}

bool SearchBoxViewBase::HasSearch() const {
  return !search_box_->text().empty();
}

gfx::Rect SearchBoxViewBase::GetViewBoundsForSearchBoxContentsBounds(
    const gfx::Rect& rect) const {
  gfx::Rect view_bounds = rect;
  view_bounds.Inset(-GetInsets());
  return view_bounds;
}

views::ImageButton* SearchBoxViewBase::back_button() {
  return static_cast<views::ImageButton*>(back_button_);
}

views::ImageButton* SearchBoxViewBase::close_button() {
  return static_cast<views::ImageButton*>(close_button_);
}

void SearchBoxViewBase::ShowBackOrGoogleIcon(bool show_back_button) {
  search_icon_->SetVisible(!show_back_button);
  back_button_->SetVisible(show_back_button);
  content_container_->Layout();
}

void SearchBoxViewBase::SetSearchBoxActive(bool active) {
  if (active == is_search_box_active_)
    return;

  is_search_box_active_ = active;
  UpdateSearchIcon();
  UpdateBackgroundColor(background_color_);
  search_box_->set_placeholder_text_draw_flags(
      active ? (base::i18n::IsRTL() ? gfx::Canvas::TEXT_ALIGN_RIGHT
                                    : gfx::Canvas::TEXT_ALIGN_LEFT)
             : gfx::Canvas::TEXT_ALIGN_CENTER);
  search_box_->set_placeholder_text_color(active ? kZeroQuerySearchboxColor
                                                 : search_box_color_);
  search_box_->SetCursorEnabled(active);

  if (active)
    search_box_->RequestFocus();

  search_box_right_space_->SetVisible(!active);

  UpdateSearchBoxBorder();
  UpdateKeyboardVisibility();

  NotifyActiveChanged();

  content_container_->Layout();
  SchedulePaint();
}

bool SearchBoxViewBase::OnTextfieldEvent() {
  if (is_search_box_active_)
    return false;

  SetSearchBoxActive(true);
  return true;
}

gfx::Size SearchBoxViewBase::CalculatePreferredSize() const {
  return gfx::Size(kSearchBoxPreferredWidth, kSearchBoxPreferredHeight);
}

bool SearchBoxViewBase::OnMouseWheel(const ui::MouseWheelEvent& event) {
  if (contents_view_)
    return contents_view_->OnMouseWheel(event);
  return false;
}

void SearchBoxViewBase::OnEnabledChanged() {
  search_box_->SetEnabled(enabled());
  if (close_button_)
    close_button_->SetEnabled(enabled());
}

const char* SearchBoxViewBase::GetClassName() const {
  return "SearchBoxView";
}

void SearchBoxViewBase::OnGestureEvent(ui::GestureEvent* event) {
  HandleSearchBoxEvent(event);
}

void SearchBoxViewBase::OnMouseEvent(ui::MouseEvent* event) {
  HandleSearchBoxEvent(event);
}

ax::mojom::Role SearchBoxViewBase::GetAccessibleWindowRole() const {
  // Default role of root view is ax::mojom::Role::kWindow which traps ChromeVox
  // focus within the root view. Assign ax::mojom::Role::kGroup here to allow
  // the focus to move from elements in search box to app list view.
  return ax::mojom::Role::kGroup;
}

bool SearchBoxViewBase::ShouldAdvanceFocusToTopLevelWidget() const {
  // Focus should be able to move from search box to items in app list view.
  return true;
}

void SearchBoxViewBase::ButtonPressed(views::Button* sender,
                                      const ui::Event& event) {
  if (back_button_ && sender == back_button_) {
    delegate_->BackButtonPressed();
  } else if (close_button_ && sender == close_button_) {
    ClearSearch();
  } else {
    NOTREACHED();
  }
}

void SearchBoxViewBase::OnTabletModeChanged(bool started) {
  is_tablet_mode_ = started;
  UpdateKeyboardVisibility();
}

void SearchBoxViewBase::OnOnSearchBoxFocusedChanged() {
  UpdateSearchBoxBorder();
  Layout();
  SchedulePaint();
}

bool SearchBoxViewBase::IsSearchBoxTrimmedQueryEmpty() const {
  base::string16 trimmed_query;
  base::TrimWhitespace(search_box_->text(), base::TrimPositions::TRIM_ALL,
                       &trimmed_query);
  return trimmed_query.empty();
}

void SearchBoxViewBase::ClearSearch() {
  search_box_->SetText(base::string16());
  UpdateCloseButtonVisisbility();
  // Updates model and fires query changed manually because SetText() above
  // does not generate ContentsChanged() notification.
  UpdateModel(false);
  NotifyQueryChanged();
}

views::View* SearchBoxViewBase::GetSelectedViewInContentsView() {
  return nullptr;
}

void SearchBoxViewBase::NotifyQueryChanged() {
  DCHECK(delegate_);
  delegate_->QueryChanged(this);
}

void SearchBoxViewBase::NotifyActiveChanged() {
  DCHECK(delegate_);
  delegate_->ActiveChanged(this);
}

// TODO(crbug.com/755219): Unify this with UpdateBackgroundColor.
void SearchBoxViewBase::SetBackgroundColor(SkColor light_vibrant) {
  const SkColor light_vibrant_mixed = color_utils::AlphaBlend(
      SK_ColorWHITE, light_vibrant, kLightVibrantBlendAlpha);
  background_color_ = SK_ColorTRANSPARENT == light_vibrant
                          ? kSearchBoxBackgroundDefault
                          : light_vibrant_mixed;
}

void SearchBoxViewBase::SetSearchBoxColor(SkColor color) {
  search_box_color_ =
      SK_ColorTRANSPARENT == color ? kDefaultSearchboxColor : color;
}

void SearchBoxViewBase::UpdateCloseButtonVisisbility() {
  if (!close_button_)
    return;
  bool should_show_close_button_ = !search_box_->text().empty();
  if (close_button_->visible() == should_show_close_button_)
    return;
  close_button_->SetVisible(should_show_close_button_);
  content_container_->Layout();
}

void SearchBoxViewBase::ContentsChanged(views::Textfield* sender,
                                        const base::string16& new_contents) {
  // Set search box focused when query changes.
  search_box_->RequestFocus();
  UpdateModel(true);
  NotifyQueryChanged();
  SetSearchBoxActive(true);
  UpdateCloseButtonVisisbility();
}

bool SearchBoxViewBase::HandleMouseEvent(views::Textfield* sender,
                                         const ui::MouseEvent& mouse_event) {
  return OnTextfieldEvent();
}

bool SearchBoxViewBase::HandleGestureEvent(
    views::Textfield* sender,
    const ui::GestureEvent& gesture_event) {
  return OnTextfieldEvent();
}

void SearchBoxViewBase::SetSearchBoxBackgroundCornerRadius(int corner_radius) {
  GetSearchBoxBackground()->set_corner_radius(corner_radius);
}

void SearchBoxViewBase::SetSearchBoxBackgroundColor(SkColor color) {
  GetSearchBoxBackground()->set_color(color);
}

void SearchBoxViewBase::SetSearchIconImage(gfx::ImageSkia image) {
  search_icon_->SetImage(image);
}

void SearchBoxViewBase::HandleSearchBoxEvent(ui::LocatedEvent* located_event) {
  if (located_event->type() == ui::ET_MOUSE_PRESSED ||
      located_event->type() == ui::ET_GESTURE_TAP) {
    bool event_is_in_searchbox_bounds =
        GetWidget()->GetWindowBoundsInScreen().Contains(
            located_event->root_location());
    if (is_search_box_active_ || !event_is_in_searchbox_bounds ||
        !search_box_->text().empty())
      return;
    // If the event was within the searchbox bounds and in an inactive empty
    // search box, enable the search box.
    SetSearchBoxActive(true);
  }
  located_event->SetHandled();
}

// TODO(crbug.com/755219): Unify this with SetBackgroundColor.
void SearchBoxViewBase::UpdateBackgroundColor(SkColor color) {
  if (is_search_box_active_)
    color = kSearchBoxBackgroundDefault;
  GetSearchBoxBackground()->set_color(color);
  search_box_->SetBackgroundColor(color);
}

SearchBoxBackground* SearchBoxViewBase::GetSearchBoxBackground() const {
  return static_cast<SearchBoxBackground*>(content_container_->background());
}

}  // namespace search_box
