// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/glass_browser_frame_view.h"

#include <dwmapi.h>
#include <utility>

#include "base/trace_event/common/trace_event_common.h"
#include "base/win/windows_version.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/profiles/profile_indicator_icon.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/win/titlebar_config.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/image_operations.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle_win.h"
#include "ui/base/theme_provider.h"
#include "ui/display/win/dpi.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/resources/grit/views_resources.h"
#include "ui/views/win/hwnd_util.h"
#include "ui/views/window/client_view.h"

HICON GlassBrowserFrameView::throbber_icons_[
    GlassBrowserFrameView::kThrobberIconCount];

namespace {

// How far the profile switcher button is from the left of the minimize button.
constexpr int kProfileSwitcherButtonOffset = 1;

// Converts the |image| to a Windows icon and returns the corresponding HICON
// handle. |image| is resized to desired |width| and |height| if needed.
base::win::ScopedHICON CreateHICONFromSkBitmapSizedTo(
    const gfx::ImageSkia& image,
    int width,
    int height) {
  return IconUtil::CreateHICONFromSkBitmap(
      width == image.width() && height == image.height()
          ? *image.bitmap()
          : skia::ImageOperations::Resize(*image.bitmap(),
                                          skia::ImageOperations::RESIZE_BEST,
                                          width, height));
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, public:

GlassBrowserFrameView::GlassBrowserFrameView(BrowserFrame* frame,
                                             BrowserView* browser_view)
    : BrowserNonClientFrameView(frame, browser_view),
      window_icon_(nullptr),
      window_title_(nullptr),
      minimize_button_(nullptr),
      maximize_button_(nullptr),
      restore_button_(nullptr),
      close_button_(nullptr),
      throbber_running_(false),
      throbber_frame_(0) {
  // We initialize all fields despite some of them being unused in some modes,
  // since it's possible for modes to flip dynamically (e.g. if the user enables
  // a high-contrast theme). Throbber icons are only used when ShowSystemIcon()
  // is true. Everything else here is only used when
  // ShouldCustomDrawSystemTitlebar() is true.

  if (browser_view->ShouldShowWindowIcon()) {
    InitThrobberIcons();

    window_icon_ = new TabIconView(this, nullptr);
    window_icon_->set_is_light(true);
    window_icon_->set_id(VIEW_ID_WINDOW_ICON);
    // Stop the icon from intercepting clicks intended for the HTSYSMENU region
    // of the window. Even though it does nothing on click, it will still
    // prevent us from giving the event back to Windows to handle properly.
    window_icon_->set_can_process_events_within_subtree(false);
    AddChildView(window_icon_);
  }

  if (browser_view->ShouldShowWindowTitle()) {
    window_title_ = new views::Label(browser_view->GetWindowTitle());
    window_title_->SetSubpixelRenderingEnabled(false);
    window_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    window_title_->set_id(VIEW_ID_WINDOW_TITLE);
    AddChildView(window_title_);
  }

  minimize_button_ =
      CreateCaptionButton(VIEW_ID_MINIMIZE_BUTTON, IDS_APP_ACCNAME_MINIMIZE);
  maximize_button_ =
      CreateCaptionButton(VIEW_ID_MAXIMIZE_BUTTON, IDS_APP_ACCNAME_MAXIMIZE);
  restore_button_ =
      CreateCaptionButton(VIEW_ID_RESTORE_BUTTON, IDS_APP_ACCNAME_RESTORE);
  close_button_ =
      CreateCaptionButton(VIEW_ID_CLOSE_BUTTON, IDS_APP_ACCNAME_CLOSE);
}

GlassBrowserFrameView::~GlassBrowserFrameView() {
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, BrowserNonClientFrameView implementation:

bool GlassBrowserFrameView::CaptionButtonsOnLeadingEdge() const {
  // Because we don't set WS_EX_LAYOUTRTL (which would conflict with Chrome's
  // own RTL layout logic), Windows always draws the caption buttons on the
  // right, even when we want to be RTL. See crbug.com/560619.
  return !ShouldCustomDrawSystemTitlebar() && base::i18n::IsRTL();
}

gfx::Rect GlassBrowserFrameView::GetBoundsForTabStrip(
    views::View* tabstrip) const {
  const int x = GetTabStripLeftInset();
  int end_x = width() - ClientBorderThickness(false);
  if (!CaptionButtonsOnLeadingEdge())
    end_x = std::min(MinimizeButtonX() - TabStripCaptionSpacing(), end_x);
  return gfx::Rect(x, TopAreaHeight(false), std::max(0, end_x - x),
                   tabstrip->GetPreferredSize().height());
}

int GlassBrowserFrameView::GetTopInset(bool restored) const {
  return GetClientAreaInsets(restored).top();
}

int GlassBrowserFrameView::GetThemeBackgroundXInset() const {
  return 0;
}

void GlassBrowserFrameView::UpdateThrobber(bool running) {
  if (ShowCustomIcon())
    window_icon_->Update();

  if (!ShowSystemIcon())
    return;

  if (throbber_running_) {
    if (running) {
      DisplayNextThrobberFrame();
    } else {
      StopThrobber();
    }
  } else if (running) {
    StartThrobber();
  }
}

gfx::Size GlassBrowserFrameView::GetMinimumSize() const {
  gfx::Size min_size(browser_view()->GetMinimumSize());

  // Account for the client area insets.
  gfx::Insets insets = GetClientAreaInsets(false);
  min_size.Enlarge(insets.width(), insets.height());
  // The content edge images have a shadow built into them.  Client area insets
  // do not include this shadow thickness.
  constexpr int kContentEdgeShadowThickness = 2;
  min_size.Enlarge(2 * kContentEdgeShadowThickness, 0);

  // Ensure that the minimum width is enough to hold a tab strip with minimum
  // width at its usual insets.
  if (browser_view()->IsTabStripVisible()) {
    TabStrip* tabstrip = browser_view()->tabstrip();
    int min_tabstrip_width = tabstrip->GetMinimumSize().width();
    int min_tabstrip_area_width =
        width() - GetBoundsForTabStrip(tabstrip).width() + min_tabstrip_width;
    min_size.set_width(std::max(min_tabstrip_area_width, min_size.width()));
  }

  return min_size;
}

int GlassBrowserFrameView::GetTabStripLeftInset() const {
  return incognito_bounds_.right() + GetAvatarIconPadding();
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, views::NonClientFrameView implementation:

gfx::Rect GlassBrowserFrameView::GetBoundsForClientView() const {
  return client_view_bounds_;
}

gfx::Rect GlassBrowserFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  HWND hwnd = views::HWNDForWidget(frame());
  if (!browser_view()->IsTabStripVisible() && hwnd) {
    // If we don't have a tabstrip, we're either a popup or an app window, in
    // which case we have a standard size non-client area and can just use
    // AdjustWindowRectEx to obtain it. We check for a non-null window handle in
    // case this gets called before the window is actually created.
    RECT rect = client_bounds.ToRECT();
    AdjustWindowRectEx(&rect, GetWindowLong(hwnd, GWL_STYLE), FALSE,
                       GetWindowLong(hwnd, GWL_EXSTYLE));
    return gfx::Rect(rect);
  }

  gfx::Insets insets = GetClientAreaInsets(false);
  return gfx::Rect(std::max(0, client_bounds.x() - insets.left()),
                   std::max(0, client_bounds.y() - insets.top()),
                   client_bounds.width() + insets.width(),
                   client_bounds.height() + insets.height());
}

namespace {

bool HitTestCaptionButton(Windows10CaptionButton* button,
                          const gfx::Point& point) {
  return button && button->visible() &&
         button->GetMirroredBounds().Contains(point);
}

}  // namespace

int GlassBrowserFrameView::NonClientHitTest(const gfx::Point& point) {
  // For app windows and popups without a custom titlebar we haven't customized
  // the frame at all so Windows can figure it out.
  if (!ShouldCustomDrawSystemTitlebar() &&
      !browser_view()->IsBrowserTypeNormal())
    return HTNOWHERE;

  // If the point isn't within our bounds, then it's in the native portion of
  // the frame so again Windows can figure it out.
  if (!bounds().Contains(point))
    return HTNOWHERE;

  // See if the point is within the incognito icon or the profile switcher menu.
  views::View* profile_switcher_view = GetProfileSwitcherButton();
  if ((profile_indicator_icon() &&
       profile_indicator_icon()->GetMirroredBounds().Contains(point)) ||
      (profile_switcher_view &&
       profile_switcher_view->GetMirroredBounds().Contains(point))) {
    return HTCLIENT;
  }

  int frame_component = frame()->client_view()->NonClientHitTest(point);

  // See if we're in the sysmenu region.  We still have to check the tabstrip
  // first so that clicks in a tab don't get treated as sysmenu clicks.
  int client_border_thickness = ClientBorderThickness(false);
  gfx::Rect sys_menu_region(
      client_border_thickness,
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSIZEFRAME),
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSMICON),
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSMICON));
  if (sys_menu_region.Contains(point))
    return (frame_component == HTCLIENT) ? HTCLIENT : HTSYSMENU;

