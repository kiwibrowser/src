// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"

#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/avatar_menu.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/theme_resources.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/background.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "chrome/browser/ui/ash/session_util.h"
#endif  // defined(OS_CHROMEOS)

#if defined(OS_WIN)
#include "chrome/browser/ui/views/frame/taskbar_decorator_win.h"
#endif

using MD = ui::MaterialDesignController;

BrowserNonClientFrameView::BrowserNonClientFrameView(BrowserFrame* frame,
                                                     BrowserView* browser_view)
    : frame_(frame),
      browser_view_(browser_view),
      profile_switcher_(this),
      profile_indicator_icon_(nullptr),
      tab_strip_observer_(this) {
  // The profile manager may by null in tests.
  if (g_browser_process->profile_manager()) {
    g_browser_process->profile_manager()->
        GetProfileAttributesStorage().AddObserver(this);
  }
}

BrowserNonClientFrameView::~BrowserNonClientFrameView() {
  // The profile manager may by null in tests.
  if (g_browser_process->profile_manager()) {
    g_browser_process->profile_manager()->
        GetProfileAttributesStorage().RemoveObserver(this);
  }
}

// static
int BrowserNonClientFrameView::GetAvatarIconPadding() {
  return MD::IsNewerMaterialUi() ? 8 : 4;
}

void BrowserNonClientFrameView::OnBrowserViewInitViewsComplete() {
  if (browser_view()->tabstrip()) {
    DCHECK(!tab_strip_observer_.IsObserving(browser_view()->tabstrip()));
    tab_strip_observer_.Add(browser_view()->tabstrip());
  }
  UpdateMinimumSize();
}

void BrowserNonClientFrameView::OnMaximizedStateChanged() {}

void BrowserNonClientFrameView::OnFullscreenStateChanged() {}

bool BrowserNonClientFrameView::CaptionButtonsOnLeadingEdge() const {
  return false;
}

gfx::ImageSkia BrowserNonClientFrameView::GetIncognitoAvatarIcon() const {
  const SkColor icon_color = color_utils::PickContrastingColor(
      SK_ColorWHITE, gfx::kChromeIconGrey, GetFrameColor());
  return gfx::CreateVectorIcon(kIncognitoIcon, icon_color);
}

SkColor BrowserNonClientFrameView::GetToolbarTopSeparatorColor() const {
  const auto color_id =
      ShouldPaintAsActive()
          ? ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR
          : ThemeProperties::COLOR_TOOLBAR_TOP_SEPARATOR_INACTIVE;
  return ShouldPaintAsThemed() ? GetThemeProvider()->GetColor(color_id)
                               : ThemeProperties::GetDefaultColor(
                                     color_id, browser_view_->IsIncognito());
}

views::Button* BrowserNonClientFrameView::GetProfileSwitcherButton() const {
  return profile_switcher_.avatar_button();
}

void BrowserNonClientFrameView::UpdateClientArea() {}

void BrowserNonClientFrameView::UpdateMinimumSize() {}

int BrowserNonClientFrameView::GetTabStripLeftInset() const {
  if (profile_indicator_icon())
    return 2 * GetAvatarIconPadding() + GetIncognitoAvatarIcon().width();
  return MD::IsRefreshUi() ? 8 : 4;
}

void BrowserNonClientFrameView::ChildPreferredSizeChanged(views::View* child) {
  if (child == GetProfileSwitcherButton()) {
    // Perform a re-layout if the avatar button has changed, since that can
    // affect the size of the tabs.
    frame()->GetRootView()->Layout();
  }
}

void BrowserNonClientFrameView::VisibilityChanged(views::View* starting_from,
                                                  bool is_visible) {
  // UpdateTaskbarDecoration() calls DrawTaskbarDecoration(), but that does
  // nothing if the window is not visible.  So even if we've already gotten the
  // up-to-date decoration, we need to run the update procedure again here when
  // the window becomes visible.
  if (is_visible)
    OnProfileAvatarChanged(base::FilePath());
}

