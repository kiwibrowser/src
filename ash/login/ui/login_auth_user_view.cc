// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_auth_user_view.h"

#include <map>
#include <memory>

#include "ash/login/login_screen_controller.h"
#include "ash/login/ui/layout_util.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_display_style.h"
#include "ash/login/ui/login_password_view.h"
#include "ash/login/ui/login_pin_view.h"
#include "ash/login/ui/login_user_view.h"
#include "ash/login/ui/non_accessible_view.h"
#include "ash/login/ui/pin_keyboard_animation.h"
#include "ash/public/cpp/login_constants.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/night_light/time_of_day.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "base/strings/utf_string_conversions.h"
#include "components/user_manager/user.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/callback_layer_animation_observer.h"
#include "ui/compositor/layer_animation_sequence.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_analysis.h"
#include "ui/gfx/interpolated_transform.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"

namespace ash {
namespace {

constexpr const char kLoginAuthUserViewClassName[] = "LoginAuthUserView";

// Distance between the user view (ie, the icon and name) and the password
// textfield.
const int kDistanceBetweenUserViewAndPasswordDp = 28;

// Distance between the password textfield and the the pin keyboard.
const int kDistanceBetweenPasswordFieldAndPinKeyboard = 20;

// Distance from the end of pin keyboard to the bottom of the big user view.
const int kDistanceFromPinKeyboardToBigUserViewBottom = 50;

// Distance from the top of the user view to the user icon.
constexpr int kDistanceFromTopOfBigUserViewToUserIconDp = 54;

// The color of the online sign-in message text.
constexpr SkColor kOnlineSignInMessageColor = SkColorSetRGB(0xE6, 0x7C, 0x73);

constexpr SkColor kFingerprintIconViewBorderColor =
    SkColorSetARGB(0x57, 0xFF, 0xFF, 0xFF);
constexpr SkColor kFingerprintIconAndTextColor =
    SkColorSetARGB(0x8A, 0xFF, 0xFF, 0xFF);
constexpr int kFingerprintIconViewBorderThickness = 1;
constexpr int kFingerprintIconViewSizeDp = 64;
constexpr int kFingerprintIconSizeDp = 32;
constexpr int kResetToDefaultIconColorDelayMs = 500;
constexpr int kFingerprintIconTopSpacing = 50;
constexpr int kSpacingBetweenFingerprintIconAndLabel = 20;

constexpr int kDisabledAuthMessageVerticalBorderDp = 14;
constexpr int kDisabledAuthMessageHorizontalBorderDp = 0;
constexpr int kDisabledAuthMessageChildrenSpacingDp = 4;
constexpr int kDisabledAuthMessageWidthDp = 204;
constexpr int kDisabledAuthMessageHeightDp = 98;
constexpr int kDisabledAuthMessageIconSizeDp = 24;
constexpr int kDisabledAuthMessageTitleFontSizeDeltaDp = 3;
constexpr int kDisabledAuthMessageContentsFontSizeDeltaDp = -1;
constexpr int kDisabledAuthMessageRoundedCornerRadiusDp = 8;

// Returns an observer that will hide |view| when it fires. The observer will
// delete itself after firing. Make sure to call |observer->SetReady()| after
// attaching it.
ui::CallbackLayerAnimationObserver* BuildObserverToHideView(views::View* view) {
  return new ui::CallbackLayerAnimationObserver(base::Bind(
      [](views::View* view,
         const ui::CallbackLayerAnimationObserver& observer) {
        // Don't hide the view if the animation is aborted, as |view| may no
        // longer be valid.
        if (observer.aborted_count())
          return true;

        view->SetVisible(false);
        return true;
      },
      view));
}

// A view which has a round border and a fingerprint icon at the center.
class FingerprintIconView : public views::View {
 public:
  FingerprintIconView() {
    SetPreferredSize(
        gfx::Size(kFingerprintIconViewSizeDp, kFingerprintIconViewSizeDp));
    icon_ = new views::ImageView;
    icon_->SetVerticalAlignment(views::ImageView::CENTER);
    icon_->SetPreferredSize(
        gfx::Size(kFingerprintIconSizeDp, kFingerprintIconSizeDp));
    icon_->SetImage(gfx::CreateVectorIcon(kLockScreenFingerprintIcon,
                                          kFingerprintIconSizeDp, color_));
    AddChildView(icon_);
    SetBorder(views::CreateRoundedRectBorder(
        kFingerprintIconViewBorderThickness, kFingerprintIconViewSizeDp / 2,
        kFingerprintIconViewBorderColor));
  }

