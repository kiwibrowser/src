// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/ime_menu/ime_menu_tray.h"

#include "ash/accessibility/accessibility_controller.h"
#include "ash/ime/ime_controller.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/ime_menu/ime_list_view.h"
#include "ash/system/tray/system_menu_button.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_container.h"
#include "ash/system/tray/tray_popup_item_style.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tray_utils.h"
#include "ash/virtual_keyboard_controller.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "components/session_manager/session_manager_types.h"
#include "ui/base/ime/ime_bridge.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/range/range.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

// Used for testing.
const int kEmojiButtonId = 1;

// Returns the height range of ImeListView.
gfx::Range GetImeListViewRange() {
  const int max_items = 5;
  const int min_items = 1;
  const int tray_item_height = kTrayPopupItemMinHeight;
  return gfx::Range(tray_item_height * min_items, tray_item_height * max_items);
}

// Shows language and input settings page.
void ShowIMESettings() {
  base::RecordAction(base::UserMetricsAction("StatusArea_IME_Detailed"));
  Shell::Get()->system_tray_controller()->ShowIMESettings();
}

// Returns true if the current screen is login or lock screen.
bool IsInLoginOrLockScreen() {
  using session_manager::SessionState;
  SessionState state = Shell::Get()->session_controller()->GetSessionState();
  return state == SessionState::LOGIN_PRIMARY ||
         state == SessionState::LOCKED ||
         state == SessionState::LOGIN_SECONDARY;
}

// Returns true if the current input context type is password.
bool IsInPasswordInputContext() {
  return ui::IMEBridge::Get()->GetCurrentInputContext().type ==
         ui::TEXT_INPUT_TYPE_PASSWORD;
}

class ImeMenuLabel : public views::Label {
 public:
  ImeMenuLabel() {
    // Sometimes the label will be more than 2 characters, e.g. INTL and EXTD.
    // This border makes sure we only leave room for ~2 and the others are
    // truncated.
    SetBorder(views::CreateEmptyBorder(gfx::Insets(0, 6)));
  }
  ~ImeMenuLabel() override = default;

  // views:Label:
  gfx::Size CalculatePreferredSize() const override {
    return gfx::Size(kTrayItemSize, kTrayItemSize);
  }
  int GetHeightForWidth(int width) const override { return kTrayItemSize; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ImeMenuLabel);
};

SystemMenuButton* CreateImeMenuButton(views::ButtonListener* listener,
                                      const gfx::VectorIcon& icon,
                                      int accessible_name_id,
                                      int right_border) {
  return new SystemMenuButton(listener, icon, accessible_name_id);
}

// The view that contains IME menu title.
class ImeTitleView : public views::View, public views::ButtonListener {
 public:
  explicit ImeTitleView(bool show_settings_button) : settings_button_(nullptr) {
    SetBorder(views::CreatePaddedBorder(
        views::CreateSolidSidedBorder(0, 0, kSeparatorWidth, 0,
                                      kMenuSeparatorColor),
        gfx::Insets(kMenuSeparatorVerticalPadding - kSeparatorWidth, 0)));
    auto box_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal);
    box_layout->set_minimum_cross_axis_size(kTrayPopupItemMinHeight);
    views::BoxLayout* layout_ptr = SetLayoutManager(std::move(box_layout));
    auto* title_label =
        new views::Label(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_IME));
    title_label->SetBorder(
        views::CreateEmptyBorder(0, kMenuEdgeEffectivePadding, 1, 0));
    title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    TrayPopupItemStyle style(TrayPopupItemStyle::FontStyle::TITLE);
    style.SetupLabel(title_label);

    AddChildView(title_label);
    layout_ptr->SetFlexForView(title_label, 1);

    if (show_settings_button) {
      settings_button_ = CreateImeMenuButton(
          this, kSystemMenuSettingsIcon, IDS_ASH_STATUS_TRAY_IME_SETTINGS, 0);
      if (!TrayPopupUtils::CanOpenWebUISettings())
        settings_button_->SetEnabled(false);
      AddChildView(settings_button_);
    }
  }

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    DCHECK_EQ(sender, settings_button_);
    ShowIMESettings();
  }

  ~ImeTitleView() override = default;

 private:
  // Settings button that is only used if the emoji, handwriting and voice
  // buttons are not available.
  SystemMenuButton* settings_button_;

  DISALLOW_COPY_AND_ASSIGN(ImeTitleView);
};

