// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/combobox/combobox.h"

#include <stddef.h>

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_action_data.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/default_style.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/models/combobox_model_observer.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/throb_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/common_theme.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/focus_ring.h"
#include "ui/views/controls/focusable_border.h"
#include "ui/views/controls/menu/menu_config.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/prefix_selector.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/mouse_constants.h"
#include "ui/views/painter.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

// STYLE_ACTION arrow container padding widths.
const int kActionLeftPadding = 12;
const int kActionRightPadding = 11;

// Menu border widths
const int kMenuBorderWidthLeft = 1;
const int kMenuBorderWidthTop = 1;
const int kMenuBorderWidthRight = 1;

// Limit how small a combobox can be.
const int kMinComboboxWidth = 25;

// Define the id of the first item in the menu (since it needs to be > 0)
const int kFirstMenuItemId = 1000;

// Used to indicate that no item is currently selected by the user.
const int kNoSelection = -1;

const int kBodyButtonImages[] = IMAGE_GRID(IDR_COMBOBOX_BUTTON);
const int kHoveredBodyButtonImages[] = IMAGE_GRID(IDR_COMBOBOX_BUTTON_H);
const int kPressedBodyButtonImages[] = IMAGE_GRID(IDR_COMBOBOX_BUTTON_P);
const int kFocusedBodyButtonImages[] = IMAGE_GRID(IDR_COMBOBOX_BUTTON_F);
const int kFocusedHoveredBodyButtonImages[] =
    IMAGE_GRID(IDR_COMBOBOX_BUTTON_F_H);
const int kFocusedPressedBodyButtonImages[] =
    IMAGE_GRID(IDR_COMBOBOX_BUTTON_F_P);

#define MENU_IMAGE_GRID(x) { \
    x ## _MENU_TOP, x ## _MENU_CENTER, x ## _MENU_BOTTOM, }

const int kMenuButtonImages[] = MENU_IMAGE_GRID(IDR_COMBOBOX_BUTTON);
const int kHoveredMenuButtonImages[] = MENU_IMAGE_GRID(IDR_COMBOBOX_BUTTON_H);
const int kPressedMenuButtonImages[] = MENU_IMAGE_GRID(IDR_COMBOBOX_BUTTON_P);
const int kFocusedMenuButtonImages[] = MENU_IMAGE_GRID(IDR_COMBOBOX_BUTTON_F);
const int kFocusedHoveredMenuButtonImages[] =
    MENU_IMAGE_GRID(IDR_COMBOBOX_BUTTON_F_H);
const int kFocusedPressedMenuButtonImages[] =
    MENU_IMAGE_GRID(IDR_COMBOBOX_BUTTON_F_P);

#undef MENU_IMAGE_GRID

bool UseMd() {
  return ui::MaterialDesignController::IsSecondaryUiMaterial();
}

SkColor GetTextColorForEnableState(const Combobox& combobox, bool enabled) {
  return style::GetColor(
      combobox, style::CONTEXT_TEXTFIELD,
      enabled ? style::STYLE_PRIMARY : style::STYLE_DISABLED);
}

gfx::Rect PositionArrowWithinContainer(const gfx::Rect& container_bounds,
                                       const gfx::Size& arrow_size,
                                       Combobox::Style style) {
  gfx::Rect bounds(container_bounds);
  if (style == Combobox::STYLE_ACTION) {
    // This positions the arrow horizontally. The later call to
    // ClampToCenteredSize will position it vertically without touching the
    // horizontal position.
    bounds.Inset(kActionLeftPadding, 0, kActionRightPadding, 0);
    DCHECK_EQ(bounds.width(), arrow_size.width());
  }

  bounds.ClampToCenteredSize(arrow_size);
  return bounds;
}

// The transparent button which holds a button state but is not rendered.
class TransparentButton : public Button {
 public:
  TransparentButton(ButtonListener* listener, bool animate_state_change)
      : Button(listener) {
    set_animate_on_state_change(animate_state_change);
    if (animate_state_change)
      SetAnimationDuration(LabelButton::kHoverAnimationDurationMs);
    SetFocusBehavior(FocusBehavior::NEVER);
    set_notify_action(PlatformStyle::kMenuNotifyActivationAction);

    if (UseMd()) {
      SetInkDropMode(InkDropMode::ON);
      set_has_ink_drop_action_on_click(true);
    }
  }
  ~TransparentButton() override {}

