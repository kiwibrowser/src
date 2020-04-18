// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_NON_CLIENT_FRAME_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_NON_CLIENT_FRAME_VIEW_H_

#include "base/scoped_observer.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/views/frame/avatar_button_manager.h"
#include "chrome/browser/ui/views/profiles/profile_indicator_icon.h"
#include "chrome/browser/ui/views/tabs/tab_strip_observer.h"
#include "ui/views/window/non_client_view.h"

class BrowserFrame;
class BrowserView;

// A specialization of the NonClientFrameView object that provides additional
// Browser-specific methods.
class BrowserNonClientFrameView : public views::NonClientFrameView,
                                  public ProfileAttributesStorage::Observer,
                                  public TabStripObserver {
 public:
  BrowserNonClientFrameView(BrowserFrame* frame, BrowserView* browser_view);
  ~BrowserNonClientFrameView() override;

  // Returns the padding on the left, right, and bottom of the avatar icon.
  static int GetAvatarIconPadding();

  BrowserView* browser_view() const { return browser_view_; }
  BrowserFrame* frame() const { return frame_; }

  const views::View* profile_indicator_icon() const {
    return profile_indicator_icon_;
  }
  views::View* profile_indicator_icon() { return profile_indicator_icon_; }

  // Called when BrowserView creates all it's child views.
  virtual void OnBrowserViewInitViewsComplete();

  // Called on Linux X11 after the browser window is maximized or restored.
  virtual void OnMaximizedStateChanged();

  // Called on Linux X11 after the browser window is fullscreened or
  // unfullscreened.
  virtual void OnFullscreenStateChanged();

  // Returns whether the caption buttons are drawn at the leading edge (i.e. the
  // left in LTR mode, or the right in RTL mode).
  virtual bool CaptionButtonsOnLeadingEdge() const;

  // Retrieves the bounds, in non-client view coordinates within which the
  // TabStrip should be laid out.
  virtual gfx::Rect GetBoundsForTabStrip(views::View* tabstrip) const = 0;

  // Returns the inset of the topmost view in the client view from the top of
  // the non-client view. The topmost view depends on the window type. The
  // topmost view is the tab strip for tabbed browser windows, the toolbar for
  // popups, the web contents for app windows and varies for fullscreen windows.
  // If |restored| is true, this is calculated as if the window was restored,
  // regardless of its current state.
  virtual int GetTopInset(bool restored) const = 0;

  // Returns the amount that the theme background should be inset.
  virtual int GetThemeBackgroundXInset() const = 0;

  // Retrieves the icon to use in the frame to indicate an incognito window.
  gfx::ImageSkia GetIncognitoAvatarIcon() const;

  // Returns COLOR_TOOLBAR_TOP_SEPARATOR[,_INACTIVE] depending on the activation
  // state of the window.
  SkColor GetToolbarTopSeparatorColor() const;

  // Updates the throbber.
  virtual void UpdateThrobber(bool running) = 0;

  // Returns the profile switcher button, if this frame has any, nullptr if it
  // doesn't.
  views::Button* GetProfileSwitcherButton() const;

  // Provided for mus. Updates the client-area of the WindowTreeHostMus.
  virtual void UpdateClientArea();

  // Provided for mus to update the minimum window size property.
  virtual void UpdateMinimumSize();

  // Distance between the leading edge of the NonClientFrameView and the tab
  // strip.
  // TODO: Consider refactoring and unifying tabstrip bounds calculations.
  // https://crbug.com/820485.
  virtual int GetTabStripLeftInset() const;

  // views::NonClientFrameView:
  void ChildPreferredSizeChanged(views::View* child) override;
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;

  // TabStripObserver:
  void OnTabAdded(int index) override;
  void OnTabRemoved(int index) override;

 protected:
  // Whether the frame should be painted with theming.
  // By default, tabbed browser windows are themed but popup and app windows are
  // not.
  virtual bool ShouldPaintAsThemed() const;

  // Whether the frame should be painted with a special mode for one tab.
  bool ShouldPaintAsSingleTabMode() const;

  // Compute aspects of the frame needed to paint the frame background.
  SkColor GetFrameColor(bool active) const;
  gfx::ImageSkia GetFrameImage(bool active) const;
  gfx::ImageSkia GetFrameOverlayImage(bool active) const;

  // Convenience versions of the above which use ShouldPaintAsActive() for
  // |active|.
  SkColor GetFrameColor() const;
  gfx::ImageSkia GetFrameImage() const;
  gfx::ImageSkia GetFrameOverlayImage() const;

  // Returns the style of the profile switcher avatar button.
  virtual AvatarButtonStyle GetAvatarButtonStyle() const = 0;

  // Updates all the profile icons as necessary (profile switcher button, or the
  // icon that indicates incognito (or a teleported window in ChromeOS)).
  void UpdateProfileIcons();

  void LayoutIncognitoButton();

  void PaintToolbarBackground(gfx::Canvas* canvas) const;

  // views::NonClientFrameView:
  void ActivationChanged(bool active) override;
  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override;

  AvatarButtonManager* profile_switcher() { return &profile_switcher_; }

 private:
  // views::NonClientFrameView:
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;

  // ProfileAttributesStorage::Observer:
  void OnProfileAdded(const base::FilePath& profile_path) override;
  void OnProfileWasRemoved(const base::FilePath& profile_path,
                           const base::string16& profile_name) override;
  void OnProfileAvatarChanged(const base::FilePath& profile_path) override;
  void OnProfileHighResAvatarLoaded(
      const base::FilePath& profile_path) override;

  // Gets a theme provider that should be non-null even before we're added to a
  // view hierarchy.
  const ui::ThemeProvider* GetThemeProviderForProfile() const;

  // Draws a taskbar icon if avatars are enabled, erases it otherwise.
  void UpdateTaskbarDecoration();

  // Returns true if |profile_indicator_icon_| should be shown.
  bool ShouldShowProfileIndicatorIcon() const;

  // The frame that hosts this view.
  BrowserFrame* frame_;

  // The BrowserView hosted within this View.
  BrowserView* browser_view_;

  // Wrapper around the in-frame profile switcher. Might not be used on all
  // platforms.
  AvatarButtonManager profile_switcher_;

  // On desktop, this is used to show an incognito icon. On CrOS, it's also used
  // for teleported windows (in multi-profile mode).
  ProfileIndicatorIcon* profile_indicator_icon_;

  ScopedObserver<TabStrip, BrowserNonClientFrameView> tab_strip_observer_;

  DISALLOW_COPY_AND_ASSIGN(BrowserNonClientFrameView);
};

namespace chrome {

// Provided by a browser_non_client_frame_view_factory_*.cc implementation
BrowserNonClientFrameView* CreateBrowserNonClientFrameView(
    BrowserFrame* frame, BrowserView* browser_view);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_NON_CLIENT_FRAME_VIEW_H_
