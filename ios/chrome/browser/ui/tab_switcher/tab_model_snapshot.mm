// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_model_snapshot.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TabModelSnapshot::TabModelSnapshot(TabModel* tab_model) {
  if (tab_model) {
    DCHECK(tab_model.webStateList);
    WebStateList* web_state_list = tab_model.webStateList;
    for (int index = 0; index < web_state_list->count(); ++index) {
      web::WebState* web_state = web_state_list->GetWebStateAt(index);
      Tab* tab = LegacyTabHelper::GetTabForWebState(web_state);
      hashes_.push_back(HashOfTheVisiblePropertiesOfATab(tab));
      tabs_.push_back(tab);
    }
  }
}

TabModelSnapshot::~TabModelSnapshot() {}

// static
size_t TabModelSnapshot::HashOfTheVisiblePropertiesOfATab(Tab* tab) {
  DCHECK(tab);
  std::stringstream ss;
  // lastVisitedTimestamp is used as an approximation for whether the tab's
  // snapshot changed.
  ss << base::SysNSStringToUTF8(tab.tabId) << std::endl
     << base::SysNSStringToUTF8(tab.urlDisplayString) << std::endl
     << std::hexfloat << tab.lastVisitedTimestamp << std::endl;
  return std::hash<std::string>()(ss.str());
}
