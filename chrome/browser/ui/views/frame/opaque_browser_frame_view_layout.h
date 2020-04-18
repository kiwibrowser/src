// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_LAYOUT_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_LAYOUT_H_

#include "base/macros.h"
#include "chrome/browser/ui/frame_button_display_types.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/window/frame_buttons.h"

class OpaqueBrowserFrameViewLayoutDelegate;

namespace views {
class ImageButton;
class Label;
}

// Calculates the position of the widgets in the opaque browser frame view.
//
// This is separated out for testing reasons. OpaqueBrowserFrameView has tight
// dependencies with Browser and classes that depend on Browser.
class OpaqueBrowserFrameViewLayout : public views::LayoutManager {
 public:
  // Constants used by OpaqueBrowserFrameView as well.
  static const int kContentEdgeShadowThickness;

  // Constants public for testing only.
  static const int kNonClientRestoredExtraThickness;
  static const int kFrameBorderThickness;
  static const int kTitlebarTopEdgeThickness;
  static const int kIconLeftSpacing;
  static const int kIconTitleSpacing;
  static const int kCaptionSpacing;
  static const int kCaptionButtonBottomPadding;
  static const int kNewTabCaptionCondensedSpacing;

  OpaqueBrowserFrameViewLayout();
  ~OpaqueBrowserFrameViewLayout() override;

  void set_delegate(OpaqueBrowserFrameViewLayoutDelegate* delegate) {
    delegate_ = delegate;
  }

  // Configures the button ordering in the frame.
  void SetButtonOrdering(
      const std::vector<views::FrameButton>& leading_buttons,
      const std::vector<views::FrameButton>& trailing_buttons);

  gfx::Rect GetBoundsForTabStrip(
      const gfx::Size& tabstrip_preferred_size,
      int available_width) const;

  gfx::Size GetMinimumSize(int available_width) const;

  // Distance between the left edge of the NonClientFrameView and the tab strip.
  int GetTabStripLeftInset() const;

  // Returns the bounds of the window required to display the content area at
  // the specified bounds.
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const;

  // Returns the thickness of the border that makes up the window frame edges.
  // This does not include any client edge.  If |restored| is true, acts as if
  // the window is restored regardless of the real mode.
  int FrameBorderThickness(bool restored) const;

  // Returns the thickness of the entire nonclient left, right, and bottom
  // borders, including both the window frame and any client edge.
  int NonClientBorderThickness() const;

  // Returns the height of the entire nonclient top border, from the edge of the
  // window to the top of the tabs. If |restored| is true, this is calculated as
  // if the window was restored, regardless of its current state.
  int NonClientTopHeight(bool restored) const;

  int GetTabStripInsetsTop(bool restored) const;

  // Returns the y-coordinate of the caption button when native frame buttons
  // are disabled.  Also used to position the profile chooser button.  If
  // |restored| is true, acts as if the window is restored regardless of the
  // real mode.
  int DefaultCaptionButtonY(bool restored) const;

  // Returns the y-coordinate of button |button_id|.  If |restored| is true,
  // acts as if the window is restored regardless of the real mode.
  virtual int CaptionButtonY(chrome::FrameButtonDisplayType button_id,
                             bool restored) const;

  // Returns the initial spacing between the edge of the browser window and the
  // first button.
  virtual int TopAreaPadding() const;

  // Returns the thickness of the 3D edge along the top of the titlebar.  If
  // |restored| is true, acts as if the window is restored regardless of the
  // real mode.
  int TitlebarTopThickness(bool restored) const;

  // Returns the bounds of the titlebar icon (or where the icon would be if
  // there was one).
  gfx::Rect IconBounds() const;

  // Returns the bounds of the client area for the specified view size.
  gfx::Rect CalculateClientAreaBounds(int width, int height) const;

  // Converts a FrameButton to a FrameButtonDisplayType, taking into
  // consideration the maximized state of the browser window.
  chrome::FrameButtonDisplayType GetButtonDisplayType(
      views::FrameButton button_id) const;

