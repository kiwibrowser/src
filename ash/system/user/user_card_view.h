// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_USER_USER_CARD_VIEW_H_
#define ASH_SYSTEM_USER_USER_CARD_VIEW_H_

#include "ash/media_controller.h"
#include "base/macros.h"
#include "ui/views/view.h"

namespace views {
class BoxLayout;
class ImageView;
class Label;
}

namespace ash {

namespace tray {

// The view displaying information about the user, such as user's avatar, email
// address, name, and more. View has no borders. For the active user, this is
// nested inside a UserView. For other users, it's inside the dropdown.
class UserCardView : public views::View, public MediaCaptureObserver {
 public:
  explicit UserCardView(int user_index);
  ~UserCardView() override;

  // Called to suppress the appearance of the media capture icon. The active
  // user will show the capture state of all users, unless the user dropdown is
  // open, in which case the individual user with capture will show an icon.
  void SetSuppressCaptureIcon(bool suppressed);

  // View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // MediaCaptureObserver:
  void OnMediaCaptureChanged(
      const std::vector<mojom::MediaCaptureState>& capture_states) override;

 private:
  // Creates the content for the public mode.
  void AddPublicModeUserContent();

  void AddUserContent(views::BoxLayout* layout);

  bool is_active_user() const { return !user_index_; }

  const int user_index_;

  views::Label* user_name_ = nullptr;
  views::ImageView* media_capture_icon_ = nullptr;

  // This view wraps |media_capture_icon_| so that we can control showing it
  // orthogonally to the capture state.
  views::View* media_capture_container_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(UserCardView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_USER_USER_CARD_VIEW_H_
