// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"

#include "base/command_line.h"
#include "base/containers/adapters.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/profiles/profile_indicator_icon.h"
#include "chrome/common/chrome_switches.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/font.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"

namespace {

constexpr int kCaptionButtonHeight = 18;

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
// Default spacing around window caption buttons.
constexpr int kCaptionButtonSpacing = 2;
#else
constexpr int kCaptionButtonSpacing = 0;
#endif

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameViewLayout, public:

// statics

// The content edge images have a shadow built into them.
const int OpaqueBrowserFrameViewLayout::kContentEdgeShadowThickness = 2;

// Besides the frame border, there's empty space atop the window in restored
// mode, to use to drag the window around.
const int OpaqueBrowserFrameViewLayout::kNonClientRestoredExtraThickness = 11;

// The frame border is only visible in restored mode and is hardcoded to 4 px on
// each side regardless of the system window border size.
const int OpaqueBrowserFrameViewLayout::kFrameBorderThickness = 4;

// The titlebar has a 2 px 3D edge along the top.
const int OpaqueBrowserFrameViewLayout::kTitlebarTopEdgeThickness = 2;

// The icon is inset 1 px from the left frame border.
const int OpaqueBrowserFrameViewLayout::kIconLeftSpacing = 1;

// There is a 4 px gap between the icon and the title text.
const int OpaqueBrowserFrameViewLayout::kIconTitleSpacing = 4;

// The horizontal spacing to use in most cases when laying out things near the
// caption button area.
const int OpaqueBrowserFrameViewLayout::kCaptionSpacing = 5;

// The minimum vertical padding between the bottom of the caption buttons and
// the top of the content shadow.
const int OpaqueBrowserFrameViewLayout::kCaptionButtonBottomPadding = 3;

// When the title bar is condensed to one row (as when maximized), the New Tab
// button and the caption buttons are at similar vertical coordinates, so we
// need to reserve a larger, 16 px gap to avoid looking too cluttered.
const int OpaqueBrowserFrameViewLayout::kNewTabCaptionCondensedSpacing = 16;

OpaqueBrowserFrameViewLayout::OpaqueBrowserFrameViewLayout()
    : new_avatar_button_(nullptr),
      leading_button_start_(0),
      trailing_button_start_(0),
      minimum_size_for_buttons_(0),
      has_leading_buttons_(false),
      has_trailing_buttons_(false),
      extra_caption_y_(kCaptionButtonSpacing),
      forced_window_caption_spacing_(-1),
      minimize_button_(nullptr),
      maximize_button_(nullptr),
      restore_button_(nullptr),
      close_button_(nullptr),
      window_icon_(nullptr),
      window_title_(nullptr),
      incognito_icon_(nullptr),
      trailing_buttons_{views::FRAME_BUTTON_MINIMIZE,
                        views::FRAME_BUTTON_MAXIMIZE,
                        views::FRAME_BUTTON_CLOSE} {}

OpaqueBrowserFrameViewLayout::~OpaqueBrowserFrameViewLayout() {}

void OpaqueBrowserFrameViewLayout::SetButtonOrdering(
    const std::vector<views::FrameButton>& leading_buttons,
    const std::vector<views::FrameButton>& trailing_buttons) {
  leading_buttons_ = leading_buttons;
  trailing_buttons_ = trailing_buttons;
}

gfx::Rect OpaqueBrowserFrameViewLayout::GetBoundsForTabStrip(
    const gfx::Size& tabstrip_preferred_size,
    int available_width) const {
  const int x = GetTabStripLeftInset();
  available_width -= x + TabStripCaptionSpacing() + trailing_button_start_;
  return gfx::Rect(x, GetTabStripInsetsTop(false), std::max(0, available_width),
                   tabstrip_preferred_size.height());
}

gfx::Size OpaqueBrowserFrameViewLayout::GetMinimumSize(
    int available_width) const {
  gfx::Size min_size = delegate_->GetBrowserViewMinimumSize();
  int border_thickness = NonClientBorderThickness();
  min_size.Enlarge(2 * border_thickness,
                   NonClientTopHeight(false) + border_thickness);

  // Ensure that we can, at minimum, hold our window controls and avatar icon.
  min_size.set_width(std::max(min_size.width(), minimum_size_for_buttons_));

  // Ensure that the minimum width is enough to hold a minimum width tab strip
  // at its usual insets.
  if (delegate_->IsTabStripVisible()) {
    gfx::Size preferred_size = delegate_->GetTabstripPreferredSize();
    const int min_tabstrip_width = preferred_size.width();
    const int caption_spacing = TabStripCaptionSpacing();
    min_size.Enlarge(min_tabstrip_width + caption_spacing, 0);
  }

  return min_size;
}

int OpaqueBrowserFrameViewLayout::GetTabStripLeftInset() const {
  return leading_button_start_ + OpaqueBrowserFrameView::GetAvatarIconPadding();
}

gfx::Rect OpaqueBrowserFrameViewLayout::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  int top_height = NonClientTopHeight(false);
  int border_thickness = NonClientBorderThickness();
  return gfx::Rect(std::max(0, client_bounds.x() - border_thickness),
                   std::max(0, client_bounds.y() - top_height),
                   client_bounds.width() + (2 * border_thickness),
                   client_bounds.height() + top_height + border_thickness);
}