  bool OnMousePressed(const ui::MouseEvent& mouse_event) override {
#if !defined(OS_MACOSX)
    // On Mac, comboboxes do not take focus on mouse click, but on other
    // platforms they do.
    parent()->RequestFocus();
#endif
    return Button::OnMousePressed(mouse_event);
  }

  double GetAnimationValue() const {
    return hover_animation().GetCurrentValue();
  }

  // Overridden from InkDropHost:
  std::unique_ptr<InkDrop> CreateInkDrop() override {
    std::unique_ptr<views::InkDropImpl> ink_drop = CreateDefaultInkDropImpl();
    ink_drop->SetShowHighlightOnHover(false);
    return std::move(ink_drop);
  }

  std::unique_ptr<InkDropRipple> CreateInkDropRipple() const override {
    return std::unique_ptr<views::InkDropRipple>(
        new views::FloodFillInkDropRipple(
            size(), GetInkDropCenterBasedOnLastEvent(),
            GetNativeTheme()->GetSystemColor(
                ui::NativeTheme::kColorId_LabelEnabledColor),
            ink_drop_visible_opacity()));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TransparentButton);
};

#if !defined(OS_MACOSX)
// Returns the next or previous valid index (depending on |increment|'s value).
// Skips separator or disabled indices. Returns -1 if there is no valid adjacent
// index.
int GetAdjacentIndex(ui::ComboboxModel* model, int increment, int index) {
  DCHECK(increment == -1 || increment == 1);

  index += increment;
  while (index >= 0 && index < model->GetItemCount()) {
    if (!model->IsItemSeparatorAt(index) || !model->IsItemEnabledAt(index))
      return index;
    index += increment;
  }
  return kNoSelection;
}
#endif

// Returns the image resource ids of an array for the body button.
//
// TODO(hajimehoshi): This function should return the images for the 'disabled'
// status. (crbug/270052)
const int* GetBodyButtonImageIds(bool focused,
                                 Button::ButtonState state,
                                 size_t* num) {
  DCHECK(num);
  *num = 9;
  switch (state) {
    case Button::STATE_DISABLED:
      return focused ? kFocusedBodyButtonImages : kBodyButtonImages;
    case Button::STATE_NORMAL:
      return focused ? kFocusedBodyButtonImages : kBodyButtonImages;
    case Button::STATE_HOVERED:
      return focused ?
          kFocusedHoveredBodyButtonImages : kHoveredBodyButtonImages;
    case Button::STATE_PRESSED:
      return focused ?
          kFocusedPressedBodyButtonImages : kPressedBodyButtonImages;
    default:
      NOTREACHED();
  }
  return NULL;
}

// Returns the image resource ids of an array for the menu button.
const int* GetMenuButtonImageIds(bool focused,
                                 Button::ButtonState state,
                                 size_t* num) {
  DCHECK(num);
  *num = 3;
  switch (state) {
    case Button::STATE_DISABLED:
      return focused ? kFocusedMenuButtonImages : kMenuButtonImages;
    case Button::STATE_NORMAL:
      return focused ? kFocusedMenuButtonImages : kMenuButtonImages;
    case Button::STATE_HOVERED:
      return focused ?
          kFocusedHoveredMenuButtonImages : kHoveredMenuButtonImages;
    case Button::STATE_PRESSED:
      return focused ?
          kFocusedPressedMenuButtonImages : kPressedMenuButtonImages;
    default:
      NOTREACHED();
  }
  return NULL;
}

// Returns the images for the menu buttons.
std::vector<const gfx::ImageSkia*> GetMenuButtonImages(
    bool focused,
    Button::ButtonState state) {
  const int* ids;
  size_t num_ids;
  ids = GetMenuButtonImageIds(focused, state, &num_ids);
  std::vector<const gfx::ImageSkia*> images;
  images.reserve(num_ids);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  for (size_t i = 0; i < num_ids; i++)
    images.push_back(rb.GetImageSkiaNamed(ids[i]));
  return images;
}

// Paints three images in a column at the given location. The center image is
// stretched so as to fit the given height.
void PaintImagesVertically(gfx::Canvas* canvas,
                           const gfx::ImageSkia& top_image,
                           const gfx::ImageSkia& center_image,
                           const gfx::ImageSkia& bottom_image,
                           int x, int y, int width, int height) {
  canvas->DrawImageInt(top_image,
                       0, 0, top_image.width(), top_image.height(),
                       x, y, width, top_image.height(), false);
  y += top_image.height();
  int center_height = height - top_image.height() - bottom_image.height();
  canvas->DrawImageInt(center_image,
                       0, 0, center_image.width(), center_image.height(),
                       x, y, width, center_height, false);
  y += center_height;
  canvas->DrawImageInt(bottom_image,
                       0, 0, bottom_image.width(), bottom_image.height(),
                       x, y, width, bottom_image.height(), false);
}

