// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AVATAR_BUTTON_ERROR_CONTROLLER_DELEGATE_H_
#define CHROME_BROWSER_UI_AVATAR_BUTTON_ERROR_CONTROLLER_DELEGATE_H_

// Delegate that allows AvatarButtonErrorController to communicate to
// AvatarButton.
class AvatarButtonErrorControllerDelegate {
 public:
  // Called when the signin/sync errors that should be exposed in the avatar
  // button change.
  virtual void OnAvatarErrorChanged() = 0;

 protected:
  virtual ~AvatarButtonErrorControllerDelegate() {}
};

#endif  // CHROME_BROWSER_UI_AVATAR_BUTTON_ERROR_CONTROLLER_DELEGATE_H_