int OpaqueBrowserFrameViewLayout::FrameBorderThickness(bool restored) const {
  return (!restored && (IsTitleBarCondensed() || delegate_->IsFullscreen())) ?
      0 : kFrameBorderThickness;
}

int OpaqueBrowserFrameViewLayout::NonClientBorderThickness() const {
  const int frame = FrameBorderThickness(false);
  // When we fill the screen, we don't show a client edge.
  return (IsTitleBarCondensed() || delegate_->IsFullscreen()) ?
      frame : (frame + views::NonClientFrameView::kClientEdgeThickness);
}

int OpaqueBrowserFrameViewLayout::NonClientTopHeight(bool restored) const {
  if (delegate_->ShouldShowWindowTitle()) {
    // The + 2 here puts at least 1 px of space on top and bottom of the icon.
    const int icon_height =
        TitlebarTopThickness(restored) + delegate_->GetIconSize() + 2;
    const int caption_button_height = DefaultCaptionButtonY(restored) +
                                      kCaptionButtonHeight +
                                      kCaptionButtonBottomPadding;
    return std::max(icon_height, caption_button_height) +
        kContentEdgeShadowThickness;
  }

  int thickness = FrameBorderThickness(restored);
  // The tab top inset is equal to the height of any shadow region above the
  // tabs, plus a 1 px top stroke.  In maximized mode, we want to push the
  // shadow region off the top of the screen but leave the top stroke.
  if (!restored && delegate_->IsTabStripVisible() && IsTitleBarCondensed())
    thickness -= GetLayoutInsets(TAB).top() - 1;
  return thickness;
}

int OpaqueBrowserFrameViewLayout::GetTabStripInsetsTop(bool restored) const {
  const int top = NonClientTopHeight(restored);
  return (!restored && (IsTitleBarCondensed() || delegate_->IsFullscreen()))
             ? top
             : (top + kNonClientRestoredExtraThickness);
}

int OpaqueBrowserFrameViewLayout::TitlebarTopThickness(bool restored) const {
  if (!delegate_->UseCustomFrame())
    return 0;
  return (restored || !IsTitleBarCondensed()) ?
      kTitlebarTopEdgeThickness : FrameBorderThickness(false);
}

int OpaqueBrowserFrameViewLayout::DefaultCaptionButtonY(bool restored) const {
  // Maximized buttons start at window top, since the window has no border. This
  // offset is for the image (the actual clickable bounds extend all the way to
  // the top to take Fitts' Law into account).
  const int frame = (!restored && IsTitleBarCondensed()) ?
      FrameBorderThickness(false) :
      views::NonClientFrameView::kFrameShadowThickness;
  return frame + extra_caption_y_;
}

int OpaqueBrowserFrameViewLayout::CaptionButtonY(
    chrome::FrameButtonDisplayType button_id,
    bool restored) const {
  return DefaultCaptionButtonY(restored);
}

int OpaqueBrowserFrameViewLayout::TopAreaPadding() const {
  return FrameBorderThickness(false);
}

gfx::Rect OpaqueBrowserFrameViewLayout::IconBounds() const {
  return window_icon_bounds_;
}

gfx::Rect OpaqueBrowserFrameViewLayout::CalculateClientAreaBounds(
    int width,
    int height) const {
  int top_height = NonClientTopHeight(false);
  int border_thickness = NonClientBorderThickness();
  return gfx::Rect(border_thickness, top_height,
                   std::max(0, width - (2 * border_thickness)),
                   std::max(0, height - top_height - border_thickness));
}

