// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_APP_INFO_DIALOG_CONTAINER_H_
#define CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_APP_INFO_DIALOG_CONTAINER_H_

#include "base/callback_forward.h"
#include "chrome/common/buildflags.h"
#include "ui/gfx/geometry/size.h"

namespace views {
class DialogDelegateView;
class View;
}

#if BUILDFLAG(ENABLE_APP_LIST)

// Creates a new dialog containing |view| that can be displayed inside the app
// list, covering the entire app list and adding a close button. Takes ownership
// of |view|.
views::DialogDelegateView* CreateAppListContainerForView(
    views::View* view,
    const base::Closure& close_callback);

#endif  // ENABLE_APP_LIST

// Creates a new native dialog of the given |size| containing |view| with a
// close button and draggable titlebar. Takes ownership of |view|.
views::DialogDelegateView* CreateDialogContainerForView(
    views::View* view,
    const gfx::Size& size,
    const base::Closure& close_callback);

#endif  // CHROME_BROWSER_UI_VIEWS_APPS_APP_INFO_DIALOG_APP_INFO_DIALOG_CONTAINER_H_