// Paints the arrow button.
void PaintArrowButton(
    gfx::Canvas* canvas,
    const std::vector<const gfx::ImageSkia*>& arrow_button_images,
    int x, int height) {
  PaintImagesVertically(canvas,
                        *arrow_button_images[0],
                        *arrow_button_images[1],
                        *arrow_button_images[2],
                        x, 0, arrow_button_images[0]->width(), height);
}

}  // namespace

// static
const char Combobox::kViewClassName[] = "views/Combobox";

// Adapts a ui::ComboboxModel to a ui::MenuModel.
class Combobox::ComboboxMenuModel : public ui::MenuModel,
                                    public ui::ComboboxModelObserver {
 public:
  ComboboxMenuModel(Combobox* owner, ui::ComboboxModel* model)
      : owner_(owner), model_(model) {
    model_->AddObserver(this);
  }

  ~ComboboxMenuModel() override { model_->RemoveObserver(this); }

 private:
  bool UseCheckmarks() const {
    return owner_->style_ != STYLE_ACTION &&
           MenuConfig::instance().check_selected_combobox_item;
  }

  // Overridden from MenuModel:
  bool HasIcons() const override { return false; }

  int GetItemCount() const override { return model_->GetItemCount(); }

  ItemType GetTypeAt(int index) const override {
    if (model_->IsItemSeparatorAt(index)) {
      // In action menus, disallow <item>, <separator>, ... since that would put
      // a separator at the top of the menu.
      DCHECK(index != 1 || owner_->style_ != STYLE_ACTION);
      return TYPE_SEPARATOR;
    }
    return UseCheckmarks() ? TYPE_CHECK : TYPE_COMMAND;
  }

  ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override {
    return ui::NORMAL_SEPARATOR;
  }

  int GetCommandIdAt(int index) const override {
    return index + kFirstMenuItemId;
  }

  base::string16 GetLabelAt(int index) const override {
    // Inserting the Unicode formatting characters if necessary so that the
    // text is displayed correctly in right-to-left UIs.
    base::string16 text = model_->GetItemAt(index);
    base::i18n::AdjustStringForLocaleDirection(&text);
    return text;
  }

  bool IsItemDynamicAt(int index) const override { return true; }

  const gfx::FontList* GetLabelFontListAt(int index) const override {
    return &GetFontList();
  }

  bool GetAcceleratorAt(int index,
                        ui::Accelerator* accelerator) const override {
    return false;
  }

  bool IsItemCheckedAt(int index) const override {
    return UseCheckmarks() && index == owner_->selected_index_;
  }

  int GetGroupIdAt(int index) const override { return -1; }

  bool GetIconAt(int index, gfx::Image* icon) override { return false; }

  ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override {
    return nullptr;
  }

  bool IsEnabledAt(int index) const override {
    return model_->IsItemEnabledAt(index);
  }

  bool IsVisibleAt(int index) const override {
    // When STYLE_ACTION is used, the first item is not added to the menu. It is
    // assumed that the first item is always selected and rendered on the top of
    // the action button.
    return index > 0 || owner_->style_ != STYLE_ACTION;
  }

  void HighlightChangedTo(int index) override {}

  void ActivatedAt(int index) override {
    owner_->selected_index_ = index;
    owner_->OnPerformAction();
  }

  void ActivatedAt(int index, int event_flags) override { ActivatedAt(index); }

  MenuModel* GetSubmenuModelAt(int index) const override { return nullptr; }

  void SetMenuModelDelegate(
      ui::MenuModelDelegate* menu_model_delegate) override {}

  ui::MenuModelDelegate* GetMenuModelDelegate() const override {
    return nullptr;
  }

  // Overridden from ComboboxModelObserver:
  void OnComboboxModelChanged(ui::ComboboxModel* model) override {
    owner_->ModelChanged();
  }

  Combobox* owner_;           // Weak. Owns this.
  ui::ComboboxModel* model_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(ComboboxMenuModel);
};

////////////////////////////////////////////////////////////////////////////////
// Combobox, public:

Combobox::Combobox(std::unique_ptr<ui::ComboboxModel> model, Style style)
    : Combobox(model.get(), style) {
  owned_model_ = std::move(model);
}