  ~FingerprintIconView() override = default;

  // Set color of the icon. The color will be reset to
  // kFingerprintIconAndTextColor after a short period if different.
  void SetIconColor(SkColor color) {
    if (color_ == color)
      return;
    color_ = color;
    reset_icon_color_.Stop();
    icon_->SetImage(gfx::CreateVectorIcon(kLockScreenFingerprintIcon,
                                          kFingerprintIconSizeDp, color));

    if (color_ != kFingerprintIconAndTextColor) {
      reset_icon_color_.Start(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kResetToDefaultIconColorDelayMs),
          base::BindRepeating(&FingerprintIconView::SetIconColor,
                              base::Unretained(this),
                              kFingerprintIconAndTextColor));
    }
  }

  void Layout() override {
    gfx::Rect icon_bounds = GetContentsBounds();
    icon_bounds.ClampToCenteredSize(
        gfx::Size(kFingerprintIconSizeDp, kFingerprintIconSizeDp));
    icon_->SetBoundsRect(icon_bounds);
  }

 private:
  views::ImageView* icon_ = nullptr;
  base::OneShotTimer reset_icon_color_;
  SkColor color_ = kFingerprintIconAndTextColor;

  DISALLOW_COPY_AND_ASSIGN(FingerprintIconView);
};

}  // namespace

// Consists of fingerprint icon view and a label.
class LoginAuthUserView::FingerprintView : public views::View {
 public:
  FingerprintView() {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);

    icon_view_ = new FingerprintIconView();
    AddChildView(icon_view_);
    label_ = new views::Label(
        l10n_util::GetStringUTF16(IDS_ASH_LOGIN_FINGERPRINT_UNLOCK_MESSAGE));
    label_->SetSubpixelRenderingEnabled(false);
    label_->SetAutoColorReadabilityEnabled(false);
    label_->SetEnabledColor(kFingerprintIconAndTextColor);
    AddChildView(label_);
  }

  void SetIconColor(SkColor color) { icon_view_->SetIconColor(color); }

  void Layout() override {
    gfx::Rect bounds = GetContentsBounds();
    icon_view_->SizeToPreferredSize();
    icon_view_->SetPosition(
        gfx::Point((bounds.width() - icon_view_->width()) / 2,
                   bounds.y() + kFingerprintIconTopSpacing));
    label_->SizeToPreferredSize();
    label_->SetPosition(
        gfx::Point(bounds.x(), icon_view_->bounds().bottom() +
                                   kSpacingBetweenFingerprintIconAndLabel));
  }

  ~FingerprintView() override = default;

  gfx::Size CalculatePreferredSize() const override {
    int preferred_height = label_->GetPreferredSize().height() +
                           icon_view_->GetPreferredSize().height() +
                           kFingerprintIconTopSpacing +
                           kSpacingBetweenFingerprintIconAndLabel;
    int preferred_width = std::max(label_->GetPreferredSize().width(),
                                   icon_view_->GetPreferredSize().width());
    return gfx::Size(preferred_width, preferred_height);
  }

 private:
  FingerprintIconView* icon_view_ = nullptr;
  views::Label* label_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(FingerprintView);
};