void BrowserNonClientFrameView::OnTabAdded(int index) {
  if (MD::IsRefreshUi() && browser_view()->tabstrip()->tab_count() == 2) {
    // We are exiting single-tab mode and need to repaint the frame.
    SchedulePaint();
  }
}

void BrowserNonClientFrameView::OnTabRemoved(int index) {
  if (MD::IsRefreshUi() && browser_view()->tabstrip()->tab_count() == 1) {
    // We are entering single-tab mode and need to repaint the frame.
    SchedulePaint();
  }
}

bool BrowserNonClientFrameView::ShouldPaintAsThemed() const {
  return browser_view_->IsBrowserTypeNormal();
}

bool BrowserNonClientFrameView::ShouldPaintAsSingleTabMode() const {
  // Single-tab mode is only available in Refresh. The special color we use for
  // won't be visible if there's a frame image, but since it's used to determine
  // constrast of other UI elements, the theme color should be used instead.
  return MD::IsRefreshUi() && GetFrameImage().isNull() &&
         browser_view()->IsTabStripVisible() &&
         browser_view()->tabstrip()->tab_count() == 1;
}

SkColor BrowserNonClientFrameView::GetFrameColor(bool active) const {
  ThemeProperties::OverwritableByUserThemeProperty color_id;
  if (ShouldPaintAsSingleTabMode()) {
    color_id = ThemeProperties::COLOR_TOOLBAR;
  } else {
    color_id = active ? ThemeProperties::COLOR_FRAME
                      : ThemeProperties::COLOR_FRAME_INACTIVE;
  }
  return ShouldPaintAsThemed()
             ? GetThemeProviderForProfile()->GetColor(color_id)
             : ThemeProperties::GetDefaultColor(color_id,
                                                browser_view_->IsIncognito());
}

gfx::ImageSkia BrowserNonClientFrameView::GetFrameImage(bool active) const {
  const ui::ThemeProvider* tp = frame_->GetThemeProvider();
  int frame_image_id = active ? IDR_THEME_FRAME : IDR_THEME_FRAME_INACTIVE;
  return ShouldPaintAsThemed() && (tp->HasCustomImage(frame_image_id) ||
                                   tp->HasCustomImage(IDR_THEME_FRAME))
             ? *tp->GetImageSkiaNamed(frame_image_id)
             : gfx::ImageSkia();
}

gfx::ImageSkia BrowserNonClientFrameView::GetFrameOverlayImage(
    bool active) const {
  if (browser_view_->IsIncognito() || !browser_view_->IsBrowserTypeNormal())
    return gfx::ImageSkia();

  const ui::ThemeProvider* tp = frame_->GetThemeProvider();
  int frame_overlay_image_id =
      active ? IDR_THEME_FRAME_OVERLAY : IDR_THEME_FRAME_OVERLAY_INACTIVE;
  return tp->HasCustomImage(frame_overlay_image_id)
             ? *tp->GetImageSkiaNamed(frame_overlay_image_id)
             : gfx::ImageSkia();
}

SkColor BrowserNonClientFrameView::GetFrameColor() const {
  return GetFrameColor(ShouldPaintAsActive());
}

gfx::ImageSkia BrowserNonClientFrameView::GetFrameImage() const {
  return GetFrameImage(ShouldPaintAsActive());
}

gfx::ImageSkia BrowserNonClientFrameView::GetFrameOverlayImage() const {
  return GetFrameOverlayImage(ShouldPaintAsActive());
}