Combobox::Combobox(ui::ComboboxModel* model, Style style)
    : model_(model),
      style_(style),
      listener_(nullptr),
      selected_index_(style == STYLE_ACTION ? 0 : model_->GetDefaultIndex()),
      invalid_(false),
      menu_model_(new ComboboxMenuModel(this, model)),
      text_button_(new TransparentButton(this, style_ == STYLE_ACTION)),
      arrow_button_(new TransparentButton(this, style_ == STYLE_ACTION)),
      size_to_largest_label_(style_ == STYLE_NORMAL),
      weak_ptr_factory_(this) {
  ModelChanged();
#if defined(OS_MACOSX)
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
#else
  SetFocusBehavior(FocusBehavior::ALWAYS);
#endif

  UpdateBorder();

  // Initialize the button images.
  Button::ButtonState button_states[] = {
    Button::STATE_DISABLED,
    Button::STATE_NORMAL,
    Button::STATE_HOVERED,
    Button::STATE_PRESSED,
  };
  for (int i = 0; i < 2; i++) {
    for (size_t state_index = 0; state_index < arraysize(button_states);
         state_index++) {
      Button::ButtonState state = button_states[state_index];
      size_t num;
      bool focused = !!i;
      const int* ids = GetBodyButtonImageIds(focused, state, &num);
      body_button_painters_[focused][state] =
          Painter::CreateImageGridPainter(ids);
      menu_button_images_[focused][state] = GetMenuButtonImages(focused, state);
    }
  }

  text_button_->SetVisible(true);
  arrow_button_->SetVisible(true);
  AddChildView(text_button_);
  AddChildView(arrow_button_);

  // A layer is applied to make sure that canvas bounds are snapped to pixel
  // boundaries (for the sake of drawing the arrow).
  if (UseMd()) {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  } else {
    arrow_image_ = *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
        IDR_MENU_DROPARROW);
  }

  if (UseMd())
    focus_ring_ = FocusRing::Install(this);
}

Combobox::~Combobox() {
  if (GetInputMethod() && selector_.get()) {
    // Combobox should have been blurred before destroy.
    DCHECK(selector_.get() != GetInputMethod()->GetTextInputClient());
  }
}

// static
const gfx::FontList& Combobox::GetFontList() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetFontListWithDelta(ui::kLabelFontSizeDelta);
}

void Combobox::ModelChanged() {
  // If the selection is no longer valid (or the model is empty), restore the
  // default index.
  if (selected_index_ >= model_->GetItemCount() ||
      model_->GetItemCount() == 0 ||
      model_->IsItemSeparatorAt(selected_index_)) {
    selected_index_ = model_->GetDefaultIndex();
  }

  content_size_ = GetContentSize();
  PreferredSizeChanged();
  SchedulePaint();
}

void Combobox::SetSelectedIndex(int index) {
  if (style_ == STYLE_ACTION)
    return;

  selected_index_ = index;
  if (size_to_largest_label_) {
    SchedulePaint();
  } else {
    content_size_ = GetContentSize();
    PreferredSizeChanged();
  }
}

bool Combobox::SelectValue(const base::string16& value) {
  if (style_ == STYLE_ACTION)
    return false;

  for (int i = 0; i < model()->GetItemCount(); ++i) {
    if (value == model()->GetItemAt(i)) {
      SetSelectedIndex(i);
      return true;
    }
  }
  return false;
}

void Combobox::SetTooltipText(const base::string16& tooltip_text) {
  arrow_button_->SetTooltipText(tooltip_text);
  if (accessible_name_.empty())
    accessible_name_ = tooltip_text;
}

void Combobox::SetAccessibleName(const base::string16& name) {
  accessible_name_ = name;
}

void Combobox::SetInvalid(bool invalid) {
  if (invalid == invalid_)
    return;

  invalid_ = invalid;

  if (focus_ring_)
    focus_ring_->SetInvalid(invalid);

  UpdateBorder();
  SchedulePaint();
}

void Combobox::Layout() {
  View::Layout();

  int text_button_width = 0;
  int arrow_button_width = 0;

  switch (style_) {
    case STYLE_NORMAL: {
      arrow_button_width = width();
      break;
    }
    case STYLE_ACTION: {
      arrow_button_width = GetArrowContainerWidth();
      text_button_width = width() - arrow_button_width;
      break;
    }
  }

  int arrow_button_x = std::max(0, text_button_width);
  text_button_->SetBounds(0, 0, std::max(0, text_button_width), height());
  arrow_button_->SetBounds(arrow_button_x, 0, arrow_button_width, height());
}