  if (frame_component != HTNOWHERE)
    return frame_component;

  // Then see if the point is within any of the window controls.
  if (HitTestCaptionButton(minimize_button_, point))
    return HTMINBUTTON;
  if (HitTestCaptionButton(maximize_button_, point))
    return HTMAXBUTTON;
  if (HitTestCaptionButton(restore_button_, point))
    return HTMAXBUTTON;
  if (HitTestCaptionButton(close_button_, point))
    return HTCLOSE;

  // On Windows 8+, the caption buttons are almost butted up to the top right
  // corner of the window. This code ensures the mouse isn't set to a size
  // cursor while hovering over the caption buttons, thus giving the incorrect
  // impression that the user can resize the window.
  if (base::win::GetVersion() >= base::win::VERSION_WIN8) {
    RECT button_bounds = {0};
    if (SUCCEEDED(DwmGetWindowAttribute(views::HWNDForWidget(frame()),
                                        DWMWA_CAPTION_BUTTON_BOUNDS,
                                        &button_bounds,
                                        sizeof(button_bounds)))) {
      gfx::Rect buttons = gfx::ConvertRectToDIP(display::win::GetDPIScale(),
                                                gfx::Rect(button_bounds));

      // There is a small one-pixel strip right above the caption buttons in
      // which the resize border "peeks" through.
      constexpr int kCaptionButtonTopInset = 1;
      // The sizing region at the window edge above the caption buttons is
      // 1 px regardless of scale factor. If we inset by 1 before converting
      // to DIPs, the precision loss might eliminate this region entirely. The
      // best we can do is to inset after conversion. This guarantees we'll
      // show the resize cursor when resizing is possible. The cost of which
      // is also maybe showing it over the portion of the DIP that isn't the
      // outermost pixel.
      buttons.Inset(0, kCaptionButtonTopInset, 0, 0);
      if (buttons.Contains(point))
        return HTNOWHERE;
    }
  }

