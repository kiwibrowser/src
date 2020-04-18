// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTROLLER_H_

#include "base/callback_forward.h"
#include "chrome/browser/ui/views/tabs/tab_strip_types.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ui_base_types.h"

class Tab;

namespace gfx {
class Path;
class Point;
class Rect;
}
namespace ui {
class ListSelectionModel;
class LocatedEvent;
class MouseEvent;
}
namespace views {
class View;
}

// Controller for tabs.
class TabController {
 public:
  // Used in GetAdjacentTab to indicate which adjacent tab to retrieve. FORWARD
  // will return the adjacent tab to the right. BACKWARD will return the
  // adjacent tab to the left of the given tab.
  enum Direction { FORWARD, BACKWARD };

  virtual const ui::ListSelectionModel& GetSelectionModel() const = 0;

  // Returns true if multiple selection is supported.
  virtual bool SupportsMultipleSelection() = 0;

  // Returns true if the close buttons of the inactive tabs are forced to be
  // hidden.
  virtual bool ShouldHideCloseButtonForInactiveTabs() = 0;

  // Returns true if the close button on an inactive tab should be shown on
  // mouse hover. This is predicated on ShouldHideCloseButtonForInactiveTabs()
  // returning true.
  virtual bool ShouldShowCloseButtonOnHover() = 0;

  // Returns true if ShouldPaintTab() could return a non-empty clip path.
  virtual bool MaySetClip() = 0;

  // Selects the tab.
  virtual void SelectTab(Tab* tab) = 0;

  // Extends the selection from the anchor to |tab|.
  virtual void ExtendSelectionTo(Tab* tab) = 0;

  // Toggles whether |tab| is selected.
  virtual void ToggleSelected(Tab* tab) = 0;

  // Adds the selection from the anchor to |tab|.
  virtual void AddSelectionFromAnchorTo(Tab* tab) = 0;

  // Closes the tab.
  virtual void CloseTab(Tab* tab, CloseTabSource source) = 0;

  // Toggles whether tab-wide audio muting is active.
  virtual void ToggleTabAudioMute(Tab* tab) = 0;

  // Shows a context menu for the tab at the specified point in screen coords.
  virtual void ShowContextMenuForTab(Tab* tab,
                                     const gfx::Point& p,
                                     ui::MenuSourceType source_type) = 0;

  // Returns true if |tab| is the active tab. The active tab is the one whose
  // content is shown in the browser.
  virtual bool IsActiveTab(const Tab* tab) const = 0;

  // Returns true if the specified Tab is selected.
  virtual bool IsTabSelected(const Tab* tab) const = 0;

  // Returns true if the specified Tab is pinned.
  virtual bool IsTabPinned(const Tab* tab) const = 0;

  // Returns true if the tab is a part of an incognito profile.
  virtual bool IsIncognito() const = 0;

  // Potentially starts a drag for the specified Tab.
  virtual void MaybeStartDrag(
      Tab* tab,
      const ui::LocatedEvent& event,
      const ui::ListSelectionModel& original_selection) = 0;

  // Continues dragging a Tab.
  virtual void ContinueDrag(views::View* view,
                            const ui::LocatedEvent& event) = 0;

  // Ends dragging a Tab. Returns whether the tab has been destroyed.
  virtual bool EndDrag(EndDragReason reason) = 0;

  // Returns the tab that contains the specified coordinates, in terms of |tab|,
  // or NULL if there is no tab that contains the specified point.
  virtual Tab* GetTabAt(Tab* tab,
                        const gfx::Point& tab_in_tab_coordinates) = 0;

  // Returns the next/previous tab in the model order. Returns nullptr if there
  // isn't an adjacent tab in the given direction.
  virtual Tab* GetAdjacentTab(Tab* tab, Direction direction) = 0;

  // Invoked when a mouse event occurs on |source|.
  virtual void OnMouseEventInTab(views::View* source,
                                 const ui::MouseEvent& event) = 0;

  // Returns whether |tab| needs to be painted. When this returns true, |clip|
  // is set to the path which should be clipped out of the current tab's region
  // (for hit testing or painting), if any.  |clip| is only non-empty when
  // stacking tabs; if it is empty, no clipping is needed.  |border_callback| is
  // a callback which returns a tab's border given its size, and is used in
  // computing |clip|.
  virtual bool ShouldPaintTab(
      const Tab* tab,
      const base::RepeatingCallback<gfx::Path(const gfx::Rect&)>&
          border_callback,
      gfx::Path* clip) = 0;

  // Returns true if tab loading throbbers can be painted to a composited layer.
  // This can only be done when the TabController can guarantee that nothing
  // in the same window will redraw on top of the the favicon area of any tab.
  virtual bool CanPaintThrobberToLayer() const = 0;

  // Returns COLOR_TOOLBAR_TOP_SEPARATOR[,_INACTIVE] depending on the activation
  // state of the window.
  virtual SkColor GetToolbarTopSeparatorColor() const = 0;

  // Returns the resource ID for the image to use as the tab background.
  // |custom_image| is an outparam set to true if either the tab or the frame
  // background images have been customized; see implementation comments.
  virtual int GetBackgroundResourceId(bool* custom_image) const = 0;

  // Returns the accessible tab name for this tab.
  virtual base::string16 GetAccessibleTabName(const Tab* tab) const = 0;

 protected:
  virtual ~TabController() {}
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_CONTROLLER_H_