void Combobox::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  if (!UseMd())
    return;

  SetBackground(
      CreateBackgroundFromPainter(Painter::CreateSolidRoundRectPainter(
          theme->GetSystemColor(
              ui::NativeTheme::kColorId_TextfieldDefaultBackground),
          FocusableBorder::kCornerRadiusDp)));
}

int Combobox::GetRowCount() {
  return model()->GetItemCount();
}

int Combobox::GetSelectedRow() {
  return selected_index_;
}

void Combobox::SetSelectedRow(int row) {
  int prev_index = selected_index_;
  SetSelectedIndex(row);
  if (selected_index_ != prev_index)
    OnPerformAction();
}

base::string16 Combobox::GetTextForRow(int row) {
  return model()->IsItemSeparatorAt(row) ? base::string16() :
                                           model()->GetItemAt(row);
}

////////////////////////////////////////////////////////////////////////////////
// Combobox, View overrides:

gfx::Size Combobox::CalculatePreferredSize() const {
  // The preferred size will drive the local bounds which in turn is used to set
  // the minimum width for the dropdown list.
  gfx::Insets insets = GetInsets();
  const LayoutProvider* provider = LayoutProvider::Get();
  insets += gfx::Insets(
      provider->GetDistanceMetric(DISTANCE_CONTROL_VERTICAL_TEXT_PADDING),
      provider->GetDistanceMetric(DISTANCE_TEXTFIELD_HORIZONTAL_TEXT_PADDING));
  int total_width = std::max(kMinComboboxWidth, content_size_.width()) +
                    insets.width() + GetArrowContainerWidth();
  return gfx::Size(total_width, content_size_.height() + insets.height());
}

const char* Combobox::GetClassName() const {
  return kViewClassName;
}

bool Combobox::SkipDefaultKeyEventProcessing(const ui::KeyEvent& e) {
  // Escape should close the drop down list when it is active, not host UI.
  if (e.key_code() != ui::VKEY_ESCAPE ||
      e.IsShiftDown() || e.IsControlDown() || e.IsAltDown()) {
    return false;
  }
  return !!menu_runner_;
}

bool Combobox::OnKeyPressed(const ui::KeyEvent& e) {
  // TODO(oshima): handle IME.
  DCHECK_EQ(e.type(), ui::ET_KEY_PRESSED);

  DCHECK_GE(selected_index_, 0);
  DCHECK_LT(selected_index_, model()->GetItemCount());
  if (selected_index_ < 0 || selected_index_ > model()->GetItemCount())
    selected_index_ = 0;

  bool show_menu = false;
  int new_index = kNoSelection;
  switch (e.key_code()) {
#if defined(OS_MACOSX)
    case ui::VKEY_DOWN:
    case ui::VKEY_UP:
    case ui::VKEY_SPACE:
    case ui::VKEY_HOME:
    case ui::VKEY_END:
      // On Mac, navigation keys should always just show the menu first.
      show_menu = true;
      break;
#else
    // Show the menu on F4 without modifiers.
    case ui::VKEY_F4:
      if (e.IsAltDown() || e.IsAltGrDown() || e.IsControlDown())
        return false;
      show_menu = true;
      break;

    // Move to the next item if any, or show the menu on Alt+Down like Windows.
    case ui::VKEY_DOWN:
      if (e.IsAltDown())
        show_menu = true;
      else
        new_index = GetAdjacentIndex(model(), 1, selected_index_);
      break;

    // Move to the end of the list.
    case ui::VKEY_END:
    case ui::VKEY_NEXT:  // Page down.
      new_index = GetAdjacentIndex(model(), -1, model()->GetItemCount());
      break;

    // Move to the beginning of the list.
    case ui::VKEY_HOME:
    case ui::VKEY_PRIOR:  // Page up.
      new_index = GetAdjacentIndex(model(), 1, -1);
      break;

    // Move to the previous item if any.
    case ui::VKEY_UP:
      new_index = GetAdjacentIndex(model(), -1, selected_index_);
      break;

    case ui::VKEY_SPACE:
      if (style_ == STYLE_ACTION) {
        // When pressing space, the click event will be raised after the key is
        // released.
        text_button_->SetState(Button::STATE_PRESSED);
      } else {
        show_menu = true;
      }
      break;

    case ui::VKEY_RETURN:
      if (style_ == STYLE_ACTION)
        OnPerformAction();
      else
        show_menu = true;
      break;
#endif  // OS_MACOSX
    default:
      return false;
  }

  if (show_menu) {
    ShowDropDownMenu(ui::MENU_SOURCE_KEYBOARD);
  } else if (new_index != selected_index_ && new_index != kNoSelection &&
             style_ != STYLE_ACTION) {
    DCHECK(!model()->IsItemSeparatorAt(new_index));
    selected_index_ = new_index;
    OnPerformAction();
  }

  return true;
}