  int top_border_thickness = FrameTopBorderThickness(false);
  // At the window corners the resize area is not actually bigger, but the 16
  // pixels at the end of the top and bottom edges trigger diagonal resizing.
  constexpr int kResizeCornerWidth = 16;
  // We want the resize corner behavior to apply to the kResizeCornerWidth
  // pixels at each end of the top and bottom edges.  Because |point|'s x
  // coordinate is based on the DWM-inset portion of the window (so, it's 0 at
  // the first pixel inside the left DWM margin), we need to subtract the DWM
  // margin thickness, which we calculate as the total frame border thickness
  // minus the nonclient border thickness.
  const int dwm_margin = FrameBorderThickness() - client_border_thickness;
  int window_component = GetHTComponentForFrame(
      point, top_border_thickness, client_border_thickness,
      top_border_thickness, kResizeCornerWidth - dwm_margin,
      frame()->widget_delegate()->CanResize());
  // Fall back to the caption if no other component matches.
  return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

void GlassBrowserFrameView::UpdateWindowIcon() {
  if (ShowCustomIcon() && !frame()->IsFullscreen())
    window_icon_->SchedulePaint();
}

void GlassBrowserFrameView::UpdateWindowTitle() {
  if (ShowCustomTitle() && !frame()->IsFullscreen())
    window_title_->SchedulePaint();
}

void GlassBrowserFrameView::ResetWindowControls() {
  minimize_button_->SetState(views::Button::STATE_NORMAL);
  maximize_button_->SetState(views::Button::STATE_NORMAL);
  restore_button_->SetState(views::Button::STATE_NORMAL);
  close_button_->SetState(views::Button::STATE_NORMAL);
}

void GlassBrowserFrameView::ButtonPressed(views::Button* sender,
                                          const ui::Event& event) {
  if (sender == minimize_button_)
    frame()->Minimize();
  else if (sender == maximize_button_)
    frame()->Maximize();
  else if (sender == restore_button_)
    frame()->Restore();
  else if (sender == close_button_)
    frame()->Close();
}

bool GlassBrowserFrameView::ShouldTabIconViewAnimate() const {
  DCHECK(ShowCustomIcon());
  const content::WebContents* current_tab =
      browser_view()->GetActiveWebContents();
  return current_tab && current_tab->IsLoading();
}

gfx::ImageSkia GlassBrowserFrameView::GetFaviconForTabIconView() {
  DCHECK(ShowCustomIcon());
  return frame()->widget_delegate()->GetWindowIcon();
}

void GlassBrowserFrameView::OnTabRemoved(int index) {
  BrowserNonClientFrameView::OnTabRemoved(index);
  // The profile switcher button may need to change height here, too.
  // TabStripMaxXChanged is not enough when a tab other than the last tab is
  // closed.
  LayoutProfileSwitcher();
}

void GlassBrowserFrameView::OnTabsMaxXChanged() {
  BrowserNonClientFrameView::OnTabsMaxXChanged();
  // The profile switcher button's height depends on the position of the new
  // tab button, which may have changed if the tabs max X changed.
  LayoutProfileSwitcher();
}

bool GlassBrowserFrameView::IsMaximized() const {
  return frame()->IsMaximized();
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, views::View overrides:

void GlassBrowserFrameView::OnPaint(gfx::Canvas* canvas) {
  TRACE_EVENT0("views.frame", "GlassBrowserFrameView::OnPaint");
  if (ShouldCustomDrawSystemTitlebar())
    PaintTitlebar(canvas);
  if (!browser_view()->IsTabStripVisible())
    return;
  if (IsToolbarVisible())
    PaintToolbarBackground(canvas);
  if (ClientBorderThickness(false) > 0)
    PaintClientEdge(canvas);
}

void GlassBrowserFrameView::Layout() {
  TRACE_EVENT0("views.frame", "GlassBrowserFrameView::Layout");
  // The profile switcher and incognito icon depends on the caption button
  // layout, so always call it first.
  if (ShouldCustomDrawSystemTitlebar())
    LayoutCaptionButtons();

  LayoutProfileSwitcher();

  // The incognito area must be laid out even if we're not in incognito as
  // tab-strip insets depend on it. When not in incognito the bounds will be
  // zero-width but positioned correctly for the titlebar to start after it.
  LayoutIncognitoIcon();

  if (ShouldCustomDrawSystemTitlebar())
    LayoutTitleBar();

  LayoutClientView();
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, protected:

// BrowserNonClientFrameView:
AvatarButtonStyle GlassBrowserFrameView::GetAvatarButtonStyle() const {
  return AvatarButtonStyle::NATIVE;
}

///////////////////////////////////////////////////////////////////////////////
// GlassBrowserFrameView, private:

// views::NonClientFrameView:
bool GlassBrowserFrameView::DoesIntersectRect(const views::View* target,
                                              const gfx::Rect& rect) const {
  if (ShouldCustomDrawSystemTitlebar())
    return BrowserNonClientFrameView::DoesIntersectRect(target, rect);

  // TODO(bsep): This override has "dead zones" where you can't click on the
  // custom titlebar buttons. It's not clear why it's necessary at all.
  // Investigate tearing this out.
  CHECK_EQ(target, this);
  bool hit_incognito_icon =
      profile_indicator_icon() &&
      profile_indicator_icon()->GetMirroredBounds().Intersects(rect);
  views::View* profile_switcher_view = GetProfileSwitcherButton();
  bool hit_profile_switcher_button =
      profile_switcher_view &&
      profile_switcher_view->GetMirroredBounds().Intersects(rect);
  return hit_incognito_icon || hit_profile_switcher_button ||
         !frame()->client_view()->bounds().Intersects(rect);
}

int GlassBrowserFrameView::ClientBorderThickness(bool restored) const {
  // The frame ends abruptly at the 1 pixel window border drawn by Windows 10.
  if (!browser_view()->HasClientEdge())
    return 0;

  if ((IsMaximized() || frame()->IsFullscreen()) && !restored)
    return 0;

  // Thickness of the frame edge between the non-client area and web content.
  constexpr int kClientBorderThickness = 3;
  return kClientBorderThickness;
}

int GlassBrowserFrameView::FrameBorderThickness() const {
  return (IsMaximized() || frame()->IsFullscreen())
             ? 0
             : display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME);
}

int GlassBrowserFrameView::FrameTopBorderThickness(bool restored) const {
  // Mouse and touch locations are floored but GetSystemMetricsInDIP is rounded,
  // so we need to floor instead or else the difference will cause the hittest
  // to fail when it ought to succeed.
  // TODO(robliao): Resolve this GetSystemMetrics call.
  return std::floor(FrameTopBorderThicknessPx(restored) /
                    display::win::GetDPIScale());
}

int GlassBrowserFrameView::FrameTopBorderThicknessPx(bool restored) const {
  // Distinct from FrameBorderThickness() because Windows gives maximized
  // windows an offscreen CYSIZEFRAME-thick region around the edges. The
  // left/right/bottom edges don't worry about this because we cancel them out
  // in BrowserDesktopWindowTreeHostWin::GetClientAreaInsets() so the offscreen
  // area is non-client as far as Windows is concerned. However we can't do this
  // with the top inset because otherwise Windows will give us a standard
  // titlebar. Thus we must compensate here to avoid having UI elements drift
  // off the top of the screen.
  if (frame()->IsFullscreen() && !restored)
    return 0;
  return GetSystemMetrics(SM_CYSIZEFRAME);
}

int GlassBrowserFrameView::TopAreaHeight(bool restored) const {
  if (frame()->IsFullscreen() && !restored)
    return 0;

  const int top = FrameTopBorderThickness(restored);
  // The tab top inset is equal to the height of any shadow region above the
  // tabs, plus a 1 px top stroke.  In maximized mode, we want to push the
  // shadow region off the top of the screen but leave the top stroke.
  if (IsMaximized() && !restored)
    return top - GetLayoutInsets(TAB).top() + 1;

  // Besides the frame border, there's empty space atop the window in restored
  // mode, to use to drag the window around.
  constexpr int kNonClientRestoredExtraThickness = 11;
  return top + kNonClientRestoredExtraThickness;
}

int GlassBrowserFrameView::TitlebarMaximizedVisualHeight() const {
  return display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYCAPTION);
}

int GlassBrowserFrameView::TitlebarHeight(bool restored) const {
  if (frame()->IsFullscreen() && !restored)
    return 0;
  // The titlebar's actual height is the same in restored and maximized, but
  // some of it is above the screen in maximized mode. See the comment in
  // FrameTopBorderThicknessPx().
  return TitlebarMaximizedVisualHeight() + FrameTopBorderThickness(false);
}

int GlassBrowserFrameView::WindowTopY() const {
  // The window top is SM_CYSIZEFRAME pixels when maximized (see the comment in
  // FrameTopBorderThickness()) and floor(system dsf) pixels when restored.
  // Unfortunately we can't represent either of those at hidpi without using
  // non-integral dips, so we return the closest reasonable values instead.
  return IsMaximized() ? FrameTopBorderThickness(false) : 1;
}

int GlassBrowserFrameView::MinimizeButtonX() const {
  // When CaptionButtonsOnLeadingEdge() is true call
  // frame()->GetMinimizeButtonOffset() directly, because minimize_button_->x()
  // will give the wrong edge of the button.
  DCHECK(!CaptionButtonsOnLeadingEdge());
  // If we're drawing the button we can query the layout directly, otherwise we
  // need to ask Windows where the minimize button is.
  // TODO(bsep): Ideally these would always be the same. When we're always
  // custom drawing the caption buttons, remove GetMinimizeButtonOffset().
  return ShouldCustomDrawSystemTitlebar() ? minimize_button_->x()
                                          : frame()->GetMinimizeButtonOffset();
}

int GlassBrowserFrameView::TabStripCaptionSpacing() const {
  // For Material Refresh, the end of the tabstrip contains empty space to
  // ensure the window remains draggable, which is sufficient padding to the
  // other tabstrip contents.
  using MD = ui::MaterialDesignController;
  if (MD::IsRefreshUi())
    return 0;

  // In restored mode, the New Tab button isn't at the same height as the
  // caption buttons, but the space will look cluttered if it actually slides
  // under them, so we stop it when the gap between the two is down to 5 px.
  constexpr int kNewTabCaptionRestoredSpacing = 5;
  // In maximized mode, where the New Tab button and the caption buttons are at
  // similar vertical coordinates, we need to reserve a larger, 16 px gap to
  // avoid looking too cluttered.
  constexpr int kNewTabCaptionMaximizedSpacing = 16;
  const int caption_spacing = IsMaximized() ? kNewTabCaptionMaximizedSpacing
                                            : kNewTabCaptionRestoredSpacing;

  // The profile switcher button is optionally displayed to the left of the
  // minimize button.
  views::View* profile_switcher = GetProfileSwitcherButton();
  if (!profile_switcher)
    return caption_spacing;

  int profile_spacing =
      profile_switcher->width() + kProfileSwitcherButtonOffset;

  // In non-maximized mode, allow the new tab button to slide completely under
  // the profile switcher button.
  if (!IsMaximized()) {
    const bool incognito = browser_view()->tabstrip()->IsIncognito();
    profile_spacing -= GetLayoutSize(NEW_TAB_BUTTON, incognito).width();
  }

  return std::max(caption_spacing, profile_spacing);
}

bool GlassBrowserFrameView::IsToolbarVisible() const {
  return browser_view()->IsToolbarVisible() &&
      !browser_view()->toolbar()->GetPreferredSize().IsEmpty();
}

bool GlassBrowserFrameView::ShowCustomIcon() const {
  // Don't show the window icon when the incognito badge is visible, since
  // they're competing for the same space.
  return !profile_indicator_icon() && ShouldCustomDrawSystemTitlebar() &&
         browser_view()->ShouldShowWindowIcon();
}

bool GlassBrowserFrameView::ShowCustomTitle() const {
  return ShouldCustomDrawSystemTitlebar() &&
         browser_view()->ShouldShowWindowTitle();
}

bool GlassBrowserFrameView::ShowSystemIcon() const {
  return !ShouldCustomDrawSystemTitlebar() &&
         browser_view()->ShouldShowWindowIcon();
}

SkColor GlassBrowserFrameView::GetTitlebarColor() const {
  return GetFrameColor();
}

Windows10CaptionButton* GlassBrowserFrameView::CreateCaptionButton(
    ViewID button_type,
    int accessible_name_resource_id) {
  Windows10CaptionButton* button = new Windows10CaptionButton(
      this, button_type,
      l10n_util::GetStringUTF16(accessible_name_resource_id));
  AddChildView(button);
  return button;
}

void GlassBrowserFrameView::PaintTitlebar(gfx::Canvas* canvas) const {
  TRACE_EVENT0("views.frame", "GlassBrowserFrameView::PaintTitlebar");
  gfx::Rect tabstrip_bounds = GetBoundsForTabStrip(browser_view()->tabstrip());

  cc::PaintFlags flags;
  gfx::ScopedCanvas scoped_canvas(canvas);
  float scale = canvas->UndoDeviceScaleFactor();
  // This is the pixel-accurate version of WindowTopY(). Scaling the DIP values
  // here compounds precision error, which exposes unpainted client area. When
  // restored it uses the system dsf instead of the per-monitor dsf to match
  // Windows' behavior.
  const int y = IsMaximized() ? FrameTopBorderThicknessPx(false)
                              : std::floor(display::win::GetDPIScale());

  // Draw the top of the accent border.
  //
  // We let the DWM do this for the other sides of the window by insetting the
  // client area to leave nonclient area available. However, along the top
  // window edge, we have to have zero nonclient area or the DWM will draw a
  // full native titlebar outside our client area. See
  // BrowserDesktopWindowTreeHostWin::GetClientAreaInsets().
  //
  // We could ask the DWM to draw the top accent border in the client area (by
  // calling DwmExtendFrameIntoClientArea() in
  // BrowserDesktopWindowTreeHostWin::UpdateDWMFrame()), but this requires
  // that we leave part of the client surface transparent. If we draw this
  // ourselves, we can make the client surface fully opaque and avoid the
  // power consumption needed for DWM to blend the window contents.
  //
  // So the accent border also has to be opaque, but native inactive borders
  // are #494949 with 47% alpha. Against white (the most visible case) this is
  // #AAAAAA, so we color with that normally. However, when the titlebar is dark
  // that color sometimes stands out badly. In that case we lighten the titlebar
  // color slightly, which creates a subtle highlight effect. This isn't exactly
  // native but it looks good given our constraints.
  const SkColor titlebar_color = GetTitlebarColor();
  const SkColor inactive_border_color =
      color_utils::IsDark(titlebar_color)
          ? color_utils::BlendTowardOppositeLuma(titlebar_color, 0x0F)
          : SkColorSetRGB(0xAA, 0xAA, 0xAA);
  flags.setColor(
      ShouldPaintAsActive()
          ? GetThemeProvider()->GetColor(ThemeProperties::COLOR_ACCENT_BORDER)
          : inactive_border_color);
  canvas->DrawRect(gfx::RectF(0, 0, width() * scale, y), flags);

  const gfx::Rect titlebar_rect = gfx::ToEnclosingRect(
      gfx::RectF(0, y, width() * scale, tabstrip_bounds.bottom() * scale - y));
  // Paint the titlebar first so we have a background if an area isn't covered
  // by the theme image.
  flags.setColor(titlebar_color);
  canvas->DrawRect(titlebar_rect, flags);
  const gfx::ImageSkia frame_image = GetFrameImage();
  if (!frame_image.isNull()) {
    canvas->TileImageInt(frame_image, 0, 0, titlebar_rect.x(),
                         titlebar_rect.y(), titlebar_rect.width(),
                         titlebar_rect.height(), scale);
  }
  const gfx::ImageSkia frame_overlay_image = GetFrameOverlayImage();
  if (!frame_overlay_image.isNull()) {
    canvas->DrawImageInt(frame_overlay_image, 0, 0, frame_overlay_image.width(),
                         frame_overlay_image.height(), titlebar_rect.x(),
                         titlebar_rect.y(), frame_overlay_image.width() * scale,
                         frame_overlay_image.height() * scale, true);
  }

  if (ShowCustomTitle()) {
    const SkAlpha title_alpha =
        ShouldPaintAsActive() ? SK_AlphaOPAQUE : kInactiveTitlebarFeatureAlpha;
    const SkColor title_color = SkColorSetA(
        color_utils::BlendTowardOppositeLuma(titlebar_color, SK_AlphaOPAQUE),
        title_alpha);
    window_title_->SetEnabledColor(title_color);
  }
}

void GlassBrowserFrameView::PaintClientEdge(gfx::Canvas* canvas) const {
  // Draw the client edge images.
  gfx::Rect client_bounds = CalculateClientAreaBounds();
  const int x = client_bounds.x();
  const int y = client_bounds.y() + browser_view()->GetToolbarBounds().y();
  const int right = client_bounds.right();
  const int bottom = std::max(y, height() - ClientBorderThickness(false));

  const ui::ThemeProvider* tp = GetThemeProvider();
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    const gfx::ImageSkia* const right_image =
        tp->GetImageSkiaNamed(IDR_CONTENT_RIGHT_SIDE);
    const int img_w = right_image->width();
    const int height = bottom - y;
    canvas->TileImageInt(*right_image, right, y, img_w, height);
    canvas->DrawImageInt(
        *tp->GetImageSkiaNamed(IDR_CONTENT_BOTTOM_RIGHT_CORNER), right, bottom);
    const gfx::ImageSkia* const bottom_image =
        tp->GetImageSkiaNamed(IDR_CONTENT_BOTTOM_CENTER);
    canvas->TileImageInt(*bottom_image, x, bottom, client_bounds.width(),
                         bottom_image->height());
    canvas->DrawImageInt(*tp->GetImageSkiaNamed(IDR_CONTENT_BOTTOM_LEFT_CORNER),
                         x - img_w, bottom);
    canvas->TileImageInt(*tp->GetImageSkiaNamed(IDR_CONTENT_LEFT_SIDE),
                         x - img_w, y, img_w, height);
  }
  FillClientEdgeRects(x, y, right, bottom,
                      tp->GetColor(ThemeProperties::COLOR_TOOLBAR), canvas);
}

