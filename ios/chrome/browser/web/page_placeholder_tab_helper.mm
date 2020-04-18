// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/page_placeholder_tab_helper.h"

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/web/public/web_state/ui/crw_web_view_proxy.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Placeholder will not be displayed longer than this time.
const double kPlaceholderMaxDisplayTimeInSeconds = 1.5;

// Placeholder removal will include a fade-out animation of this length.
const NSTimeInterval kPlaceholderFadeOutAnimationLengthInSeconds = 0.5;
}  // namespace

DEFINE_WEB_STATE_USER_DATA_KEY(PagePlaceholderTabHelper);

PagePlaceholderTabHelper::PagePlaceholderTabHelper(web::WebState* web_state)
    : web_state_(web_state), weak_factory_(this) {
  web_state_->AddObserver(this);
}

PagePlaceholderTabHelper::~PagePlaceholderTabHelper() {
  RemovePlaceholder();
}

void PagePlaceholderTabHelper::AddPlaceholderForNextNavigation() {
  add_placeholder_for_next_navigation_ = true;
}

void PagePlaceholderTabHelper::CancelPlaceholderForNextNavigation() {
  add_placeholder_for_next_navigation_ = false;
  if (displaying_placeholder_) {
    RemovePlaceholder();
  }
}

void PagePlaceholderTabHelper::DidStartNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  DCHECK_EQ(web_state_, web_state);
  if (add_placeholder_for_next_navigation_) {
    add_placeholder_for_next_navigation_ = false;
    AddPlaceholder();
  }
}

void PagePlaceholderTabHelper::PageLoaded(web::WebState* web_state,
                                          web::PageLoadCompletionStatus) {
  DCHECK_EQ(web_state_, web_state);
  RemovePlaceholder();
}

void PagePlaceholderTabHelper::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
  RemovePlaceholder();
}

void PagePlaceholderTabHelper::AddPlaceholder() {
  if (displaying_placeholder_)
    return;

  displaying_placeholder_ = true;

  // Lazily create the placeholder view.
  if (!placeholder_view_) {
    placeholder_view_ = [[UIImageView alloc] init];
    placeholder_view_.contentMode = UIViewContentModeScaleAspectFit;
    placeholder_view_.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }

  // Update placeholder view's image and display it on top of WebState's view.
  __weak UIImageView* weak_placeholder_view = placeholder_view_;
  __weak UIView* weak_web_state_view = web_state_->GetView();
  __weak id<CRWWebViewProxy> web_view_proxy = web_state_->GetWebViewProxy();
  SnapshotTabHelper::FromWebState(web_state_)
      ->RetrieveGreySnapshot(^(UIImage* snapshot) {
        CGRect frame = weak_web_state_view.frame;
        UIEdgeInsets inset = web_view_proxy.contentInset;
        frame.origin.x += inset.left;
        frame.origin.y += inset.top;
        frame.size.width -= (inset.right + inset.left);
        frame.size.height -= (inset.bottom + inset.top);
        weak_placeholder_view.frame = frame;
        weak_placeholder_view.image = snapshot;
        [weak_web_state_view addSubview:weak_placeholder_view];
      });

  // Remove placeholder if it takes too long to load the page.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&PagePlaceholderTabHelper::RemovePlaceholder,
                 weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSecondsD(kPlaceholderMaxDisplayTimeInSeconds));
}

void PagePlaceholderTabHelper::RemovePlaceholder() {
  if (!displaying_placeholder_)
    return;

  displaying_placeholder_ = false;

  // Remove placeholder view with a fade-out animation.
  __weak UIImageView* weak_placeholder_view = placeholder_view_;
  [UIView animateWithDuration:kPlaceholderFadeOutAnimationLengthInSeconds
      animations:^{
        weak_placeholder_view.alpha = 0.0f;
      }
      completion:^(BOOL finished) {
        [weak_placeholder_view removeFromSuperview];
      }];
}
