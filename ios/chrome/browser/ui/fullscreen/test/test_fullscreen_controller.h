// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_CONTROLLER_H_

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

class FullscreenModel;

// Test version of FullscreenController that enables and disabled the
// FullscreenModel passed on initialization.
class TestFullscreenController : public FullscreenController {
 public:
  explicit TestFullscreenController(FullscreenModel* model);
  ~TestFullscreenController() override;

  // FullscreenController:
  ChromeBroadcaster* broadcaster() override;
  void SetWebStateList(WebStateList* web_state_list) override;
  void AddObserver(FullscreenControllerObserver* observer) override;
  void RemoveObserver(FullscreenControllerObserver* observer) override;
  bool IsEnabled() const override;
  void IncrementDisabledCounter() override;
  void DecrementDisabledCounter() override;
  CGFloat GetProgress() const override;
  void ResetModel() override;

 private:
  // The model.
  FullscreenModel* model_;
};

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_CONTROLLER_H_