void BrowserNonClientFrameView::UpdateProfileIcons() {
  const AvatarButtonStyle avatar_button_style = GetAvatarButtonStyle();
  if (avatar_button_style != AvatarButtonStyle::NONE &&
      browser_view()->IsRegularOrGuestSession()) {
    // Platform supports a profile switcher that will be shown. Skip the rest.
    profile_switcher_.Update(avatar_button_style);
    return;
  }

  if (!ShouldShowProfileIndicatorIcon()) {
    if (profile_indicator_icon_) {
      delete profile_indicator_icon_;
      profile_indicator_icon_ = nullptr;
      frame_->GetRootView()->Layout();
    }
    return;
  }

  if (!profile_indicator_icon_) {
    profile_indicator_icon_ = new ProfileIndicatorIcon();
    profile_indicator_icon_->set_id(VIEW_ID_PROFILE_INDICATOR_ICON);
    AddChildView(profile_indicator_icon_);
    // Invalidate here because adding a child does not invalidate the layout.
    InvalidateLayout();
    frame_->GetRootView()->Layout();
  }

  gfx::Image icon;
  Profile* profile = browser_view()->browser()->profile();
  const bool is_incognito =
      profile->GetProfileType() == Profile::INCOGNITO_PROFILE;
  if (is_incognito) {
    icon = gfx::Image(GetIncognitoAvatarIcon());
    profile_indicator_icon_->set_stroke_color(SK_ColorTRANSPARENT);
  } else {
#if defined(OS_CHROMEOS)
    icon = gfx::Image(GetAvatarImageForContext(profile));
    // Draw a stroke around the profile icon only for the avatar.
    profile_indicator_icon_->set_stroke_color(GetToolbarTopSeparatorColor());
#else
    NOTREACHED();
#endif
  }

  profile_indicator_icon_->SetIcon(icon);
}

void BrowserNonClientFrameView::LayoutIncognitoButton() {
  DCHECK(profile_indicator_icon());
#if !defined(OS_CHROMEOS)
  // ChromeOS shows avatar on V1 app.
  DCHECK(browser_view()->IsTabStripVisible());
#endif
  gfx::ImageSkia incognito_icon = GetIncognitoAvatarIcon();
  int avatar_bottom = GetTopInset(false) + browser_view()->GetTabStripHeight() -
                      GetAvatarIconPadding();
  int avatar_y = avatar_bottom - incognito_icon.height();
  int avatar_height = incognito_icon.height();
  gfx::Rect avatar_bounds(GetAvatarIconPadding(), avatar_y,
                          incognito_icon.width(), avatar_height);

  profile_indicator_icon()->SetBoundsRect(avatar_bounds);
  profile_indicator_icon()->SetVisible(true);
}

void BrowserNonClientFrameView::PaintToolbarBackground(
    gfx::Canvas* canvas) const {
  gfx::Rect toolbar_bounds(browser_view()->GetToolbarBounds());
  if (toolbar_bounds.IsEmpty())
    return;
  gfx::Point toolbar_origin(toolbar_bounds.origin());
  ConvertPointToTarget(browser_view(), this, &toolbar_origin);
  toolbar_bounds.set_origin(toolbar_origin);

  const ui::ThemeProvider* tp = GetThemeProvider();
  const int x = toolbar_bounds.x();
  const int y = toolbar_bounds.y();
  const int w = toolbar_bounds.width();

  // Background.
  if (tp->HasCustomImage(IDR_THEME_TOOLBAR)) {
    canvas->TileImageInt(*tp->GetImageSkiaNamed(IDR_THEME_TOOLBAR),
                         x + GetThemeBackgroundXInset(),
                         y - GetTopInset(false) - GetLayoutInsets(TAB).top(), x,
                         y, w, toolbar_bounds.height());
  } else {
    canvas->FillRect(toolbar_bounds,
                     tp->GetColor(ThemeProperties::COLOR_TOOLBAR));
  }

  gfx::ScopedCanvas scoped_canvas(canvas);
  if (TabStrip::ShouldDrawStrokes()) {
    // Top stroke.
    gfx::Rect tabstrip_bounds =
        GetMirroredRect(GetBoundsForTabStrip(browser_view()->tabstrip()));
    canvas->ClipRect(tabstrip_bounds, SkClipOp::kDifference);
    gfx::Rect separator_rect(x, y, w, 0);
    separator_rect.set_y(tabstrip_bounds.bottom());
    BrowserView::Paint1pxHorizontalLine(canvas, GetToolbarTopSeparatorColor(),
                                        separator_rect, true);
  }
  // Toolbar/content separator.
  BrowserView::Paint1pxHorizontalLine(
      canvas, tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BOTTOM_SEPARATOR),
      toolbar_bounds, true);
}

