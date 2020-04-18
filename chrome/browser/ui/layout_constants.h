// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_
#define CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_

#include "build/build_config.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"

enum LayoutConstant {
  // The minimum height of Bookmarks Bar, when attached to the toolbar. The
  // height of the toolbar may grow to more than this value if the embedded
  // views need more space, for example, when the font is larger than normal.
  BOOKMARK_BAR_HEIGHT,

#if defined(OS_MACOSX)
  // This is a little smaller than the bookmarkbar height because of the visual
  // overlap with the main toolbar. This height should not be used when
  // computing the height of the toolbar.
  BOOKMARK_BAR_HEIGHT_NO_OVERLAP,
#endif

  // The height of Bookmarks Bar, when visible in "New Tab Page" mode.
  BOOKMARK_BAR_NTP_HEIGHT,

#if defined(OS_MACOSX)
  // The amount of space between the inner bookmark bar and the outer toolbar on
  // new tab pages.
  BOOKMARK_BAR_NTP_PADDING,
#endif

  // The size of the app menu button in a hosted app browser window.
  HOSTED_APP_MENU_BUTTON_SIZE,

  // The vertical padding (additional to TOOLBAR_ELEMENT_PADDING) above and
  // below location bar bubbles.
  LOCATION_BAR_BUBBLE_VERTICAL_PADDING,

  // The vertical padding between the edge of a location bar bubble and its
  // contained text.
  LOCATION_BAR_BUBBLE_FONT_VERTICAL_PADDING,

  // The corner radius used for the location bar bubble.
  LOCATION_BAR_BUBBLE_CORNER_RADIUS,

  // The vertical inset to apply to the bounds of a location bar bubble's anchor
  // view, to bring the bubble closer to the anchor.  This compensates for the
  // space between the bottoms of most such views and the visible bottoms of the
  // images inside.
  LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET,

  // The horizontal padding between location bar decorations as well as the
  // vertical and horizontal padding inside the border.
  LOCATION_BAR_ELEMENT_PADDING,

  // The height to be occupied by the LocationBar.
  LOCATION_BAR_HEIGHT,

  // The size of the icons used inside the LocationBar.
  LOCATION_BAR_ICON_SIZE,

  // The amount of padding used around the icon inside the LocationBar, i.e. the
  // full width of a LocationBar icon will be LOCATION_BAR_ICON_SIZE + 2 *
  // LOCATION_BAR_ICON_INTERIOR_PADDING. Icons may additionally be spaced
  // horizontally by LOCATION_BAR_ELEMENT_PADDING, but this region is not part
  // of the icon view (e.g. does not highlight on hover).
  LOCATION_BAR_ICON_INTERIOR_PADDING,

  // Padding after the tab title.
  TAB_AFTER_TITLE_PADDING,

  // Width of the alert indicator shown for a tab using media capture.
  TAB_ALERT_INDICATOR_CAPTURE_ICON_WIDTH,

  // Width of the alert indicator icon displayed in the tab. The same width is
  // used for all 3 states of normal, hovered and pressed.
  TAB_ALERT_INDICATOR_ICON_WIDTH,

  // The height of a tab, including outer strokes.  In non-100% scales this is
  // slightly larger than the apparent height of the tab, as the top stroke is
  // drawn as a 1-px line flush with the bottom of the tab's topmost DIP.
  TAB_HEIGHT,

  // Padding before the tab title.
  TAB_PRE_TITLE_PADDING,

  // The distance between the edge of one tab to the corresponding edge or the
  // subsequent tab when tabs are stacked.
  TAB_STACK_DISTANCE,

  // The standard width of a tab when is stacked layout. This does not include
  // the endcap width.
  TAB_STACK_TAB_WIDTH,

  // The standard tab width excluding the overlap (which is the endcap width on
  // one side)
  TAB_STANDARD_WIDTH,

  // Additional horizontal padding between the elements in the toolbar.
  TOOLBAR_ELEMENT_PADDING,

  // The horizontal space between most items in the toolbar.
  TOOLBAR_STANDARD_SPACING,
};

enum LayoutInset {
  // The padding inside the tab bounds that defines the tab contents region.
  TAB,

  // The padding inside the border of a toolbar button (around the image).
  TOOLBAR_BUTTON,

  // The padding inside the border of a toolbar action view button.
  TOOLBAR_ACTION_VIEW,
};

enum LayoutSize {
  // The visible size of the new tab button; does not include any Fitts' Law
  // extensions. Note that in touch-optimized UI mode, the new tab button's
  // width is larger when the browser is in incognito mode. The height remains
  // the same whether incognito or not.
  NEW_TAB_BUTTON,
};

int GetLayoutConstant(LayoutConstant constant);
#if defined(OS_MACOSX)
// Use this function instead of GetLayoutConstant() for Cocoa browser.
// This will handle Cocoa specific layout constants. For non Cocoa specific
// constants, it will call GetLayoutConstant() anyway.
int GetCocoaLayoutConstant(LayoutConstant constant);
#endif

gfx::Insets GetLayoutInsets(LayoutInset inset);
gfx::Size GetLayoutSize(LayoutSize size, bool is_incognito);

#endif  // CHROME_BROWSER_UI_LAYOUT_CONSTANTS_H_
