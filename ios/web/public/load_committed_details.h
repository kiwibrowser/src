// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_LOAD_COMMITTED_DETAILS_H_
#define IOS_WEB_PUBLIC_LOAD_COMMITTED_DETAILS_H_

#include "url/gurl.h"

namespace web {
class NavigationItem;

struct LoadCommittedDetails {
  // By default, the item will be filled according to a new main frame
  // navigation.
  LoadCommittedDetails();

  // The committed item. This will be the active item in the controller.
  web::NavigationItem* item;

  // The index of the previously committed navigation item. This will be -1
  // if there are no previous items.
  int previous_item_index;

  // True if the navigation was in-page. This means that the active item's
  // URL and the previous URL are the same except for reference fragments.
  bool is_in_page;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_LOAD_COMMITTED_DETAILS_H_