// The message shown to user when the auth method is |AUTH_DISABLED|.
class LoginAuthUserView::DisabledAuthMessageView : public views::View {
 public:
  DisabledAuthMessageView() {
    SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::kVertical,
        gfx::Insets(kDisabledAuthMessageVerticalBorderDp,
                    kDisabledAuthMessageHorizontalBorderDp),
        kDisabledAuthMessageChildrenSpacingDp));
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    SetPreferredSize(
        gfx::Size(kDisabledAuthMessageWidthDp, kDisabledAuthMessageHeightDp));
    views::ImageView* alarm_clock_icon = new views::ImageView();
    alarm_clock_icon->SetPreferredSize(gfx::Size(
        kDisabledAuthMessageIconSizeDp, kDisabledAuthMessageIconSizeDp));
    alarm_clock_icon->SetImage(
        gfx::CreateVectorIcon(kLockScreenTimeLimitAlarmIcon, SK_ColorWHITE));
    AddChildView(alarm_clock_icon);

    auto decorate_label = [](views::Label* label) {
      label->SetSubpixelRenderingEnabled(false);
      label->SetAutoColorReadabilityEnabled(false);
      label->SetEnabledColor(SK_ColorWHITE);
    };
    views::Label* message_title = new views::Label(
        l10n_util::GetStringUTF16(IDS_ASH_LOGIN_TAKE_BREAK_MESSAGE),
        views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY);
    message_title->SetFontList(
        gfx::FontList().Derive(kDisabledAuthMessageTitleFontSizeDeltaDp,
                               gfx::Font::NORMAL, gfx::Font::Weight::MEDIUM));
    decorate_label(message_title);
    AddChildView(message_title);

    message_contents_ =
        new views::Label(base::string16(), views::style::CONTEXT_LABEL,
                         views::style::STYLE_PRIMARY);
    message_contents_->SetFontList(
        gfx::FontList().Derive(kDisabledAuthMessageContentsFontSizeDeltaDp,
                               gfx::Font::NORMAL, gfx::Font::Weight::NORMAL));
    decorate_label(message_contents_);
    AddChildView(message_contents_);
  }

  ~DisabledAuthMessageView() override = default;

  // Set the time when auth will be reenabled. It will be included in the
  // message.
  void SetAuthReenabledTime(const base::Time& auth_reenabled_time) {
    const std::string time_of_day =
        TimeOfDay::FromTime(auth_reenabled_time).ToString();
    message_contents_->SetText(l10n_util::GetStringFUTF16(
        IDS_ASH_LOGIN_COME_BACK_MESSAGE, base::UTF8ToUTF16(time_of_day)));
    Layout();
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    views::View::OnPaint(canvas);

    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setColor(Shell::Get()->wallpaper_controller()->GetProminentColor(
        color_utils::ColorProfile(color_utils::LumaRange::DARK,
                                  color_utils::SaturationRange::MUTED)));
    canvas->DrawRoundRect(GetContentsBounds(),
                          kDisabledAuthMessageRoundedCornerRadiusDp, flags);
  }

 private:
  views::Label* message_contents_;

  DISALLOW_COPY_AND_ASSIGN(DisabledAuthMessageView);
};

struct LoginAuthUserView::AnimationState {
  int non_pin_y_start_in_screen = 0;
  gfx::Point pin_start_in_screen;
  bool had_pin = false;
  bool had_password = false;

  explicit AnimationState(LoginAuthUserView* view) {
    non_pin_y_start_in_screen = view->GetBoundsInScreen().y();
    pin_start_in_screen = view->pin_view_->GetBoundsInScreen().origin();

    had_pin = (view->auth_methods() & LoginAuthUserView::AUTH_PIN) != 0;
    had_password =
        (view->auth_methods() & LoginAuthUserView::AUTH_PASSWORD) != 0;
  }
};

LoginAuthUserView::TestApi::TestApi(LoginAuthUserView* view) : view_(view) {}

LoginAuthUserView::TestApi::~TestApi() = default;

LoginUserView* LoginAuthUserView::TestApi::user_view() const {
  return view_->user_view_;
}

LoginPasswordView* LoginAuthUserView::TestApi::password_view() const {
  return view_->password_view_;
}

LoginPinView* LoginAuthUserView::TestApi::pin_view() const {
  return view_->pin_view_;
}

views::Button* LoginAuthUserView::TestApi::online_sign_in_message() const {
  return view_->online_sign_in_message_;
}

views::View* LoginAuthUserView::TestApi::disabled_auth_message() const {
  return view_->disabled_auth_message_;
}

LoginAuthUserView::Callbacks::Callbacks() = default;

LoginAuthUserView::Callbacks::Callbacks(const Callbacks& other) = default;

LoginAuthUserView::Callbacks::~Callbacks() = default;

