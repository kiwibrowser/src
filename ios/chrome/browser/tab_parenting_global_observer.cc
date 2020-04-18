// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/tab_parenting_global_observer.h"

#include "base/memory/singleton.h"

TabParentingGlobalObserver* TabParentingGlobalObserver::GetInstance() {
  return base::Singleton<TabParentingGlobalObserver>::get();
}

std::unique_ptr<base::CallbackList<void(web::WebState*)>::Subscription>
TabParentingGlobalObserver::RegisterCallback(const OnTabParentedCallback& cb) {
  return on_tab_parented_callback_list_.Add(cb);
}

void TabParentingGlobalObserver::OnTabParented(web::WebState* web_state) {
  on_tab_parented_callback_list_.Notify(web_state);
}

TabParentingGlobalObserver::TabParentingGlobalObserver() {}

TabParentingGlobalObserver::~TabParentingGlobalObserver() {}