bool Combobox::OnKeyReleased(const ui::KeyEvent& e) {
  if (style_ != STYLE_ACTION)
    return false;  // crbug.com/127520

  if (e.key_code() == ui::VKEY_SPACE && style_ == STYLE_ACTION &&
      text_button_->state() == Button::STATE_PRESSED)
    OnPerformAction();

  return false;
}

void Combobox::OnPaint(gfx::Canvas* canvas) {
  switch (style_) {
    case STYLE_NORMAL: {
      OnPaintBackground(canvas);
      PaintText(canvas);
      OnPaintBorder(canvas);
      break;
    }
    case STYLE_ACTION: {
      PaintButtons(canvas);
      PaintText(canvas);
      break;
    }
  }
}

void Combobox::OnFocus() {
  if (GetInputMethod())
    GetInputMethod()->SetFocusedTextInputClient(GetPrefixSelector());

  View::OnFocus();
  // Border renders differently when focused.
  SchedulePaint();
}

void Combobox::OnBlur() {
  if (GetInputMethod())
    GetInputMethod()->DetachTextInputClient(GetPrefixSelector());

  if (selector_)
    selector_->OnViewBlur();
  // Border renders differently when focused.
  SchedulePaint();
}

void Combobox::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  // ax::mojom::Role::kComboBox is for UI elements with a dropdown and
  // an editable text field, which views::Combobox does not have. Use
  // ax::mojom::Role::kPopUpButton to match an HTML <select> element.
  node_data->role = ax::mojom::Role::kPopUpButton;

  node_data->SetName(accessible_name_);
  node_data->SetValue(model_->GetItemAt(selected_index_));
  if (enabled()) {
    node_data->SetDefaultActionVerb(ax::mojom::DefaultActionVerb::kOpen);
  }
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kPosInSet,
                             selected_index_);
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kSetSize,
                             model_->GetItemCount());
}

bool Combobox::HandleAccessibleAction(const ui::AXActionData& action_data) {
  // The action handling in View would generate a mouse event and send it to
  // |this|. However, mouse events for Combobox are handled by |arrow_button_|,
  // which is hidden from the a11y tree (so can't expose actions). Rather than
  // forwarding ax::mojom::Action::kDoDefault to View and then forwarding the
  // mouse event it generates to |arrow_button_| to have it forward back to
  // |this| (as its ButtonListener), just handle the action explicitly here and
  // bypass View.
  if (enabled() && action_data.action == ax::mojom::Action::kDoDefault) {
    ShowDropDownMenu(ui::MENU_SOURCE_KEYBOARD);
    return true;
  }
  return View::HandleAccessibleAction(action_data);
}

void Combobox::ButtonPressed(Button* sender, const ui::Event& event) {
  if (!enabled())
    return;

  if (!UseMd())
    RequestFocus();

  if (sender == text_button_) {
    OnPerformAction();
  } else {
    DCHECK_EQ(arrow_button_, sender);
    // TODO(hajimehoshi): Fix the problem that the arrow button blinks when
    // cliking this while the dropdown menu is opened.
    const base::TimeDelta delta = base::Time::Now() - closed_time_;
    if (delta.InMilliseconds() <= kMinimumMsBetweenButtonClicks)
      return;

    ui::MenuSourceType source_type = ui::MENU_SOURCE_MOUSE;
    if (event.IsKeyEvent())
      source_type = ui::MENU_SOURCE_KEYBOARD;
    else if (event.IsGestureEvent() || event.IsTouchEvent())
      source_type = ui::MENU_SOURCE_TOUCH;
    ShowDropDownMenu(source_type);
  }
}

void Combobox::UpdateBorder() {
  std::unique_ptr<FocusableBorder> border(new FocusableBorder());
  if (style_ == STYLE_ACTION)
    border->SetInsets(5, 10, 5, 10);
  if (invalid_)
    border->SetColorId(ui::NativeTheme::kColorId_AlertSeverityHigh);
  SetBorder(std::move(border));
}

void Combobox::AdjustBoundsForRTLUI(gfx::Rect* rect) const {
  rect->set_x(GetMirroredXForRect(*rect));
}