LoginAuthUserView::LoginAuthUserView(const mojom::LoginUserInfoPtr& user,
                                     const Callbacks& callbacks)
    : NonAccessibleView(kLoginAuthUserViewClassName),
      on_auth_(callbacks.on_auth),
      on_tap_(callbacks.on_tap),
      weak_factory_(this) {
  DCHECK(callbacks.on_auth);
  DCHECK(callbacks.on_tap);
  DCHECK(callbacks.on_remove_warning_shown);
  DCHECK(callbacks.on_remove);
  DCHECK(callbacks.on_easy_unlock_icon_hovered);
  DCHECK(callbacks.on_easy_unlock_icon_tapped);
  DCHECK_NE(user->basic_user_info->type,
            user_manager::USER_TYPE_PUBLIC_ACCOUNT);

  // Build child views.
  user_view_ = new LoginUserView(
      LoginDisplayStyle::kLarge, true /*show_dropdown*/, false /*show_domain*/,
      base::BindRepeating(&LoginAuthUserView::OnUserViewTap,
                          base::Unretained(this)),
      callbacks.on_remove_warning_shown, callbacks.on_remove);

  password_view_ = new LoginPasswordView();
  password_view_->SetPaintToLayer();  // Needed for opacity animation.
  password_view_->layer()->SetFillsBoundsOpaquely(false);
  password_view_->UpdateForUser(user);

  pin_view_ =
      new LoginPinView(base::BindRepeating(&LoginPasswordView::InsertNumber,
                                           base::Unretained(password_view_)),
                       base::BindRepeating(&LoginPasswordView::Backspace,
                                           base::Unretained(password_view_)));
  DCHECK(pin_view_->layer());

  // Initialization of |password_view_| is deferred because it needs the
  // |pin_view_| pointer.
  password_view_->Init(
      base::Bind(&LoginAuthUserView::OnAuthSubmit, base::Unretained(this)),
      base::Bind(&LoginPinView::OnPasswordTextChanged,
                 base::Unretained(pin_view_)),
      callbacks.on_easy_unlock_icon_hovered,
      callbacks.on_easy_unlock_icon_tapped);

  online_sign_in_message_ = new views::LabelButton(
      this, base::UTF8ToUTF16(user->basic_user_info->display_name));
  DecorateOnlineSignInMessage();

  disabled_auth_message_ = new DisabledAuthMessageView();

  fingerprint_view_ = new FingerprintView();

  SetPaintToLayer(ui::LayerType::LAYER_NOT_DRAWN);

  // Build layout.
  auto* wrapped_password_view =
      login_layout_util::WrapViewForPreferredSize(password_view_);
  auto* wrapped_online_sign_in_message_view =
      login_layout_util::WrapViewForPreferredSize(online_sign_in_message_);
  auto* wrapped_disabled_auth_message_view =
      login_layout_util::WrapViewForPreferredSize(disabled_auth_message_);
  auto* wrapped_user_view =
      login_layout_util::WrapViewForPreferredSize(user_view_);
  auto* wrapped_pin_view =
      login_layout_util::WrapViewForPreferredSize(pin_view_);
  auto* wrapped_fingerprint_view =
      login_layout_util::WrapViewForPreferredSize(fingerprint_view_);

  // Add views in tabbing order; they are rendered in a different order below.
  AddChildView(wrapped_password_view);
  AddChildView(wrapped_online_sign_in_message_view);
  AddChildView(wrapped_disabled_auth_message_view);
  AddChildView(wrapped_fingerprint_view);
  AddChildView(wrapped_pin_view);
  AddChildView(wrapped_user_view);

  // Use views::GridLayout instead of views::BoxLayout because views::BoxLayout
  // lays out children according to the view->children order.
  views::GridLayout* grid_layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));
  views::ColumnSet* column_set = grid_layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::CENTER, views::GridLayout::LEADING,
                        0 /*resize_percent*/, views::GridLayout::USE_PREF,
                        0 /*fixed_width*/, 0 /*min_width*/);
  auto add_view = [&](views::View* view) {
    grid_layout->StartRow(0 /*vertical_resize*/, 0 /*column_set_id*/);
    grid_layout->AddView(view);
  };
  auto add_padding = [&](int amount) {
    grid_layout->AddPaddingRow(0 /*vertical_resize*/, amount /*size*/);
  };

  // Add views in rendering order.
  add_padding(kDistanceFromTopOfBigUserViewToUserIconDp);
  add_view(wrapped_user_view);
  add_padding(kDistanceBetweenUserViewAndPasswordDp);
  add_view(wrapped_password_view);
  add_view(wrapped_online_sign_in_message_view);
  add_view(wrapped_disabled_auth_message_view);
  add_padding(kDistanceBetweenPasswordFieldAndPinKeyboard);
  add_view(wrapped_fingerprint_view);
  add_view(wrapped_pin_view);
  add_padding(kDistanceFromPinKeyboardToBigUserViewBottom);

  // Update authentication UI.
  SetAuthMethods(auth_methods_);
  user_view_->UpdateForUser(user, false /*animate*/);
}