// The view that contains buttons shown on the bottom of IME menu.
class ImeButtonsView : public views::View, public views::ButtonListener {
 public:
  ImeButtonsView(ImeMenuTray* ime_menu_tray,
                 bool show_emoji,
                 bool show_handwriting,
                 bool show_voice)
      : ime_menu_tray_(ime_menu_tray) {
    DCHECK(ime_menu_tray_);

    Init(show_emoji, show_handwriting, show_voice);
  }

  ~ImeButtonsView() override = default;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    if (sender == settings_button_) {
      ime_menu_tray_->CloseBubble();
      ShowIMESettings();
      return;
    }

    // The |keyset| will be used for drawing input view keyset in IME
    // extensions. ImeMenuTray::ShowKeyboardWithKeyset() will deal with
    // the |keyset| string to generate the right input view url.
    using chromeos::input_method::mojom::ImeKeyset;
    ImeKeyset keyset = ImeKeyset::kNone;
    if (sender == emoji_button_)
      keyset = ImeKeyset::kEmoji;
    else if (sender == voice_button_)
      keyset = ImeKeyset::kVoice;
    else if (sender == handwriting_button_)
      keyset = ImeKeyset::kHandwriting;
    else
      NOTREACHED();

    // TODO(dcheng): When https://crbug.com/742517 is fixed, Mojo will generate
    // a constant for the number of values in the enum. For now, we just define
    // it here and keep it in sync with the enum.
    const int kImeKeysetUmaBoundary = 4;
    UMA_HISTOGRAM_ENUMERATION("InputMethod.ImeMenu.EmojiHandwritingVoiceButton",
                              keyset, kImeKeysetUmaBoundary);
    ime_menu_tray_->ShowKeyboardWithKeyset(keyset);
  }

 private:
  void Init(bool show_emoji, bool show_handwriting, bool show_voice) {
    auto box_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal);
    box_layout->set_minimum_cross_axis_size(kTrayPopupItemMinHeight);
    SetLayoutManager(std::move(box_layout));
    SetBorder(views::CreatePaddedBorder(
        views::CreateSolidSidedBorder(kSeparatorWidth, 0, 0, 0,
                                      kMenuSeparatorColor),
        gfx::Insets(kMenuSeparatorVerticalPadding - kSeparatorWidth,
                    kMenuExtraMarginFromLeftEdge)));

    const int right_border = 1;
    if (show_emoji) {
      emoji_button_ =
          CreateImeMenuButton(this, kImeMenuEmoticonIcon,
                              IDS_ASH_STATUS_TRAY_IME_EMOJI, right_border);
      emoji_button_->set_id(kEmojiButtonId);
      AddChildView(emoji_button_);
    }

    if (show_handwriting) {
      handwriting_button_ = CreateImeMenuButton(
          this, kImeMenuWriteIcon, IDS_ASH_STATUS_TRAY_IME_HANDWRITING,
          right_border);
      AddChildView(handwriting_button_);
    }

    if (show_voice) {
      voice_button_ =
          CreateImeMenuButton(this, kImeMenuMicrophoneIcon,
                              IDS_ASH_STATUS_TRAY_IME_VOICE, right_border);
      AddChildView(voice_button_);
    }

    settings_button_ = CreateImeMenuButton(this, kSystemMenuSettingsIcon,
                                           IDS_ASH_STATUS_TRAY_IME_SETTINGS, 0);
    AddChildView(settings_button_);
    if (!TrayPopupUtils::CanOpenWebUISettings())
      settings_button_->SetEnabled(false);
  }

  ImeMenuTray* ime_menu_tray_;
  SystemMenuButton* emoji_button_;
  SystemMenuButton* handwriting_button_;
  SystemMenuButton* voice_button_;
  SystemMenuButton* settings_button_;

  DISALLOW_COPY_AND_ASSIGN(ImeButtonsView);
};

// A list of available IMEs shown in the opt-in IME menu, which has a different
// height depending on the number of IMEs in the list.
class ImeMenuListView : public ImeListView {
 public:
  ImeMenuListView(DetailedViewDelegate* delegate) : ImeListView(delegate) {
    set_should_focus_ime_after_selection_with_keyboard(true);
  }

  ~ImeMenuListView() override = default;