void Combobox::PaintText(gfx::Canvas* canvas) {
  gfx::Insets insets = GetInsets();
  insets += gfx::Insets(0, LayoutProvider::Get()->GetDistanceMetric(
                               DISTANCE_TEXTFIELD_HORIZONTAL_TEXT_PADDING));

  gfx::ScopedCanvas scoped_canvas(canvas);
  canvas->ClipRect(GetContentsBounds());

  int x = insets.left();
  int y = insets.top();
  int text_height = height() - insets.height();
  SkColor text_color = GetTextColorForEnableState(*this, enabled());
  DCHECK_GE(selected_index_, 0);
  DCHECK_LT(selected_index_, model()->GetItemCount());
  if (selected_index_ < 0 || selected_index_ > model()->GetItemCount())
    selected_index_ = 0;
  base::string16 text = model()->GetItemAt(selected_index_);

  int disclosure_arrow_offset = width() - GetArrowContainerWidth();

  const gfx::FontList& font_list = Combobox::GetFontList();
  int text_width = gfx::GetStringWidth(text, font_list);
  if ((text_width + insets.width()) > disclosure_arrow_offset)
    text_width = disclosure_arrow_offset - insets.width();

  gfx::Rect text_bounds(x, y, text_width, text_height);
  AdjustBoundsForRTLUI(&text_bounds);
  canvas->DrawStringRect(text, font_list, text_color, text_bounds);

  gfx::Rect arrow_bounds(disclosure_arrow_offset, 0, GetArrowContainerWidth(),
                         height());
  arrow_bounds =
      PositionArrowWithinContainer(arrow_bounds, ArrowSize(), style_);
  AdjustBoundsForRTLUI(&arrow_bounds);

  if (UseMd()) {
    // Since this is a core piece of UI and vector icons don't handle fractional
    // scale factors particularly well, manually draw an arrow and make sure it
    // looks good at all scale factors.
    float dsf = canvas->UndoDeviceScaleFactor();
    SkScalar x = std::ceil(arrow_bounds.x() * dsf);
    SkScalar y = std::ceil(arrow_bounds.y() * dsf);
    SkScalar height = std::floor(arrow_bounds.height() * dsf);
    SkPath path;
    // This epsilon makes sure that all the aliasing pixels are slightly more
    // than half full. Otherwise, rounding issues cause some to be considered
    // slightly less than half full and come out a little lighter.
    const SkScalar kEpsilon = 0.0001f;
    path.moveTo(x - kEpsilon, y);
    path.rLineTo(height, height);
    path.rLineTo(2 * kEpsilon, 0);
    path.rLineTo(height, -height);
    path.close();
    cc::PaintFlags flags;
    SkColor arrow_color = GetTextColorForEnableState(*this, true);
    if (!enabled())
      arrow_color = SkColorSetA(arrow_color, gfx::kDisabledControlAlpha);
    flags.setColor(arrow_color);
    flags.setAntiAlias(true);
    canvas->DrawPath(path, flags);
  } else {
    canvas->DrawImageInt(arrow_image_, arrow_bounds.x(), arrow_bounds.y());
  }
}

void Combobox::PaintButtons(gfx::Canvas* canvas) {
  DCHECK(style_ == STYLE_ACTION);

  gfx::ScopedRTLFlipCanvas scoped_canvas(canvas, width());

  bool focused = HasFocus();
  const std::vector<const gfx::ImageSkia*>& arrow_button_images =
      menu_button_images_[focused][
          arrow_button_->state() == Button::STATE_HOVERED ?
          Button::STATE_NORMAL : arrow_button_->state()];

  int text_button_hover_alpha =
      text_button_->state() == Button::STATE_PRESSED ? 0 :
      static_cast<int>(static_cast<TransparentButton*>(text_button_)->
                       GetAnimationValue() * 255);
  if (text_button_hover_alpha < 255) {
    canvas->SaveLayerAlpha(255 - text_button_hover_alpha);
    Painter* text_button_painter =
        body_button_painters_[focused][
            text_button_->state() == Button::STATE_HOVERED ?
            Button::STATE_NORMAL : text_button_->state()].get();
    Painter::PaintPainterAt(canvas, text_button_painter,
                            gfx::Rect(0, 0, text_button_->width(), height()));
    canvas->Restore();
  }
  if (0 < text_button_hover_alpha) {
    canvas->SaveLayerAlpha(text_button_hover_alpha);
    Painter* text_button_hovered_painter =
        body_button_painters_[focused][Button::STATE_HOVERED].get();
    Painter::PaintPainterAt(canvas, text_button_hovered_painter,
                            gfx::Rect(0, 0, text_button_->width(), height()));
    canvas->Restore();
  }

  int arrow_button_hover_alpha =
      arrow_button_->state() == Button::STATE_PRESSED ? 0 :
      static_cast<int>(static_cast<TransparentButton*>(arrow_button_)->
                       GetAnimationValue() * 255);
  if (arrow_button_hover_alpha < 255) {
    canvas->SaveLayerAlpha(255 - arrow_button_hover_alpha);
    PaintArrowButton(canvas, arrow_button_images, arrow_button_->x(), height());
    canvas->Restore();
  }
  if (0 < arrow_button_hover_alpha) {
    canvas->SaveLayerAlpha(arrow_button_hover_alpha);
    const std::vector<const gfx::ImageSkia*>& arrow_button_hovered_images =
        menu_button_images_[focused][Button::STATE_HOVERED];
    PaintArrowButton(canvas, arrow_button_hovered_images,
                     arrow_button_->x(), height());
    canvas->Restore();
  }
}