LoginAuthUserView::~LoginAuthUserView() = default;

void LoginAuthUserView::SetAuthMethods(uint32_t auth_methods) {
  bool had_password = HasAuthMethod(AUTH_PASSWORD);

  auth_methods_ = static_cast<AuthMethods>(auth_methods);
  bool has_password = HasAuthMethod(AUTH_PASSWORD);
  bool has_pin = HasAuthMethod(AUTH_PIN);
  bool has_tap = HasAuthMethod(AUTH_TAP);
  bool force_online_sign_in = HasAuthMethod(AUTH_ONLINE_SIGN_IN);
  bool has_fingerprint = HasAuthMethod(AUTH_FINGERPRINT);
  bool disabled_auth = HasAuthMethod(AUTH_DISABLED);

  online_sign_in_message_->SetVisible(force_online_sign_in);
  disabled_auth_message_->SetVisible(disabled_auth);

  password_view_->SetEnabled(has_password);
  password_view_->SetFocusEnabledForChildViews(has_password);
  password_view_->SetVisible(!force_online_sign_in && !disabled_auth);
  password_view_->layer()->SetOpacity(has_password ? 1 : 0);

  if (!had_password && has_password)
    password_view_->RequestFocus();

  pin_view_->SetVisible(has_pin);
  fingerprint_view_->SetVisible(has_fingerprint);

  // Note: if both |has_tap| and |has_pin| are true, prefer tap placeholder.
  if (has_tap) {
    password_view_->SetPlaceholderText(
        l10n_util::GetStringUTF16(IDS_ASH_LOGIN_POD_PASSWORD_TAP_PLACEHOLDER));
  } else if (has_pin) {
    password_view_->SetPlaceholderText(
        l10n_util::GetStringUTF16(IDS_ASH_LOGIN_POD_PASSWORD_PIN_PLACEHOLDER));
  } else {
    password_view_->SetPlaceholderText(
        l10n_util::GetStringUTF16(IDS_ASH_LOGIN_POD_PASSWORD_PLACEHOLDER));
  }

  // Only the active auth user view has a password displayed. If that is the
  // case, then render the user view as if it was always focused, since clicking
  // on it will not do anything (such as swapping users).
  user_view_->SetForceOpaque(has_password || force_online_sign_in ||
                             disabled_auth);
  user_view_->SetTapEnabled(!has_password);

  PreferredSizeChanged();
}

void LoginAuthUserView::SetEasyUnlockIcon(
    mojom::EasyUnlockIconId id,
    const base::string16& accessibility_label) {
  password_view_->SetEasyUnlockIcon(id, accessibility_label);
}

void LoginAuthUserView::CaptureStateForAnimationPreLayout() {
  DCHECK(!cached_animation_state_);
  cached_animation_state_ = std::make_unique<AnimationState>(this);
}

