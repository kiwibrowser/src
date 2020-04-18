// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_IMPL_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_IMPL_H_

#include <memory>

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

@class ChromeBroadcastOberverBridge;
class FullscreenMediator;
class FullscreenModel;
class FullscreenWebStateListObserver;
@class FullscreenSystemNotificationObserver;

// Implementation of FullscreenController.
class FullscreenControllerImpl : public FullscreenController {
 public:
  explicit FullscreenControllerImpl();
  ~FullscreenControllerImpl() override;

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
  // KeyedService:
  void Shutdown() override;

  // The broadcaster that drives the model.
  __strong ChromeBroadcaster* broadcaster_ = nil;
  // The WebStateList for the Browser whose fullscreen is managed by this
  // object.
  WebStateList* web_state_list_ = nullptr;
  // The model used to calculate fullscreen state.
  std::unique_ptr<FullscreenModel> model_;
  // Object that manages sending signals to FullscreenControllerImplObservers.
  std::unique_ptr<FullscreenMediator> mediator_;
  // The bridge used to forward brodcasted UI to |model_|.
  __strong ChromeBroadcastOberverBridge* bridge_ = nil;
  // A helper object that listens for system notifications.
  __strong FullscreenSystemNotificationObserver* notification_observer_ = nil;
  // A WebStateListObserver that updates |model_| for WebStateList changes.
  std::unique_ptr<FullscreenWebStateListObserver> web_state_list_observer_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenControllerImpl);
};

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_IMPL_H_
