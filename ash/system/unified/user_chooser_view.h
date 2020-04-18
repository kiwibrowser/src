// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_USER_CHOOSER_VIEW_H_
#define ASH_SYSTEM_UNIFIED_USER_CHOOSER_VIEW_H_

#include "ui/views/view.h"

namespace ash {

class UnifiedSystemTrayController;

// Circular image view with user's icon of |user_index|.
views::View* CreateUserAvatarView(int user_index);

// A detailed view of user chooser.
class UserChooserView : public views::View {
 public:
  UserChooserView(UnifiedSystemTrayController* controller);
  ~UserChooserView() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UserChooserView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_VIEW_H_