void LoginAuthUserView::ApplyAnimationPostLayout() {
  DCHECK(cached_animation_state_);

  // Cancel any running animations.
  pin_view_->layer()->GetAnimator()->AbortAllAnimations();
  password_view_->layer()->GetAnimator()->AbortAllAnimations();
  layer()->GetAnimator()->AbortAllAnimations();

  bool has_password = (auth_methods() & AUTH_PASSWORD) != 0;
  bool has_pin = (auth_methods() & AUTH_PIN) != 0;

  ////////
  // Animate the user info (ie, icon, name) up or down the screen.

  int non_pin_y_end_in_screen = GetBoundsInScreen().y();

  // Transform the layer so the user view renders where it used to be. This
  // requires a y offset.
  // Note: Doing this animation via ui::ScopedLayerAnimationSettings works, but
  // it seems that the timing gets slightly out of sync with the PIN animation.
  auto move_to_center = std::make_unique<ui::InterpolatedTranslation>(
      gfx::PointF(0, cached_animation_state_->non_pin_y_start_in_screen -
                         non_pin_y_end_in_screen),
      gfx::PointF());
  auto transition =
      ui::LayerAnimationElement::CreateInterpolatedTransformElement(
          std::move(move_to_center),
          base::TimeDelta::FromMilliseconds(
              login_constants::kChangeUserAnimationDurationMs));
  transition->set_tween_type(gfx::Tween::Type::FAST_OUT_SLOW_IN);
  auto* sequence = new ui::LayerAnimationSequence(std::move(transition));
  layer()->GetAnimator()->StartAnimation(sequence);

  ////////
  // Fade the password view if it is being hidden or shown.

  if (cached_animation_state_->had_password != has_password) {
    float opacity_start = 0, opacity_end = 1;
    if (!has_password)
      std::swap(opacity_start, opacity_end);

    password_view_->layer()->SetOpacity(opacity_start);

    {
      ui::ScopedLayerAnimationSettings settings(
          password_view_->layer()->GetAnimator());
      settings.SetTransitionDuration(base::TimeDelta::FromMilliseconds(
          login_constants::kChangeUserAnimationDurationMs));
      settings.SetTweenType(gfx::Tween::Type::FAST_OUT_SLOW_IN);

      password_view_->layer()->SetOpacity(opacity_end);
    }
  }

  ////////
  // Grow/shrink the PIN keyboard if it is being hidden or shown.

  if (cached_animation_state_->had_pin != has_pin) {
    if (!has_pin) {
      gfx::Point pin_end_in_screen = pin_view_->GetBoundsInScreen().origin();
      gfx::Rect pin_bounds = pin_view_->bounds();
      pin_bounds.set_x(cached_animation_state_->pin_start_in_screen.x() -
                       pin_end_in_screen.x());
      pin_bounds.set_y(cached_animation_state_->pin_start_in_screen.y() -
                       pin_end_in_screen.y());

      // Since PIN is disabled, the previous Layout() hid the PIN keyboard.
      // We need to redisplay it where it used to be.
      pin_view_->SetVisible(true);
      pin_view_->SetBoundsRect(pin_bounds);
    }

    auto transition = std::make_unique<PinKeyboardAnimation>(
        has_pin /*grow*/, pin_view_->height(),
        base::TimeDelta::FromMilliseconds(
            login_constants::kChangeUserAnimationDurationMs),
        gfx::Tween::FAST_OUT_SLOW_IN);
    auto* sequence = new ui::LayerAnimationSequence(std::move(transition));
    pin_view_->layer()->GetAnimator()->ScheduleAnimation(sequence);

    // Hide the PIN keyboard after animation if needed.
    if (!has_pin) {
      auto* observer = BuildObserverToHideView(pin_view_);
      sequence->AddObserver(observer);
      observer->SetActive();
    }
  }

  cached_animation_state_.reset();
}

void LoginAuthUserView::UpdateForUser(const mojom::LoginUserInfoPtr& user) {
  user_view_->UpdateForUser(user, true /*animate*/);
  password_view_->UpdateForUser(user);
  password_view_->Clear();
  online_sign_in_message_->SetText(
      base::UTF8ToUTF16(user->basic_user_info->display_name));
}

void LoginAuthUserView::SetFingerprintState(
    mojom::FingerprintUnlockState state) {
  fingerprint_view_->SetVisible(state !=
                                mojom::FingerprintUnlockState::UNAVAILABLE);

  SkColor color = kFingerprintIconAndTextColor;
  if (state == mojom::FingerprintUnlockState::AUTH_SUCCESS) {
    color = SK_ColorBLUE;
  } else if (state == mojom::FingerprintUnlockState::AUTH_FAILED) {
    color = SK_ColorRED;
  }
  fingerprint_view_->SetIconColor(color);
}