chrome::FrameButtonDisplayType
OpaqueBrowserFrameViewLayout::GetButtonDisplayType(
    views::FrameButton button_id) const {
  switch (button_id) {
    case views::FRAME_BUTTON_MINIMIZE:
      return chrome::FrameButtonDisplayType::kMinimize;
    case views::FRAME_BUTTON_MAXIMIZE:
      return delegate_->IsMaximized()
                 ? chrome::FrameButtonDisplayType::kRestore
                 : chrome::FrameButtonDisplayType::kMaximize;
    case views::FRAME_BUTTON_CLOSE:
      return chrome::FrameButtonDisplayType::kClose;
    default:
      NOTREACHED();
      return chrome::FrameButtonDisplayType::kClose;
  }
}

int OpaqueBrowserFrameViewLayout::GetWindowCaptionSpacing(
    views::FrameButton button_id,
    bool leading_spacing,
    bool is_leading_button) const {
  if (leading_spacing) {
    if (is_leading_button) {
      // If we're the first button and maximized, add width to the right
      // hand side of the screen.
      return (IsTitleBarCondensed() && is_leading_button)
                 ? (kFrameBorderThickness -
                    views::NonClientFrameView::kFrameShadowThickness)
                 : 0;
    }
    if (forced_window_caption_spacing_ >= 0)
      return forced_window_caption_spacing_;
    return kCaptionButtonSpacing;
  }
  return 0;
}