void GlassBrowserFrameView::FillClientEdgeRects(int x,
                                                int y,
                                                int right,
                                                int bottom,
                                                SkColor color,
                                                gfx::Canvas* canvas) const {
  gfx::Rect side(x - kClientEdgeThickness, y, kClientEdgeThickness,
                 bottom + kClientEdgeThickness - y);
  canvas->FillRect(side, color);
  canvas->FillRect(gfx::Rect(x, bottom, right - x, kClientEdgeThickness),
                   color);
  side.set_x(right);
  canvas->FillRect(side, color);
}

void GlassBrowserFrameView::LayoutProfileSwitcher() {
  if (!browser_view()->IsRegularOrGuestSession())
    return;

  View* profile_switcher = GetProfileSwitcherButton();
  if (!profile_switcher)
    return;

  gfx::Size button_size = profile_switcher->GetPreferredSize();
  int button_width = button_size.width();
  int button_height = button_size.height();

  int button_x;
  if (CaptionButtonsOnLeadingEdge()) {
    button_x = width() - frame()->GetMinimizeButtonOffset() +
               kProfileSwitcherButtonOffset;
  } else {
    button_x = MinimizeButtonX() - kProfileSwitcherButtonOffset - button_width;
  }

  int button_y = WindowTopY();
  if (IsMaximized()) {
    // In maximized mode the caption buttons appear only 19 pixels high, but
    // their contents are aligned as if they were 20 pixels high and extended
    // 1 pixel off the top of the screen. We position the profile switcher
    // button the same way to match.
    button_y -= 1;
  }

  // Shrink the button height when it's atop part of the tabstrip. In RTL the
  // new tab button is on the left, so it can never slide under the avatar
  // button, which is still on the right [http://crbug.com/560619].
  TabStrip* tabstrip = browser_view()->tabstrip();
  if (tabstrip && !CaptionButtonsOnLeadingEdge() &&
      (tabstrip->new_tab_button_bounds().right() > button_x))
    button_height = profile_switcher->GetMinimumSize().height();

  profile_switcher->SetBounds(button_x, button_y, button_width, button_height);
}

