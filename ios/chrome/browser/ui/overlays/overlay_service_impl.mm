// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/overlay_service_impl.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/chrome/browser/ui/overlays/browser_overlay_queue.h"
#import "ios/chrome/browser/ui/overlays/overlay_queue_manager.h"
#import "ios/chrome/browser/ui/overlays/overlay_scheduler.h"
#import "ios/chrome/browser/ui/overlays/overlay_service_observer.h"
#import "ios/chrome/browser/ui/overlays/web_state_overlay_queue.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OverlayServiceImpl::OverlayServiceImpl(BrowserList* browser_list)
    : browser_list_(browser_list) {
  DCHECK(browser_list_);
  for (int index = 0; index < browser_list_->GetCount(); ++index) {
    StartServiceForBrowser(browser_list_->GetBrowserAtIndex(index));
  }
  browser_list_->AddObserver(this);
}

OverlayServiceImpl::~OverlayServiceImpl() {}

#pragma mark - BrowserListObserver

void OverlayServiceImpl::OnBrowserCreated(BrowserList* browser_list,
                                          Browser* browser) {
  DCHECK_EQ(browser_list, browser_list_);
  StartServiceForBrowser(browser);
}

void OverlayServiceImpl::OnBrowserRemoved(BrowserList* browser_list,
                                          Browser* browser) {
  DCHECK_EQ(browser_list, browser_list_);
  StopServiceForBrowser(browser);
}

#pragma mark - KeyedService

void OverlayServiceImpl::Shutdown() {
  for (int index = 0; index < browser_list_->GetCount(); ++index) {
    StopServiceForBrowser(browser_list_->GetBrowserAtIndex(index));
  }
  browser_list_->RemoveObserver(this);
  browser_list_ = nullptr;
}

#pragma mark - OverlaySchedulerObserver

void OverlayServiceImpl::OverlaySchedulerWillShowOverlay(
    OverlayScheduler* scheduler,
    web::WebState* web_state) {
  DCHECK(scheduler);
  Browser* scheduler_browser = nullptr;
  for (int index = 0; index < browser_list_->GetCount(); ++index) {
    Browser* browser = browser_list_->GetBrowserAtIndex(index);
    if (OverlayScheduler::FromBrowser(browser) == scheduler) {
      scheduler_browser = browser;
      break;
    }
  }
  DCHECK(scheduler_browser);
  for (auto& observer : observers_) {
    observer.OverlayServiceWillShowOverlay(this, web_state, scheduler_browser);
  }
}

#pragma mark - OverlayService

void OverlayServiceImpl::AddObserver(OverlayServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void OverlayServiceImpl::RemoveObserver(OverlayServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void OverlayServiceImpl::PauseServiceForBrowser(Browser* browser) {
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  if (scheduler)
    scheduler->SetPaused(true);
}

void OverlayServiceImpl::ResumeServiceForBrowser(Browser* browser) {
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  if (scheduler)
    scheduler->SetPaused(false);
}

bool OverlayServiceImpl::IsPausedForBrowser(Browser* browser) const {
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  return scheduler && scheduler->paused();
}

bool OverlayServiceImpl::IsBrowserShowingOverlay(Browser* browser) const {
  if (browser_list_->GetIndexOfBrowser(browser) == BrowserList::kInvalidIndex)
    return false;
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  return scheduler && scheduler->IsShowingOverlay();
}

void OverlayServiceImpl::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator,
    Browser* browser) {
  DCHECK(overlay_coordinator);
  DCHECK(IsBrowserShowingOverlay(browser));
  OverlayScheduler::FromBrowser(browser)->ReplaceVisibleOverlay(
      overlay_coordinator);
}

void OverlayServiceImpl::CancelOverlays() {
  for (int index = 0; index < browser_list_->GetCount(); ++index) {
    OverlayScheduler::FromBrowser(browser_list_->GetBrowserAtIndex(index))
        ->CancelOverlays();
  }
}

void OverlayServiceImpl::ShowOverlayForBrowser(
    OverlayCoordinator* overlay_coordinator,
    BrowserCoordinator* parent_coordiantor,
    Browser* browser) {
  BrowserOverlayQueue* queue = BrowserOverlayQueue::FromBrowser(browser);
  if (queue)
    queue->AddBrowserOverlay(overlay_coordinator, parent_coordiantor);
}

void OverlayServiceImpl::CancelAllOverlaysForBrowser(Browser* browser) {
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  if (scheduler)
    scheduler->CancelOverlays();
}

void OverlayServiceImpl::ShowOverlayForWebState(
    OverlayCoordinator* overlay_coordinator,
    web::WebState* web_state) {
  WebStateOverlayQueue* queue = WebStateOverlayQueue::FromWebState(web_state);
  if (queue)
    queue->AddWebStateOverlay(overlay_coordinator);
}

void OverlayServiceImpl::SetWebStateParentCoordinator(
    BrowserCoordinator* parent_coordinator,
    web::WebState* web_state) {
  WebStateOverlayQueue* queue = WebStateOverlayQueue::FromWebState(web_state);
  if (queue)
    queue->SetWebStateParentCoordinator(parent_coordinator);
}

void OverlayServiceImpl::CancelAllOverlaysForWebState(
    web::WebState* web_state) {
  WebStateOverlayQueue* queue = WebStateOverlayQueue::FromWebState(web_state);
  if (queue)
    queue->CancelOverlays();
}

#pragma mark -

void OverlayServiceImpl::StartServiceForBrowser(Browser* browser) {
  OverlayScheduler::CreateForBrowser(browser);
  OverlayScheduler::FromBrowser(browser)->AddObserver(this);
}

void OverlayServiceImpl::StopServiceForBrowser(Browser* browser) {
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  scheduler->Disconnect();
  scheduler->RemoveObserver(this);
}
