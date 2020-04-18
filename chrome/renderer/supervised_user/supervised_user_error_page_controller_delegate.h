// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_SUPERVISED_USER_SUPERVISED_USER_ERROR_PAGE_CONTROLLER_DELEGATE_H_
#define CHROME_RENDERER_SUPERVISED_USER_SUPERVISED_USER_ERROR_PAGE_CONTROLLER_DELEGATE_H_

#include "base/callback_forward.h"

class SupervisedUserErrorPageControllerDelegate {
 public:
  // Called when the interstitial calls the installed JS methods.
  virtual void GoBack() = 0;
  virtual void RequestPermission(base::OnceCallback<void(bool)> callback) = 0;
  virtual void Feedback() = 0;

 protected:
  virtual ~SupervisedUserErrorPageControllerDelegate() {}
};

#endif  // CHROME_RENDERER_SUPERVISED_USER_SUPERVISED_USER_ERROR_PAGE_CONTROLLER_DELEGATE_H_