void GlassBrowserFrameView::LayoutIncognitoIcon() {
  const gfx::Size size(GetIncognitoAvatarIcon().size());
  int x = ClientBorderThickness(false);
  // In RTL, the icon needs to start after the caption buttons.
  if (CaptionButtonsOnLeadingEdge()) {
    x = width() - frame()->GetMinimizeButtonOffset() +
        (GetProfileSwitcherButton() ? (GetProfileSwitcherButton()->width() +
                                       kProfileSwitcherButtonOffset)
                                    : 0);
  }
  const int bottom = GetTopInset(false) + browser_view()->GetTabStripHeight() -
                     GetAvatarIconPadding();
  incognito_bounds_.SetRect(
      x + (profile_indicator_icon() ? GetAvatarIconPadding() : 0),
      bottom - size.height(), profile_indicator_icon() ? size.width() : 0,
      size.height());
  if (profile_indicator_icon())
    profile_indicator_icon()->SetBoundsRect(incognito_bounds_);
}

void GlassBrowserFrameView::LayoutTitleBar() {
  TRACE_EVENT0("views.frame", "GlassBrowserFrameView::LayoutTitleBar");
  if (!ShowCustomIcon() && !ShowCustomTitle())
    return;

  gfx::Rect window_icon_bounds;
  const int icon_size =
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYSMICON);
  constexpr int kIconMaximizedLeftMargin = 2;
  const int titlebar_visual_height =
      IsMaximized() ? TitlebarMaximizedVisualHeight() : TitlebarHeight(false);
  // Don't include the area above the screen when maximized. However it only
  // looks centered if we start from y=0 when restored.
  const int window_top = IsMaximized() ? WindowTopY() : 0;
  int x = IsMaximized()
              ? kIconMaximizedLeftMargin
              : display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME);

  const int y = window_top + (titlebar_visual_height - icon_size) / 2;
  window_icon_bounds = gfx::Rect(x, y, icon_size, icon_size);

  constexpr int kIconTitleSpacing = 5;
  if (ShowCustomIcon()) {
    window_icon_->SetBoundsRect(window_icon_bounds);
    x = window_icon_bounds.right() + kIconTitleSpacing;
  } else if (profile_indicator_icon()) {
    x = profile_indicator_icon()->bounds().right() + kIconTitleSpacing;
  }

  if (ShowCustomTitle()) {
    window_title_->SetText(browser_view()->GetWindowTitle());
    const int max_text_width = std::max(0, MinimizeButtonX() - x);
    window_title_->SetBounds(x, window_icon_bounds.y(), max_text_width,
                             window_icon_bounds.height());
    window_title_->SetAutoColorReadabilityEnabled(false);
  }
}

