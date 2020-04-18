// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_UTIL_H_
#define IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_UTIL_H_

// This file contains extensions for web::NavigationManager API without making
// them part of ios/web/public.

namespace web {

class NavigationItemImpl;
class NavigationManager;
class NavigationManagerImpl;

// Returns transient, committed or pending navigation item with given
// |unique_id| or null if item is not found. Item's unique id is retrieved via
// GetUniqueID method.
NavigationItemImpl* GetItemWithUniqueID(
    NavigationManagerImpl* navigation_manager,
    int unique_id);

// Returns committed navigation item with given |unique_id| or null if item
// is not found or it is pending or transient. Item's unique id is retrieved
// via GetUniqueID method.
NavigationItemImpl* GetCommittedItemWithUniqueID(
    NavigationManagerImpl* navigation_manager,
    int unique_id);

// Returns committed navigation item index with given |unique_id| or -1 if item
// is not found or it is pending or transient. Item's unique id is retrieved
// via GetUniqueID method.
int GetCommittedItemIndexWithUniqueID(NavigationManager* navigation_manager,
                                      int unique_id);

}  // namespace web

#endif  // IOS_WEB_NAVIGATION_NAVIGATION_MANAGER_UTIL_H_