 protected:
  void Layout() override {
    gfx::Range height_range = GetImeListViewRange();
    scroller()->ClipHeightTo(height_range.start(), height_range.end());
    ImeListView::Layout();
  }

  DISALLOW_COPY_AND_ASSIGN(ImeMenuListView);
};

}  // namespace

ImeMenuTray::ImeMenuTray(Shelf* shelf)
    : TrayBackgroundView(shelf),
      ime_controller_(Shell::Get()->ime_controller()),
      label_(new ImeMenuLabel()),
      keyboard_suppressed_(false),
      show_bubble_after_keyboard_hidden_(false),
      is_emoji_enabled_(false),
      is_handwriting_enabled_(false),
      is_voice_enabled_(false),
      weak_ptr_factory_(this) {
  DCHECK(ime_controller_);
  SetInkDropMode(InkDropMode::ON);
  SetupLabelForTray(label_);
  label_->SetElideBehavior(gfx::TRUNCATE);
  label_->SetTooltipText(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_IME));
  tray_container()->AddChildView(label_);
  SystemTrayNotifier* tray_notifier = Shell::Get()->system_tray_notifier();
  tray_notifier->AddIMEObserver(this);
  tray_notifier->AddVirtualKeyboardObserver(this);
}

ImeMenuTray::~ImeMenuTray() {
  if (bubble_)
    bubble_->bubble_view()->ResetDelegate();
  SystemTrayNotifier* tray_notifier = Shell::Get()->system_tray_notifier();
  tray_notifier->RemoveIMEObserver(this);
  tray_notifier->RemoveVirtualKeyboardObserver(this);
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (keyboard_controller)
    keyboard_controller->RemoveObserver(this);
}

void ImeMenuTray::ShowImeMenuBubbleInternal(bool show_by_click) {
  views::TrayBubbleView::InitParams init_params;
  init_params.delegate = this;
  init_params.parent_window = GetBubbleWindowContainer();
  init_params.anchor_view = GetBubbleAnchor();
  init_params.anchor_alignment = GetAnchorAlignment();
  init_params.min_width = kTrayMenuWidth;
  init_params.max_width = kTrayMenuWidth;
  init_params.close_on_deactivate = true;
  init_params.show_by_click = show_by_click;

  views::TrayBubbleView* bubble_view = new views::TrayBubbleView(init_params);
  bubble_view->set_anchor_view_insets(GetBubbleAnchorInsets());

  // Add a title item with a separator on the top of the IME menu.
  bool show_bottom_buttons = ShouldShowBottomButtons();
  bubble_view->AddChildView(new ImeTitleView(!show_bottom_buttons));

  // Adds IME list to the bubble.
  ime_list_view_ = new ImeMenuListView(nullptr);
  ime_list_view_->Init(ShouldShowKeyboardToggle(),
                       ImeListView::SHOW_SINGLE_IME);
  bubble_view->AddChildView(ime_list_view_);

  if (show_bottom_buttons) {
    bubble_view->AddChildView(new ImeButtonsView(
        this, is_emoji_enabled_, is_handwriting_enabled_, is_voice_enabled_));
  }

  bubble_ = std::make_unique<TrayBubbleWrapper>(this, bubble_view,
                                                false /* is_persistent */);
  SetIsActive(true);
}

void ImeMenuTray::ShowKeyboardWithKeyset(
    chromeos::input_method::mojom::ImeKeyset keyset) {
  CloseBubble();

  Shell::Get()->virtual_keyboard_controller()->ForceShowKeyboardWithKeyset(
      keyset);
}

bool ImeMenuTray::ShouldShowBottomButtons() {
  // Emoji, handwriting and voice input is not supported for these cases:
  // 1) features::kEHVInputOnImeMenu is not enabled.
  // 2) third party IME extensions.
  // 3) login/lock screen.
  // 4) password input client.

  bool should_show_buttom_buttoms =
      ime_controller_->is_extra_input_options_enabled() &&
      !ime_controller_->current_ime().third_party && !IsInLoginOrLockScreen() &&
      !IsInPasswordInputContext();

  if (!should_show_buttom_buttoms) {
    is_emoji_enabled_ = is_handwriting_enabled_ = is_voice_enabled_ = false;
    return false;
  }

  is_emoji_enabled_ = ime_controller_->is_emoji_enabled();
  is_handwriting_enabled_ = ime_controller_->is_handwriting_enabled();
  is_voice_enabled_ = ime_controller_->is_voice_enabled();

  return is_emoji_enabled_ || is_handwriting_enabled_ || is_voice_enabled_;
}