  // Returns the margin around button |button_id|.  If |leading_spacing| is
  // true, returns the left margin (in RTL), otherwise returns the right margin
  // (in RTL).  Extra margin may be added if |is_leading_button| is true.
  virtual int GetWindowCaptionSpacing(views::FrameButton button_id,
                                      bool leading_spacing,
                                      bool is_leading_button) const;

  void set_extra_caption_y(int extra_caption_y) {
    extra_caption_y_ = extra_caption_y;
  }

  void set_forced_window_caption_spacing_for_test(
      int forced_window_caption_spacing) {
    forced_window_caption_spacing_ = forced_window_caption_spacing;
  }

  const gfx::Rect& client_view_bounds() const { return client_view_bounds_; }

  // Determines whether the title bar is condensed vertically, as when the
  // window is maximized. If true, the title bar is just the height of a tab,
  // rather than having extra vertical space above the tabs. This also removes
  // the thick frame border and rounded corners.
  bool IsTitleBarCondensed() const;

 protected:
  // Whether a specific button should be inserted on the leading or trailing
  // side.
  enum ButtonAlignment {
    ALIGN_LEADING,
    ALIGN_TRAILING
  };

  bool has_trailing_buttons() const { return has_trailing_buttons_; }

  virtual void LayoutNewStyleAvatar(views::View* host);

  virtual bool ShouldDrawImageMirrored(views::ImageButton* button,
                                       ButtonAlignment alignment) const;

  OpaqueBrowserFrameViewLayoutDelegate* delegate_;

  views::View* new_avatar_button_;

  // How far from the leading/trailing edge of the view the next window control
  // should be placed.
  int leading_button_start_;
  int trailing_button_start_;

  // The size of the window buttons, and the avatar menu item (if any). This
  // does not count labels or other elements that should be counted in a
  // minimal frame.
  int minimum_size_for_buttons_;

 private:
  // Determines whether the incognito icon should be shown on the right side of
  // the tab strip (instead of the usual left).
  bool ShouldIncognitoIconBeOnRight() const;

  // Determines the amount of spacing between the tabstrip and the caption
  // buttons.
  int TabStripCaptionSpacing() const;

  // Layout various sub-components of this view.
  void LayoutWindowControls(views::View* host);
  void LayoutTitleBar(views::View* host);
  void LayoutIncognitoIcon(views::View* host);

  void ConfigureButton(views::View* host,
                       views::FrameButton button_id,
                       ButtonAlignment align);

  // Sets the visibility of all buttons associated with |button_id| to false.
  void HideButton(views::FrameButton button_id);

  // Adds a window caption button to either the leading or trailing side.
  void SetBoundsForButton(views::FrameButton button_id,
                          views::View* host,
                          views::ImageButton* button,
                          ButtonAlignment align);

  // Internal implementation of ViewAdded() and ViewRemoved().
  void SetView(int id, views::View* view);

  // Overriden from views::LayoutManager:
  void Layout(views::View* host) override;
  gfx::Size GetPreferredSize(const views::View* host) const override;
  void ViewAdded(views::View* host, views::View* view) override;
  void ViewRemoved(views::View* host, views::View* view) override;

  // The bounds of the ClientView.
  gfx::Rect client_view_bounds_;

  // The layout of the window icon, if visible.
  gfx::Rect window_icon_bounds_;

  // Whether any of the window control buttons were packed on the
  // leading or trailing sides.
  bool has_leading_buttons_;
  bool has_trailing_buttons_;

  // Extra offset from the top of the frame to the top of the window control
  // buttons. Configurable based on platform and whether we are under test.
  int extra_caption_y_;

  // Extra offset between the individual window caption buttons.  Set only in
  // testing, otherwise, its value will be -1.
  int forced_window_caption_spacing_;

  // Window controls.
  views::ImageButton* minimize_button_;
  views::ImageButton* maximize_button_;
  views::ImageButton* restore_button_;
  views::ImageButton* close_button_;

  views::View* window_icon_;
  views::Label* window_title_;

  views::View* incognito_icon_;

  std::vector<views::FrameButton> leading_buttons_;
  std::vector<views::FrameButton> trailing_buttons_;

  DISALLOW_COPY_AND_ASSIGN(OpaqueBrowserFrameViewLayout);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_OPAQUE_BROWSER_FRAME_VIEW_LAYOUT_H_