void BrowserNonClientFrameView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this)
    UpdateProfileIcons();
}

void BrowserNonClientFrameView::ActivationChanged(bool active) {
  // On Windows, while deactivating the widget, this is called before the
  // active HWND has actually been changed.  Since we want the avatar state to
  // reflect that the window is inactive, we force NonClientFrameView to see the
  // "correct" state as an override.
  set_active_state_override(&active);
  UpdateProfileIcons();
  set_active_state_override(nullptr);

  // Changing the activation state may change the toolbar top separator color
  // that's used as the stroke around tabs/the new tab button.
  browser_view_->tabstrip()->SchedulePaint();

  // Changing the activation state may change the visible frame color.
  SchedulePaint();
}

bool BrowserNonClientFrameView::DoesIntersectRect(const views::View* target,
                                                  const gfx::Rect& rect) const {
  DCHECK_EQ(target, this);
  if (!views::ViewTargeterDelegate::DoesIntersectRect(this, rect)) {
    // |rect| is outside the frame's bounds.
    return false;
  }

  bool should_leave_to_top_container = false;
#if defined(OS_CHROMEOS)
  if (browser_view()->immersive_mode_controller()->IsRevealed()) {
    // In immersive mode, the caption buttons container is reparented to the
    // TopContainerView and hence |rect| should not be claimed here.
    // See BrowserNonClientFrameViewAsh::OnImmersiveRevealStarted().
    should_leave_to_top_container = true;
  }
#endif  // defined(OS_CHROMEOS)

  if (!browser_view()->IsTabStripVisible()) {
    if (should_leave_to_top_container)
      return false;

    // Claim |rect| if it is above the top of the topmost client area view.
    return rect.y() < GetTopInset(false);
  }

  // If the rect is outside the bounds of the client area, claim it.
  gfx::RectF rect_in_client_view_coords_f(rect);
  View::ConvertRectToTarget(this, frame()->client_view(),
                            &rect_in_client_view_coords_f);
  gfx::Rect rect_in_client_view_coords =
      gfx::ToEnclosingRect(rect_in_client_view_coords_f);
  if (!frame()->client_view()->HitTestRect(rect_in_client_view_coords))
    return true;

  // Otherwise, claim |rect| only if it is above the bottom of the tabstrip in
  // a non-tab portion.
  TabStrip* tabstrip = browser_view()->tabstrip();
  if (!tabstrip || !browser_view()->IsTabStripVisible())
    return false;

  gfx::RectF rect_in_tabstrip_coords_f(rect);
  View::ConvertRectToTarget(this, tabstrip, &rect_in_tabstrip_coords_f);
  gfx::Rect rect_in_tabstrip_coords =
      gfx::ToEnclosingRect(rect_in_tabstrip_coords_f);
  if (rect_in_tabstrip_coords.y() >= tabstrip->GetLocalBounds().bottom()) {
    // |rect| is below the tabstrip.
    return false;
  }

  if (tabstrip->HitTestRect(rect_in_tabstrip_coords)) {
    // Claim |rect| if it is in a non-tab portion of the tabstrip.
    return tabstrip->IsRectInWindowCaption(rect_in_tabstrip_coords);
  }

  if (should_leave_to_top_container)
    return false;

  // We claim |rect| because it is above the bottom of the tabstrip, but
  // not in the tabstrip itself. In particular, the avatar label/button is left
  // of the tabstrip and the window controls are right of the tabstrip.
  return true;
}

