// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/test/test_overlay_queue_manager_observer.h"

#include <algorithm>
#include <iterator>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TestOverlayQueueManagerObserver::TestOverlayQueueManagerObserver()
    : did_add_called_(false), will_remove_called_(false) {}

TestOverlayQueueManagerObserver::~TestOverlayQueueManagerObserver() {}

void TestOverlayQueueManagerObserver::OverlayQueueManagerDidAddQueue(
    OverlayQueueManager* manager,
    OverlayQueue* queue) {
  did_add_called_ = true;
  queues_.push_back(queue);
}

void TestOverlayQueueManagerObserver::OverlayQueueManagerWillRemoveQueue(
    OverlayQueueManager* manager,
    OverlayQueue* queue) {
  will_remove_called_ = true;
  auto iter = std::find(std::begin(queues_), std::end(queues_), queue);
  if (iter != queues_.end())
    queues_.erase(iter);
}
