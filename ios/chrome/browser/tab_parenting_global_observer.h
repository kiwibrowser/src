// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TAB_PARENTING_GLOBAL_OBSERVER_H_
#define IOS_CHROME_BROWSER_TAB_PARENTING_GLOBAL_OBSERVER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/macros.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace web {
class WebState;
}

// Allows clients to observe every tab (i.e., WebState) that is parented.
// NOTE: Should be used only to correspond to //chrome flows that listen for
// the TAB_PARENTED notification from all sources.
class TabParentingGlobalObserver {
 public:
  typedef base::Callback<void(web::WebState*)> OnTabParentedCallback;

  // Returns the instance of TabParentingGlobalObserver.
  static TabParentingGlobalObserver* GetInstance();

  // Registers |cb| to be invoked when a tab is parented.
  std::unique_ptr<base::CallbackList<void(web::WebState*)>::Subscription>
  RegisterCallback(const OnTabParentedCallback& cb);

  // Called to notify all registered callbacks that |web_state| was parented.
  void OnTabParented(web::WebState* web_state);

 private:
  friend struct base::DefaultSingletonTraits<TabParentingGlobalObserver>;

  TabParentingGlobalObserver();
  ~TabParentingGlobalObserver();

  base::CallbackList<void(web::WebState*)> on_tab_parented_callback_list_;

  DISALLOW_COPY_AND_ASSIGN(TabParentingGlobalObserver);
};

#endif  // IOS_CHROME_BROWSER_TAB_PARENTING_GLOBAL_OBSERVER_H_
