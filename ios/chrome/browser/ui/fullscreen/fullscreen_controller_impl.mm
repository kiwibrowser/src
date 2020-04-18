// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_impl.h"

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcast_observer_bridge.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_mediator.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_system_notification_observer.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_list_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenControllerImpl::FullscreenControllerImpl()
    : FullscreenController(),
      broadcaster_([[ChromeBroadcaster alloc] init]),
      model_(std::make_unique<FullscreenModel>()),
      mediator_(std::make_unique<FullscreenMediator>(this, model_.get())),
      bridge_(
          [[ChromeBroadcastOberverBridge alloc] initWithObserver:model_.get()]),
      notification_observer_([[FullscreenSystemNotificationObserver alloc]
          initWithController:this
                    mediator:mediator_.get()]) {
  DCHECK(broadcaster_);
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastScrollViewSize:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastScrollViewContentSize:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastScrollViewContentInset:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastContentScrollOffset:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastScrollViewIsScrolling:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastScrollViewIsZooming:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastScrollViewIsDragging:)];
  [broadcaster_ addObserver:bridge_
                forSelector:@selector(broadcastToolbarHeight:)];
}

FullscreenControllerImpl::~FullscreenControllerImpl() = default;

ChromeBroadcaster* FullscreenControllerImpl::broadcaster() {
  return broadcaster_;
}

void FullscreenControllerImpl::SetWebStateList(WebStateList* web_state_list) {
  if (web_state_list_observer_)
    web_state_list_observer_->Disconnect();
  web_state_list_ = web_state_list;
  web_state_list_observer_ =
      web_state_list_
          ? std::make_unique<FullscreenWebStateListObserver>(
                this, model_.get(), web_state_list_, mediator_.get())
          : nullptr;
}

void FullscreenControllerImpl::AddObserver(
    FullscreenControllerObserver* observer) {
  mediator_->AddObserver(observer);
}

void FullscreenControllerImpl::RemoveObserver(
    FullscreenControllerObserver* observer) {
  mediator_->RemoveObserver(observer);
}

bool FullscreenControllerImpl::IsEnabled() const {
  return model_->enabled();
}

void FullscreenControllerImpl::IncrementDisabledCounter() {
  model_->IncrementDisabledCounter();
}

void FullscreenControllerImpl::DecrementDisabledCounter() {
  model_->DecrementDisabledCounter();
}

CGFloat FullscreenControllerImpl::GetProgress() const {
  return model_->progress();
}

void FullscreenControllerImpl::ResetModel() {
  mediator_->AnimateModelReset();
}

void FullscreenControllerImpl::Shutdown() {
  mediator_->Disconnect();
  [notification_observer_ disconnect];
  if (web_state_list_observer_)
    web_state_list_observer_->Disconnect();
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastScrollViewSize:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastScrollViewContentSize:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastScrollViewContentInset:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastContentScrollOffset:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastScrollViewIsScrolling:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastScrollViewIsZooming:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastScrollViewIsDragging:)];
  [broadcaster_ removeObserver:bridge_
                   forSelector:@selector(broadcastToolbarHeight:)];
}
