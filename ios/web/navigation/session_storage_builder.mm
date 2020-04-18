// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/session_storage_builder.h"

#include <memory>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "ios/web/navigation/legacy_navigation_manager_impl.h"
#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/navigation/navigation_item_storage_builder.h"
#include "ios/web/navigation/navigation_manager_impl.h"
#import "ios/web/public/crw_session_storage.h"
#import "ios/web/public/serializable_user_data_manager.h"
#import "ios/web/web_state/session_certificate_policy_cache_impl.h"
#include "ios/web/web_state/session_certificate_policy_cache_storage_builder.h"
#import "ios/web/web_state/web_state_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

CRWSessionStorage* SessionStorageBuilder::BuildStorage(
    WebStateImpl* web_state) const {
  DCHECK(web_state);
  web::NavigationManagerImpl* navigation_manager =
      web_state->navigation_manager_.get();
  DCHECK(navigation_manager);
  CRWSessionStorage* session_storage = [[CRWSessionStorage alloc] init];
  session_storage.hasOpener = web_state->HasOpener();
  session_storage.lastCommittedItemIndex =
      navigation_manager->GetLastCommittedItemIndex();
  session_storage.previousItemIndex =
      navigation_manager->GetPreviousItemIndex();
  NSMutableArray* item_storages = [[NSMutableArray alloc] init];
  NavigationItemStorageBuilder item_storage_builder;
  for (size_t index = 0;
       index < static_cast<size_t>(navigation_manager->GetItemCount());
       ++index) {
    web::NavigationItemImpl* item =
        navigation_manager->GetNavigationItemImplAtIndex(index);
    [item_storages addObject:item_storage_builder.BuildStorage(item)];
  }
  session_storage.itemStorages = item_storages;
  SessionCertificatePolicyCacheStorageBuilder cert_builder;
  session_storage.certPolicyCacheStorage = cert_builder.BuildStorage(
      &web_state->GetSessionCertificatePolicyCacheImpl());
  web::SerializableUserDataManager* user_data_manager =
      web::SerializableUserDataManager::FromWebState(web_state);
  [session_storage
      setSerializableUserData:user_data_manager->CreateSerializableUserData()];
  return session_storage;
}

void SessionStorageBuilder::ExtractSessionState(
    WebStateImpl* web_state,
    CRWSessionStorage* storage) const {
  DCHECK(web_state);
  DCHECK(storage);
  web_state->created_with_opener_ = storage.hasOpener;
  NSArray* item_storages = storage.itemStorages;
  web::ScopedNavigationItemList items(item_storages.count);
  NavigationItemStorageBuilder item_storage_builder;
  for (size_t index = 0; index < item_storages.count; ++index) {
    std::unique_ptr<NavigationItemImpl> item_impl =
        item_storage_builder.BuildNavigationItemImpl(item_storages[index]);
    items[index] = std::move(item_impl);
  }
  web_state->navigation_manager_->Restore(storage.lastCommittedItemIndex,
                                          std::move(items));
  web_state->navigation_manager_->SetPreviousItemIndex(
      storage.previousItemIndex);

  SessionCertificatePolicyCacheStorageBuilder cert_builder;
  std::unique_ptr<SessionCertificatePolicyCacheImpl> cert_policy_cache =
      cert_builder.BuildSessionCertificatePolicyCache(
          storage.certPolicyCacheStorage);
  if (!cert_policy_cache)
    cert_policy_cache = std::make_unique<SessionCertificatePolicyCacheImpl>();
  web_state->certificate_policy_cache_ = std::move(cert_policy_cache);
  web::SerializableUserDataManager::FromWebState(web_state)
      ->AddSerializableUserData(storage.userData);
}

}  // namespace web