bool ImeMenuTray::ShouldShowKeyboardToggle() const {
  return keyboard_suppressed_ &&
         !Shell::Get()->accessibility_controller()->IsVirtualKeyboardEnabled();
}

base::string16 ImeMenuTray::GetAccessibleNameForTray() {
  return l10n_util::GetStringUTF16(IDS_ASH_IME_MENU_ACCESSIBLE_NAME);
}

void ImeMenuTray::HideBubbleWithView(const views::TrayBubbleView* bubble_view) {
  if (bubble_->bubble_view() == bubble_view)
    CloseBubble();
}

void ImeMenuTray::ClickedOutsideBubble() {
  CloseBubble();
}

bool ImeMenuTray::PerformAction(const ui::Event& event) {
  UserMetricsRecorder::RecordUserClickOnTray(
      LoginMetricsRecorder::TrayClickTarget::kImeTray);
  if (bubble_)
    CloseBubble();
  else
    ShowBubble(event.IsMouseEvent() || event.IsGestureEvent());
  return true;
}

void ImeMenuTray::CloseBubble() {
  bubble_.reset();
  ime_list_view_ = nullptr;
  SetIsActive(false);
  shelf()->UpdateAutoHideState();
}

void ImeMenuTray::ShowBubble(bool show_by_click) {
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (keyboard_controller && keyboard_controller->keyboard_visible()) {
    show_bubble_after_keyboard_hidden_ = true;
    keyboard_controller->AddObserver(this);
    keyboard_controller->HideKeyboard(
        keyboard::KeyboardController::HIDE_REASON_AUTOMATIC);
  } else {
    base::RecordAction(base::UserMetricsAction("Tray_ImeMenu_Opened"));
    ShowImeMenuBubbleInternal(show_by_click);
  }
}

views::TrayBubbleView* ImeMenuTray::GetBubbleView() {
  return bubble_ ? bubble_->bubble_view() : nullptr;
}

void ImeMenuTray::OnIMERefresh() {
  UpdateTrayLabel();
  if (bubble_ && ime_list_view_) {
    ime_list_view_->Update(ime_controller_->current_ime().id,
                           ime_controller_->available_imes(),
                           ime_controller_->current_ime_menu_items(), false,
                           ImeListView::SHOW_SINGLE_IME);
  }
}

void ImeMenuTray::OnIMEMenuActivationChanged(bool is_activated) {
  SetVisible(is_activated);
  if (is_activated)
    UpdateTrayLabel();
  else
    CloseBubble();
}

void ImeMenuTray::BubbleViewDestroyed() {
}

void ImeMenuTray::OnMouseEnteredView() {}

void ImeMenuTray::OnMouseExitedView() {}

base::string16 ImeMenuTray::GetAccessibleNameForBubble() {
  return l10n_util::GetStringUTF16(IDS_ASH_IME_MENU_ACCESSIBLE_NAME);
}

bool ImeMenuTray::ShouldEnableExtraKeyboardAccessibility() {
  return Shell::Get()->accessibility_controller()->IsSpokenFeedbackEnabled();
}

void ImeMenuTray::HideBubble(const views::TrayBubbleView* bubble_view) {
  HideBubbleWithView(bubble_view);
}

void ImeMenuTray::OnKeyboardHidden() {
  if (show_bubble_after_keyboard_hidden_) {
    show_bubble_after_keyboard_hidden_ = false;
    keyboard::KeyboardController* keyboard_controller =
        keyboard::KeyboardController::GetInstance();
    DCHECK(keyboard_controller);
    keyboard_controller->RemoveObserver(this);

    ShowImeMenuBubbleInternal(false /* show_by_click */);
    return;
  }
}

void ImeMenuTray::OnKeyboardSuppressionChanged(bool suppressed) {
  if (suppressed != keyboard_suppressed_ && bubble_)
    CloseBubble();
  keyboard_suppressed_ = suppressed;
}

void ImeMenuTray::UpdateTrayLabel() {
  const mojom::ImeInfo& current_ime = ime_controller_->current_ime();

  // Updates the tray label based on the current input method.
  if (current_ime.third_party)
    label_->SetText(current_ime.short_name + base::UTF8ToUTF16("*"));
  else
    label_->SetText(current_ime.short_name);
}

}  // namespace ash