void LoginAuthUserView::SetAuthReenabledTime(
    const base::Time& auth_reenabled_time) {
  disabled_auth_message_->SetAuthReenabledTime(auth_reenabled_time);
}

const mojom::LoginUserInfoPtr& LoginAuthUserView::current_user() const {
  return user_view_->current_user();
}

gfx::Size LoginAuthUserView::CalculatePreferredSize() const {
  gfx::Size size = views::View::CalculatePreferredSize();
  // Make sure we are at least as big as the user view. If we do not do this the
  // view will be below minimum size when no auth methods are displayed.
  size.SetToMax(user_view_->GetPreferredSize());
  return size;
}

void LoginAuthUserView::RequestFocus() {
  password_view_->RequestFocus();
}

void LoginAuthUserView::ButtonPressed(views::Button* sender,
                                      const ui::Event& event) {
  DCHECK_EQ(online_sign_in_message_, sender);
  OnOnlineSignInMessageTap();
}

void LoginAuthUserView::OnAuthSubmit(const base::string16& password) {
  bool authenticated_by_pin = (auth_methods_ & AUTH_PIN) != 0;

  // Pressing enter when the password field is empty and tap-to-unlock is
  // enabled should attempt unlock.
  if (HasAuthMethod(AUTH_TAP) && password.empty()) {
    Shell::Get()->login_screen_controller()->AttemptUnlock(
        current_user()->basic_user_info->account_id);
    return;
  }

  password_view_->SetReadOnly(true);
  Shell::Get()->login_screen_controller()->AuthenticateUser(
      current_user()->basic_user_info->account_id, base::UTF16ToUTF8(password),
      authenticated_by_pin,
      base::BindOnce(&LoginAuthUserView::OnAuthComplete,
                     weak_factory_.GetWeakPtr()));
}

void LoginAuthUserView::OnAuthComplete(base::Optional<bool> auth_success) {
  if (!auth_success.has_value())
    return;

  // Clear the password only if auth fails. Make sure to keep the password view
  // disabled even if auth succeededs, as if the user submits a password while
  // animating the next lock screen will not work as expected. See
  // https://crbug.com/808486.
  if (!auth_success.value()) {
    password_view_->Clear();
    password_view_->SetReadOnly(false);
  }

  on_auth_.Run(auth_success.value());
}

void LoginAuthUserView::OnUserViewTap() {
  if (HasAuthMethod(AUTH_TAP)) {
    Shell::Get()->login_screen_controller()->AttemptUnlock(
        current_user()->basic_user_info->account_id);
  } else if (HasAuthMethod(AUTH_ONLINE_SIGN_IN)) {
    // Tapping anywhere in the user view is the same with tapping the message.
    OnOnlineSignInMessageTap();
  } else {
    on_tap_.Run();
  }
}

void LoginAuthUserView::OnOnlineSignInMessageTap() {
  Shell::Get()->login_screen_controller()->ShowGaiaSignin(
      current_user()->basic_user_info->account_id);
}

bool LoginAuthUserView::HasAuthMethod(AuthMethods auth_method) const {
  return (auth_methods_ & auth_method) != 0;
}

void LoginAuthUserView::DecorateOnlineSignInMessage() {
  online_sign_in_message_->SetPaintToLayer();
  online_sign_in_message_->layer()->SetFillsBoundsOpaquely(false);
  online_sign_in_message_->SetImage(
      views::Button::STATE_NORMAL,
      CreateVectorIcon(kLockScreenAlertIcon, kOnlineSignInMessageColor));
  online_sign_in_message_->SetTextSubpixelRenderingEnabled(false);
  online_sign_in_message_->SetTextColor(views::Button::STATE_NORMAL,
                                        kOnlineSignInMessageColor);
  online_sign_in_message_->SetTextColor(views::Button::STATE_HOVERED,
                                        kOnlineSignInMessageColor);
  online_sign_in_message_->SetTextColor(views::Button::STATE_PRESSED,
                                        kOnlineSignInMessageColor);
  online_sign_in_message_->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(9, 0)));
}

}  // namespace ash