void BrowserNonClientFrameView::OnProfileAdded(
    const base::FilePath& profile_path) {
  OnProfileAvatarChanged(profile_path);
}

void BrowserNonClientFrameView::OnProfileWasRemoved(
    const base::FilePath& profile_path,
    const base::string16& profile_name) {
  OnProfileAvatarChanged(profile_path);
}

void BrowserNonClientFrameView::OnProfileAvatarChanged(
    const base::FilePath& profile_path) {
  UpdateTaskbarDecoration();
  UpdateProfileIcons();
}

void BrowserNonClientFrameView::OnProfileHighResAvatarLoaded(
    const base::FilePath& profile_path) {
  UpdateTaskbarDecoration();
}

const ui::ThemeProvider*
BrowserNonClientFrameView::GetThemeProviderForProfile() const {
  // Because the frame's accessor reads the ThemeProvider from the profile and
  // not the widget, it can be called even before we're in a view hierarchy.
  return frame_->GetThemeProvider();
}

void BrowserNonClientFrameView::UpdateTaskbarDecoration() {
#if defined(OS_WIN)
  if (browser_view()->browser()->profile()->IsGuestSession() ||
      // Browser process and profile manager may be null in tests.
      (g_browser_process && g_browser_process->profile_manager() &&
       g_browser_process->profile_manager()
               ->GetProfileAttributesStorage()
               .GetNumberOfProfiles() <= 1)) {
    chrome::DrawTaskbarDecoration(frame_->GetNativeWindow(), nullptr);
    return;
  }

  // For popups and panels which don't have the avatar button, we still
  // need to draw the taskbar decoration. Even though we have an icon on the
  // window's relaunch details, we draw over it because the user may have
  // pinned the badge-less Chrome shortcut which will cause Windows to ignore
  // the relaunch details.
  // TODO(calamity): ideally this should not be necessary but due to issues
  // with the default shortcut being pinned, we add the runtime badge for
  // safety. See crbug.com/313800.
  gfx::Image decoration;
  AvatarMenu::ImageLoadStatus status = AvatarMenu::GetImageForMenuButton(
      browser_view()->browser()->profile()->GetPath(), &decoration);

  UMA_HISTOGRAM_ENUMERATION(
      "Profile.AvatarLoadStatus", status,
      static_cast<int>(AvatarMenu::ImageLoadStatus::MAX) + 1);

  // If the user is using a Gaia picture and the picture is still being loaded,
  // wait until the load finishes. This taskbar decoration will be triggered
  // again upon the finish of the picture load.
  if (status == AvatarMenu::ImageLoadStatus::LOADING ||
      status == AvatarMenu::ImageLoadStatus::PROFILE_DELETED) {
    return;
  }

  chrome::DrawTaskbarDecoration(frame_->GetNativeWindow(), &decoration);
#endif
}

bool BrowserNonClientFrameView::ShouldShowProfileIndicatorIcon() const {
  // In Material Refresh, we use a toolbar button for all
  // profile/incognito-related purposes.
  if (MD::IsRefreshUi())
    return false;

  Browser* browser = browser_view()->browser();
  Profile* profile = browser->profile();
  const bool is_incognito =
      profile->GetProfileType() == Profile::INCOGNITO_PROFILE;

  // In the touch-optimized UI, we don't show the incognito icon in the browser
  // frame. It's instead shown in the new tab button. However, we still show an
  // avatar icon for the teleported browser windows between multi-user sessions
  // (Chrome OS only). Note that you can't teleport an incognito window.
  if (is_incognito && MD::IsTouchOptimizedUiEnabled())
    return false;

#if defined(OS_CHROMEOS)
  if (!browser->is_type_tabbed() && !browser->is_app())
    return false;

  if (!is_incognito && !MultiUserWindowManager::ShouldShowAvatar(
                           browser_view()->GetNativeWindow())) {
    return false;
  }
#endif  // defined(OS_CHROMEOS)
  return true;
}
