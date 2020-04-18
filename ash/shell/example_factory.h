// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_EXAMPLE_FACTORY_H_
#define ASH_SHELL_EXAMPLE_FACTORY_H_

namespace app_list {
class AppListViewDelegate;
}

namespace views {
class View;
}

namespace ash {
namespace shell {

void CreatePointyBubble(views::View* anchor_view);

void CreateLockScreen();

// Creates a window showing samples of commonly used widgets.
void CreateWidgetsWindow();

app_list::AppListViewDelegate* CreateAppListViewDelegate();

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_EXAMPLE_FACTORY_H_