void GlassBrowserFrameView::LayoutCaptionButton(Windows10CaptionButton* button,
                                                int previous_button_x) {
  TRACE_EVENT0("views.frame", "GlassBrowserFrameView::LayoutCaptionButton");
  gfx::Size button_size = button->GetPreferredSize();
  button->SetBounds(previous_button_x - button_size.width(), WindowTopY(),
                    button_size.width(), button_size.height());
}

void GlassBrowserFrameView::LayoutCaptionButtons() {
  TRACE_EVENT0("views.frame", "GlassBrowserFrameView::LayoutCaptionButtons");
  LayoutCaptionButton(close_button_, width());

  LayoutCaptionButton(restore_button_, close_button_->x());
  restore_button_->SetVisible(IsMaximized());

  LayoutCaptionButton(maximize_button_, close_button_->x());
  maximize_button_->SetVisible(!IsMaximized());

  LayoutCaptionButton(minimize_button_, maximize_button_->x());
}

void GlassBrowserFrameView::LayoutClientView() {
  client_view_bounds_ = CalculateClientAreaBounds();
}

gfx::Insets GlassBrowserFrameView::GetClientAreaInsets(bool restored) const {
  if (!browser_view()->IsTabStripVisible()) {
    const int top =
        ShouldCustomDrawSystemTitlebar() ? TitlebarHeight(restored) : 0;
    return gfx::Insets(top, 0, 0, 0);
  }

  const int top_height = TopAreaHeight(restored);
  const int border_thickness = ClientBorderThickness(restored);
  return gfx::Insets(top_height,
                     border_thickness,
                     border_thickness,
                     border_thickness);
}