void Combobox::ShowDropDownMenu(ui::MenuSourceType source_type) {
  gfx::Rect lb = GetLocalBounds();
  gfx::Point menu_position(lb.origin());

  if (style_ == STYLE_NORMAL) {
    // Inset the menu's requested position so the border of the menu lines up
    // with the border of the combobox.
    menu_position.set_x(menu_position.x() + kMenuBorderWidthLeft);
    menu_position.set_y(menu_position.y() + kMenuBorderWidthTop);
  }
  lb.set_width(lb.width() - (kMenuBorderWidthLeft + kMenuBorderWidthRight));

  View::ConvertPointToScreen(this, &menu_position);

  gfx::Rect bounds(menu_position, lb.size());

  Button::ButtonState original_state = Button::STATE_NORMAL;
  if (arrow_button_) {
    original_state = arrow_button_->state();
    arrow_button_->SetState(Button::STATE_PRESSED);
  }
  MenuAnchorPosition anchor_position =
      style_ == STYLE_ACTION ? MENU_ANCHOR_TOPRIGHT : MENU_ANCHOR_TOPLEFT;

  // Allow |menu_runner_| to be set by the testing API, but if this method is
  // ever invoked recursively, ensure the old menu is closed.
  if (!menu_runner_ || menu_runner_->IsRunning()) {
    menu_runner_.reset(
        new MenuRunner(menu_model_.get(), MenuRunner::COMBOBOX,
                       base::Bind(&Combobox::OnMenuClosed,
                                  base::Unretained(this), original_state)));
  }
  menu_runner_->RunMenuAt(GetWidget(), nullptr, bounds, anchor_position,
                          source_type);
}

void Combobox::OnMenuClosed(Button::ButtonState original_button_state) {
  menu_runner_.reset();
  if (arrow_button_)
    arrow_button_->SetState(original_button_state);
  closed_time_ = base::Time::Now();
}

void Combobox::OnPerformAction() {
  NotifyAccessibilityEvent(ax::mojom::Event::kValueChanged, true);
  SchedulePaint();

  // This combobox may be deleted by the listener.
  base::WeakPtr<Combobox> weak_ptr = weak_ptr_factory_.GetWeakPtr();
  if (listener_)
    listener_->OnPerformAction(this);

  if (weak_ptr && style_ == STYLE_ACTION)
    selected_index_ = 0;
}

gfx::Size Combobox::ArrowSize() const {
  return UseMd() ? gfx::Size(8, 4) : arrow_image_.size();
}

gfx::Size Combobox::GetContentSize() const {
  const gfx::FontList& font_list = GetFontList();

  int width = 0;
  for (int i = 0; i < model()->GetItemCount(); ++i) {
    if (model_->IsItemSeparatorAt(i))
      continue;

    if (size_to_largest_label_ || i == selected_index_) {
      width = std::max(
          width, gfx::GetStringWidth(menu_model_->GetLabelAt(i), font_list));
    }
  }
  return gfx::Size(width, font_list.GetHeight());
}

PrefixSelector* Combobox::GetPrefixSelector() {
  if (!selector_)
    selector_.reset(new PrefixSelector(this, this));
  return selector_.get();
}

int Combobox::GetArrowContainerWidth() const {
  constexpr int kMdPaddingWidth = 8;
  constexpr int kNormalPaddingWidth = 7;
  int arrow_pad = UseMd() ? kMdPaddingWidth : kNormalPaddingWidth;
  int padding = style_ == STYLE_NORMAL
                    ? arrow_pad * 2
                    : kActionLeftPadding + kActionRightPadding;
  return ArrowSize().width() + padding;
}

}  // namespace views
