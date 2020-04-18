// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_LAYOUT_LAYOUT_MANAGER_H_
#define UI_VIEWS_LAYOUT_LAYOUT_MANAGER_H_

#include "ui/views/views_export.h"

namespace gfx {
class Size;
}

namespace views {

class View;

// LayoutManager is used by View to accomplish the following:
//
// . Provides preferred sizing information, see GetPreferredSize() and
//   GetPreferredHeightForWidth().
// . To position and size (aka layout) the children of the associated View. See
//   Layout() for details.
//
// How a LayoutManager operates is specific to the LayoutManager. Non-trivial
// LayoutManagers calculate preferred size and layout information using the
// minimum and preferred size of the children of the View. That is, they
// make use of View::GetMinimumSize(), View::CalculatePreferredSize() and/or
// View::GetHeightForWidth().
class VIEWS_EXPORT LayoutManager {
 public:
  virtual ~LayoutManager();

  // Notification that this LayoutManager has been installed on |host|.
  virtual void Installed(View* host);

  // Called by View::Layout() to position and size the children of |host|.
  // Generally this queries |host| for its size and positions and sizes the
  // children in a LayoutManager specific way.
  virtual void Layout(View* host) = 0;

  // Return the preferred size, which is typically the size needed to give each
  // child of |host| its preferred size. Generally this is calculated using the
  // View::CalculatePreferredSize() on each of the children of |host|.
  virtual gfx::Size GetPreferredSize(const View* host) const = 0;

  // Return the preferred height for a particular width. Generally this is
  // calculated using View::GetHeightForWidth() or
  // View::CalculatePreferredSize() on each of the children of |host|. Override
  // this function if the preferred height varies based on the size. For
  // example, a multi-line labels preferred height may change with the width.
  // The default implementation returns GetPreferredSize().height().
  virtual int GetPreferredHeightForWidth(const View* host, int width) const;

  // Called when a View is added as a child of the View the LayoutManager has
  // been installed on.
  virtual void ViewAdded(View* host, View* view);

  // Called when a View is removed as a child of the View the LayoutManager has
  // been installed on. This function allows the LayoutManager to cleanup any
  // state it has kept specific to a View.
  virtual void ViewRemoved(View* host, View* view);
};

}  // namespace views

#endif  // UI_VIEWS_LAYOUT_LAYOUT_MANAGER_H_