gfx::Rect GlassBrowserFrameView::CalculateClientAreaBounds() const {
  gfx::Rect bounds(GetLocalBounds());
  bounds.Inset(GetClientAreaInsets(false));
  return bounds;
}

void GlassBrowserFrameView::StartThrobber() {
  DCHECK(ShowSystemIcon());
  if (!throbber_running_) {
    throbber_running_ = true;
    throbber_frame_ = 0;
    InitThrobberIcons();
    SendMessage(views::HWNDForWidget(frame()), WM_SETICON,
                static_cast<WPARAM>(ICON_SMALL),
                reinterpret_cast<LPARAM>(throbber_icons_[throbber_frame_]));
  }
}

void GlassBrowserFrameView::StopThrobber() {
  DCHECK(ShowSystemIcon());
  if (throbber_running_) {
    throbber_running_ = false;

    base::win::ScopedHICON previous_small_icon;
    base::win::ScopedHICON previous_big_icon;
    HICON small_icon = nullptr;
    HICON big_icon = nullptr;

    gfx::ImageSkia icon = browser_view()->GetWindowIcon();
    if (!icon.isNull()) {
      // Keep previous icons alive as long as they are referenced by the HWND.
      previous_small_icon = std::move(small_window_icon_);
      previous_big_icon = std::move(big_window_icon_);

      // Take responsibility for eventually destroying the created icons.
      small_window_icon_ = CreateHICONFromSkBitmapSizedTo(
          icon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
      big_window_icon_ = CreateHICONFromSkBitmapSizedTo(
          icon, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

      small_icon = small_window_icon_.get();
      big_icon = big_window_icon_.get();
    }

    // Fallback to class icon.
    if (!small_icon) {
      small_icon = reinterpret_cast<HICON>(
          GetClassLongPtr(views::HWNDForWidget(frame()), GCLP_HICONSM));
    }
    if (!big_icon) {
      big_icon = reinterpret_cast<HICON>(
          GetClassLongPtr(views::HWNDForWidget(frame()), GCLP_HICON));
    }

    // This will reset the icon which we set in the throbber code.
    // WM_SETICON with null icon restores the icon for title bar but not
    // for taskbar. See http://crbug.com/29996
    SendMessage(views::HWNDForWidget(frame()), WM_SETICON,
                static_cast<WPARAM>(ICON_SMALL),
                reinterpret_cast<LPARAM>(small_icon));

    SendMessage(views::HWNDForWidget(frame()), WM_SETICON,
                static_cast<WPARAM>(ICON_BIG),
                reinterpret_cast<LPARAM>(big_icon));
  }
}

void GlassBrowserFrameView::DisplayNextThrobberFrame() {
  throbber_frame_ = (throbber_frame_ + 1) % kThrobberIconCount;
  SendMessage(views::HWNDForWidget(frame()), WM_SETICON,
              static_cast<WPARAM>(ICON_SMALL),
              reinterpret_cast<LPARAM>(throbber_icons_[throbber_frame_]));
}

// static
void GlassBrowserFrameView::InitThrobberIcons() {
  static bool initialized = false;
  if (!initialized) {
    for (int i = 0; i < kThrobberIconCount; ++i) {
      throbber_icons_[i] =
          ui::LoadThemeIconFromResourcesDataDLL(IDI_THROBBER_01 + i);
      DCHECK(throbber_icons_[i]);
    }
    initialized = true;
  }
}