bool OpaqueBrowserFrameViewLayout::IsTitleBarCondensed() const {
  // If there are no caption buttons, there is no need to have an uncondensed
  // title bar. If the window is maximized, the title bar is condensed
  // regardless of whether there are caption buttons.
  return !delegate_->ShouldShowCaptionButtons() || delegate_->IsMaximized();
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameViewLayout, protected:

void OpaqueBrowserFrameViewLayout::LayoutNewStyleAvatar(views::View* host) {
  if (!new_avatar_button_)
    return;

  int button_width = new_avatar_button_->GetPreferredSize().width();
  int button_width_with_offset = button_width;
  if (!trailing_buttons_.empty())
    button_width_with_offset += kCaptionSpacing;

  int button_x =
      host->width() - trailing_button_start_ - button_width_with_offset;
  int button_y = DefaultCaptionButtonY(!IsTitleBarCondensed());

  minimum_size_for_buttons_ += button_width_with_offset;
  trailing_button_start_ += button_width_with_offset;

  // In non-maximized mode, allow the new tab button to completely slide under
  // the avatar button.
  if (!IsTitleBarCondensed()) {
    trailing_button_start_ -=
        GetLayoutSize(NEW_TAB_BUTTON, delegate_->IsIncognito()).width() +
        kCaptionSpacing;
  }

  new_avatar_button_->SetBounds(button_x, button_y, button_width,
                                kCaptionButtonHeight);
}

bool OpaqueBrowserFrameViewLayout::ShouldDrawImageMirrored(
    views::ImageButton* button,
    ButtonAlignment alignment) const {
  return alignment == ALIGN_LEADING && !has_leading_buttons_ &&
         button == close_button_;
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameViewLayout, private:

bool OpaqueBrowserFrameViewLayout::ShouldIncognitoIconBeOnRight() const {
  // The incognito should be shown either on the end of the left or the
  // beginning of the right, depending on which side has fewer buttons.
  return trailing_buttons_.size() < leading_buttons_.size();
}

int OpaqueBrowserFrameViewLayout::TabStripCaptionSpacing() const {
  // For Material Refresh, the end of the tabstrip contains empty space to
  // ensure the window remains draggable, which is sufficient padding to the
  // other tabstrip contents.
  using MD = ui::MaterialDesignController;
  if (MD::IsRefreshUi())
    return 0;

  return (has_trailing_buttons_ && IsTitleBarCondensed()) ?
      kNewTabCaptionCondensedSpacing : kCaptionSpacing;
}

void OpaqueBrowserFrameViewLayout::LayoutWindowControls(views::View* host) {
  // Keep a list of all buttons that we don't show.
  std::vector<views::FrameButton> buttons_not_shown;
  buttons_not_shown.push_back(views::FRAME_BUTTON_MAXIMIZE);
  buttons_not_shown.push_back(views::FRAME_BUTTON_MINIMIZE);
  buttons_not_shown.push_back(views::FRAME_BUTTON_CLOSE);

  if (delegate_->ShouldShowCaptionButtons()) {
    for (const auto& button : leading_buttons_) {
      ConfigureButton(host, button, ALIGN_LEADING);
      buttons_not_shown.erase(
          std::remove(buttons_not_shown.begin(), buttons_not_shown.end(),
                      button),
          buttons_not_shown.end());
    }

    for (const auto& button : base::Reversed(trailing_buttons_)) {
      ConfigureButton(host, button, ALIGN_TRAILING);
      buttons_not_shown.erase(
          std::remove(buttons_not_shown.begin(), buttons_not_shown.end(),
                      button),
          buttons_not_shown.end());
    }
  }

  for (const auto& button : buttons_not_shown)
    HideButton(button);
}

void OpaqueBrowserFrameViewLayout::LayoutTitleBar(views::View* host) {
  bool use_hidden_icon_location = true;

  int size = delegate_->GetIconSize();
  bool should_show_icon = delegate_->ShouldShowWindowIcon() && window_icon_;
  bool should_show_title = delegate_->ShouldShowWindowTitle() && window_title_;

  if (should_show_icon || should_show_title) {
    use_hidden_icon_location = false;

    // Our frame border has a different "3D look" than Windows'.  Theirs has
    // a more complex gradient on the top that they push their icon/title
    // below; then the maximized window cuts this off and the icon/title are
    // centered in the remaining space.  Because the apparent shape of our
    // border is simpler, using the same positioning makes things look
    // slightly uncentered with restored windows, so when the window is
    // restored, instead of calculating the remaining space from below the
    // frame border, we calculate from below the 3D edge.
    const int unavailable_px_at_top = TitlebarTopThickness(false);
    // When the icon is shorter than the minimum space we reserve for the
    // caption button, we vertically center it.  We want to bias rounding to
    // put extra space below the icon, since we'll use the same Y coordinate for
    // the title, and the majority of the font weight is below the centerline.
    const int icon_height =
        unavailable_px_at_top + size + kContentEdgeShadowThickness;
    const int y =
        unavailable_px_at_top + (NonClientTopHeight(false) - icon_height) / 2;

    window_icon_bounds_ = gfx::Rect(leading_button_start_ + kIconLeftSpacing, y,
                                    size, size);
    leading_button_start_ += size + kIconLeftSpacing;
    minimum_size_for_buttons_ += size + kIconLeftSpacing;
  }

  if (should_show_icon)
    window_icon_->SetBoundsRect(window_icon_bounds_);

  if (window_title_) {
    window_title_->SetVisible(should_show_title);
    if (should_show_title) {
      window_title_->SetText(delegate_->GetWindowTitle());

      int text_width = std::max(
          0, host->width() - trailing_button_start_ - kCaptionSpacing -
          leading_button_start_ - kIconTitleSpacing);
      window_title_->SetBounds(leading_button_start_ + kIconTitleSpacing,
                               window_icon_bounds_.y(),
                               text_width, window_icon_bounds_.height());
      leading_button_start_ += text_width + kIconTitleSpacing;
    }
  }

  if (use_hidden_icon_location) {
    if (has_leading_buttons_) {
      // There are window button icons on the left. Don't size the hidden window
      // icon that people can double click on to close the window.
      window_icon_bounds_ = gfx::Rect();
    } else {
      // We set the icon bounds to a small rectangle in the top leading corner
      // if there are no icons on the leading side.
      const int frame_thickness = FrameBorderThickness(false);
      window_icon_bounds_ = gfx::Rect(
          frame_thickness + kIconLeftSpacing, frame_thickness, size, size);
    }
  }
}

void OpaqueBrowserFrameViewLayout::LayoutIncognitoIcon(views::View* host) {
  const int old_button_size = leading_button_start_ + trailing_button_start_;

  // Any buttons/icon/title were laid out based on the frame border thickness,
  // but the tabstrip bounds need to be based on the non-client border thickness
  // on any side where there aren't other buttons forcing a larger inset.
  int min_button_width = NonClientBorderThickness();
  leading_button_start_ = std::max(leading_button_start_, min_button_width);
  // The trailing corner is a mirror of the leading one.
  trailing_button_start_ = std::max(trailing_button_start_, min_button_width);

  if (incognito_icon_) {
    const int pad = OpaqueBrowserFrameView::GetAvatarIconPadding();
    const gfx::Size size(delegate_->GetIncognitoAvatarIcon().size());
    const int incognito_width = pad + size.width();
    int x;
    if (ShouldIncognitoIconBeOnRight()) {
      trailing_button_start_ += incognito_width;
      x = host->width() - trailing_button_start_;
    } else {
      x = leading_button_start_ + pad;
      leading_button_start_ += incognito_width;
    }
    const int bottom =
        GetTabStripInsetsTop(false) + delegate_->GetTabStripHeight() - pad;
    incognito_icon_->SetBounds(x, bottom - size.height(), size.width(),
                               size.height());
  }

  minimum_size_for_buttons_ +=
      (leading_button_start_ + trailing_button_start_ - old_button_size);
}

void OpaqueBrowserFrameViewLayout::ConfigureButton(views::View* host,
                                                   views::FrameButton button_id,
                                                   ButtonAlignment alignment) {
  switch (button_id) {
    case views::FRAME_BUTTON_MINIMIZE: {
      minimize_button_->SetVisible(true);
      SetBoundsForButton(button_id, host, minimize_button_, alignment);
      break;
    }
    case views::FRAME_BUTTON_MAXIMIZE: {
      // When the window is restored, we show a maximized button; otherwise, we
      // show a restore button.
      bool is_restored = !delegate_->IsMaximized() && !delegate_->IsMinimized();
      views::ImageButton* invisible_button = is_restored ?
          restore_button_ : maximize_button_;
      invisible_button->SetVisible(false);

      views::ImageButton* visible_button = is_restored ?
          maximize_button_ : restore_button_;
      visible_button->SetVisible(true);
      SetBoundsForButton(button_id, host, visible_button, alignment);
      break;
    }
    case views::FRAME_BUTTON_CLOSE: {
      close_button_->SetVisible(true);
      SetBoundsForButton(button_id, host, close_button_, alignment);
      break;
    }
  }
}

void OpaqueBrowserFrameViewLayout::HideButton(views::FrameButton button_id) {
  switch (button_id) {
    case views::FRAME_BUTTON_MINIMIZE:
      minimize_button_->SetVisible(false);
      break;
    case views::FRAME_BUTTON_MAXIMIZE:
      restore_button_->SetVisible(false);
      maximize_button_->SetVisible(false);
      break;
    case views::FRAME_BUTTON_CLOSE:
      close_button_->SetVisible(false);
      break;
  }
}

void OpaqueBrowserFrameViewLayout::SetBoundsForButton(
    views::FrameButton button_id,
    views::View* host,
    views::ImageButton* button,
    ButtonAlignment alignment) {
  int caption_y = CaptionButtonY(GetButtonDisplayType(button_id), false);

  gfx::Size button_size = button->GetPreferredSize();

  button->SetImageAlignment(
      (alignment == ALIGN_LEADING) ?
          views::ImageButton::ALIGN_RIGHT : views::ImageButton::ALIGN_LEFT,
      views::ImageButton::ALIGN_BOTTOM);

  // There should always be the same number of non-shadow pixels visible to the
  // side of the caption buttons.  In maximized mode we extend buttons to the
  // screen top and the rightmost button to the screen right (or leftmost button
  // to the screen left, for left-aligned buttons) to obey Fitts' Law.
  bool title_bar_condensed = IsTitleBarCondensed();

  // When we are the first button on the leading side and are the close
  // button, we must flip ourselves, because the close button assets have
  // a little notch to fit in the rounded frame.
  button->SetDrawImageMirrored(ShouldDrawImageMirrored(button, alignment));

  int extra_width = TopAreaPadding();

  switch (alignment) {
    case ALIGN_LEADING: {
      int button_start_spacing =
          GetWindowCaptionSpacing(button_id, true, !has_leading_buttons_);

      leading_button_start_ += button_start_spacing;
      minimum_size_for_buttons_ += button_start_spacing;

      bool top_spacing_clickable = title_bar_condensed;
      bool start_spacing_clickable =
          title_bar_condensed && !has_leading_buttons_;
      button->SetBounds(
          leading_button_start_ - (start_spacing_clickable
                                       ? button_start_spacing + extra_width
                                       : 0),
          top_spacing_clickable ? 0 : caption_y,
          button_size.width() + (start_spacing_clickable
                                     ? button_start_spacing + extra_width
                                     : 0),
          button_size.height() + (top_spacing_clickable ? caption_y : 0));

      int button_end_spacing =
          GetWindowCaptionSpacing(button_id, false, !has_leading_buttons_);
      leading_button_start_ += button_size.width() + button_end_spacing;
      minimum_size_for_buttons_ += button_size.width() + button_end_spacing;
      has_leading_buttons_ = true;
      break;
    }
    case ALIGN_TRAILING: {
      int button_start_spacing =
          GetWindowCaptionSpacing(button_id, true, !has_trailing_buttons_);

      trailing_button_start_ += button_start_spacing;
      minimum_size_for_buttons_ += button_start_spacing;

      bool top_spacing_clickable = title_bar_condensed;
      bool start_spacing_clickable =
          title_bar_condensed && !has_trailing_buttons_;
      button->SetBounds(
          host->width() - trailing_button_start_ - button_size.width(),
          top_spacing_clickable ? 0 : caption_y,
          button_size.width() + (start_spacing_clickable
                                     ? button_start_spacing + extra_width
                                     : 0),
          button_size.height() + (top_spacing_clickable ? caption_y : 0));

      int button_end_spacing =
          GetWindowCaptionSpacing(button_id, false, !has_trailing_buttons_);
      trailing_button_start_ += button_size.width() + button_end_spacing;
      minimum_size_for_buttons_ += button_size.width() + button_end_spacing;
      has_trailing_buttons_ = true;
      break;
    }
  }
}

void OpaqueBrowserFrameViewLayout::SetView(int id, views::View* view) {
  // Why do things this way instead of having an Init() method, where we're
  // passed the views we'll handle? Because OpaqueBrowserFrameView doesn't own
  // all the views which are part of it. The avatar stuff, for example, will be
  // added and removed by the base class of OpaqueBrowserFrameView.
  switch (id) {
    case VIEW_ID_MINIMIZE_BUTTON:
      if (view) {
        DCHECK_EQ(std::string(views::ImageButton::kViewClassName),
                  view->GetClassName());
      }
      minimize_button_ = static_cast<views::ImageButton*>(view);
      break;
    case VIEW_ID_MAXIMIZE_BUTTON:
      if (view) {
        DCHECK_EQ(std::string(views::ImageButton::kViewClassName),
                  view->GetClassName());
      }
      maximize_button_ = static_cast<views::ImageButton*>(view);
      break;
    case VIEW_ID_RESTORE_BUTTON:
      if (view) {
        DCHECK_EQ(std::string(views::ImageButton::kViewClassName),
                  view->GetClassName());
      }
      restore_button_ = static_cast<views::ImageButton*>(view);
      break;
    case VIEW_ID_CLOSE_BUTTON:
      if (view) {
        DCHECK_EQ(std::string(views::ImageButton::kViewClassName),
                  view->GetClassName());
      }
      close_button_ = static_cast<views::ImageButton*>(view);
      break;
    case VIEW_ID_WINDOW_ICON:
      window_icon_ = view;
      break;
    case VIEW_ID_WINDOW_TITLE:
      if (view) {
        DCHECK_EQ(std::string(views::Label::kViewClassName),
                  view->GetClassName());
      }
      window_title_ = static_cast<views::Label*>(view);
      break;
    case VIEW_ID_PROFILE_INDICATOR_ICON:
      incognito_icon_ = view;
      break;
    case VIEW_ID_AVATAR_BUTTON:
      new_avatar_button_ = view;
      break;
    default:
      NOTIMPLEMENTED() << "Unknown view id " << id;
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////
// OpaqueBrowserFrameViewLayout, views::LayoutManager:

void OpaqueBrowserFrameViewLayout::Layout(views::View* host) {
  TRACE_EVENT0("views.frame", "OpaqueBrowserFrameViewLayout::Layout");
  // Reset all our data so that everything is invisible.
  int top_area_padding = TopAreaPadding();
  leading_button_start_ = top_area_padding;
  trailing_button_start_ = top_area_padding;
  minimum_size_for_buttons_ = leading_button_start_ + trailing_button_start_;
  has_leading_buttons_ = false;
  has_trailing_buttons_ = false;

  LayoutWindowControls(host);
  LayoutTitleBar(host);

  if (delegate_->IsRegularOrGuestSession())
    LayoutNewStyleAvatar(host);
  LayoutIncognitoIcon(host);

  client_view_bounds_ = CalculateClientAreaBounds(
      host->width(), host->height());
}

gfx::Size OpaqueBrowserFrameViewLayout::GetPreferredSize(
    const views::View* host) const {
  // This is never used; NonClientView::CalculatePreferredSize() will be called
  // instead.
  NOTREACHED();
  return gfx::Size();
}

void OpaqueBrowserFrameViewLayout::ViewAdded(views::View* host,
                                             views::View* view) {
  SetView(view->id(), view);
}

void OpaqueBrowserFrameViewLayout::ViewRemoved(views::View* host,
                                               views::View* view) {
  SetView(view->id(), nullptr);
}
