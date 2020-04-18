// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/content_settings/cookie_details.h"

#import "base/i18n/time_formatting.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/browsing_data/cookies_tree_model.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/appcache_service.h"
#include "net/cookies/canonical_cookie.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/text/bytes_formatting.h"

#pragma mark Cocoa Cookie Details

@implementation CocoaCookieDetails

@synthesize canEditExpiration = canEditExpiration_;
@synthesize hasExpiration = hasExpiration_;
@synthesize type = type_;

- (BOOL)shouldHideCookieDetailsView {
  return type_ != kCocoaCookieDetailsTypeFolder &&
      type_ != kCocoaCookieDetailsTypeCookie;
}

- (BOOL)shouldShowLocalStorageTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeLocalStorage;
}

- (BOOL)shouldShowLocalStoragePromptDetailsView {
  return type_ == kCocoaCookieDetailsTypePromptLocalStorage;
}

- (BOOL)shouldShowDatabaseTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeDatabase;
}

- (BOOL)shouldShowAppCacheTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeAppCache;
}

- (BOOL)shouldShowDatabasePromptDetailsView {
  return type_ == kCocoaCookieDetailsTypePromptDatabase;
}

- (BOOL)shouldShowAppCachePromptDetailsView {
  return type_ == kCocoaCookieDetailsTypePromptAppCache;
}

- (BOOL)shouldShowIndexedDBTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeIndexedDB;
}

- (BOOL)shouldShowServiceWorkerTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeServiceWorker;
}

- (BOOL)shouldShowCacheStorageTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeCacheStorage;
}

- (NSString*)name {
  return name_.get();
}

- (NSString*)content {
  return content_.get();
}

- (NSString*)domain {
  return domain_.get();
}

- (NSString*)path {
  return path_.get();
}

- (NSString*)sendFor {
  return sendFor_.get();
}

- (NSString*)created {
  return created_.get();
}

- (NSString*)expires {
  return expires_.get();
}

- (NSString*)fileSize {
  return fileSize_.get();
}

- (NSString*)lastModified {
  return lastModified_.get();
}

- (NSString*)lastAccessed {
  return lastAccessed_.get();
}

- (NSString*)databaseDescription {
  return databaseDescription_.get();
}

- (NSString*)localStorageKey {
  return localStorageKey_.get();
}

- (NSString*)localStorageValue {
  return localStorageValue_.get();
}

- (NSString*)manifestURL {
  return manifestURL_.get();
}

- (NSString*)scopes {
  return scopes_.get();
}

- (id)initAsFolder {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeFolder;
  }
  return self;
}

- (id)initWithCookie:(const net::CanonicalCookie*)cookie
   canEditExpiration:(BOOL)canEditExpiration {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeCookie;
    hasExpiration_ = cookie->IsPersistent();
    canEditExpiration_ = canEditExpiration && hasExpiration_;
    name_.reset([base::SysUTF8ToNSString(cookie->Name()) retain]);
    content_.reset([base::SysUTF8ToNSString(cookie->Value()) retain]);
    path_.reset([base::SysUTF8ToNSString(cookie->Path()) retain]);
    domain_.reset([base::SysUTF8ToNSString(cookie->Domain()) retain]);

    if (cookie->IsPersistent()) {
      expires_.reset([base::SysUTF16ToNSString(
          base::TimeFormatFriendlyDateAndTime(cookie->ExpiryDate())) retain]);
    } else {
      expires_.reset([l10n_util::GetNSStringWithFixup(
          IDS_COOKIES_COOKIE_EXPIRES_SESSION) retain]);
    }

    created_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(cookie->CreationDate())) retain]);

    sendFor_.reset([l10n_util::GetNSStringWithFixup(
        CookiesTreeModel::GetSendForMessageID(*cookie)) retain]);
  }
  return self;
}

- (id)initWithDatabase:(const BrowsingDataDatabaseHelper::DatabaseInfo*)
    databaseInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeDatabase;
    canEditExpiration_ = NO;
    databaseDescription_.reset([base::SysUTF8ToNSString(
        databaseInfo->description) retain]);
    fileSize_.reset([base::SysUTF16ToNSString(
        ui::FormatBytes(databaseInfo->size)) retain]);
    lastModified_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(
            databaseInfo->last_modified)) retain]);
  }
  return self;
}

- (id)initWithLocalStorage:(
    const BrowsingDataLocalStorageHelper::LocalStorageInfo*)storageInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeLocalStorage;
    canEditExpiration_ = NO;
    domain_.reset(
        [base::SysUTF8ToNSString(storageInfo->origin_url.spec()) retain]);
    fileSize_.reset(
        [base::SysUTF16ToNSString(ui::FormatBytes(storageInfo->size)) retain]);
    lastModified_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(
            storageInfo->last_modified)) retain]);
  }
  return self;
}

- (id)initWithAppCacheInfo:(const content::AppCacheInfo*)appcacheInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeAppCache;
    canEditExpiration_ = NO;
    manifestURL_.reset([base::SysUTF8ToNSString(
        appcacheInfo->manifest_url.spec()) retain]);
    fileSize_.reset([base::SysUTF16ToNSString(
        ui::FormatBytes(appcacheInfo->size)) retain]);
    created_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(
            appcacheInfo->creation_time)) retain]);
    lastAccessed_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(
            appcacheInfo->last_access_time)) retain]);
  }
  return self;
}

- (id)initWithDatabase:(const std::string&)domain
          databaseName:(const base::string16&)databaseName
   databaseDescription:(const base::string16&)databaseDescription
              fileSize:(unsigned long)fileSize {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypePromptDatabase;
    canEditExpiration_ = NO;
    name_.reset([base::SysUTF16ToNSString(databaseName) retain]);
    domain_.reset([base::SysUTF8ToNSString(domain) retain]);
    databaseDescription_.reset(
        [base::SysUTF16ToNSString(databaseDescription) retain]);
    fileSize_.reset(
        [base::SysUTF16ToNSString(ui::FormatBytes(fileSize)) retain]);
  }
  return self;
}

- (id)initWithLocalStorage:(const std::string&)domain
                       key:(const base::string16&)key
                     value:(const base::string16&)value {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypePromptLocalStorage;
    canEditExpiration_ = NO;
    domain_.reset([base::SysUTF8ToNSString(domain) retain]);
    localStorageKey_.reset([base::SysUTF16ToNSString(key) retain]);
    localStorageValue_.reset([base::SysUTF16ToNSString(value) retain]);
  }
  return self;
}

- (id)initWithAppCacheManifestURL:(const std::string&)manifestURL {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypePromptAppCache;
    canEditExpiration_ = NO;
    manifestURL_.reset([base::SysUTF8ToNSString(manifestURL) retain]);
  }
  return self;
}

- (id)initWithIndexedDBInfo:
    (const content::IndexedDBInfo*)indexedDBInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeIndexedDB;
    canEditExpiration_ = NO;
    domain_.reset(
        [base::SysUTF8ToNSString(indexedDBInfo->origin.spec()) retain]);
    fileSize_.reset([base::SysUTF16ToNSString(
        ui::FormatBytes(indexedDBInfo->size)) retain]);
    lastModified_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(indexedDBInfo->last_modified))
        retain]);
  }
  return self;
}

- (id)initWithServiceWorkerUsageInfo:
    (const content::ServiceWorkerUsageInfo*)serviceWorkerInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeServiceWorker;
    canEditExpiration_ = NO;
    domain_.reset([base::SysUTF8ToNSString(
        serviceWorkerInfo->origin.spec()) retain]);
    fileSize_.reset([base::SysUTF16ToNSString(
        ui::FormatBytes(serviceWorkerInfo->total_size_bytes)) retain]);

    NSMutableArray *scopes = [[NSMutableArray alloc]
                         initWithCapacity:serviceWorkerInfo->scopes.size()];
    for (std::vector<GURL>::const_iterator it =
             serviceWorkerInfo->scopes.begin();
         it != serviceWorkerInfo->scopes.end(); ++it) {
      [scopes addObject:base::SysUTF8ToNSString(it->spec())];
    }
    scopes_.reset([[scopes componentsJoinedByString:@","] retain]);
  }
  return self;
}

- (id)initWithCacheStorageUsageInfo:
    (const content::CacheStorageUsageInfo*)cacheStorageInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeCacheStorage;
    canEditExpiration_ = NO;
    domain_.reset(
        [base::SysUTF8ToNSString(cacheStorageInfo->origin.spec()) retain]);
    fileSize_.reset([base::SysUTF16ToNSString(
        ui::FormatBytes(cacheStorageInfo->total_size_bytes)) retain]);
    lastModified_.reset([base::SysUTF16ToNSString(
        base::TimeFormatFriendlyDateAndTime(cacheStorageInfo->last_modified))
        retain]);
  }
  return self;
}

+ (CocoaCookieDetails*)createFromCookieTreeNode:(CookieTreeNode*)treeNode {
  CookieTreeNode::DetailedInfo info = treeNode->GetDetailedInfo();
  CookieTreeNode::DetailedInfo::NodeType nodeType = info.node_type;
  switch (nodeType) {
    case CookieTreeNode::DetailedInfo::TYPE_COOKIE:
      return [[[CocoaCookieDetails alloc] initWithCookie:info.cookie
                                       canEditExpiration:NO] autorelease];
    case CookieTreeNode::DetailedInfo::TYPE_DATABASE:
      return [[[CocoaCookieDetails alloc]
          initWithDatabase:info.database_info] autorelease];
    case CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGE:
      return [[[CocoaCookieDetails alloc]
          initWithLocalStorage:info.local_storage_info] autorelease];
    case CookieTreeNode::DetailedInfo::TYPE_APPCACHE:
      return [[[CocoaCookieDetails alloc]
          initWithAppCacheInfo:info.appcache_info] autorelease];
    case CookieTreeNode::DetailedInfo::TYPE_INDEXED_DB:
      return [[[CocoaCookieDetails alloc]
          initWithIndexedDBInfo:info.indexed_db_info] autorelease];
    case CookieTreeNode::DetailedInfo::TYPE_SERVICE_WORKER:
      return [[[CocoaCookieDetails alloc]
          initWithServiceWorkerUsageInfo:info.service_worker_info] autorelease];
    case CookieTreeNode::DetailedInfo::TYPE_CACHE_STORAGE:
      return [[[CocoaCookieDetails alloc]
          initWithCacheStorageUsageInfo:info.cache_storage_info] autorelease];
    default:
      return [[[CocoaCookieDetails alloc] initAsFolder] autorelease];
  }
}

@end

#pragma mark Content Object Adapter

@implementation CookiePromptContentDetailsAdapter

- (id)initWithDetails:(CocoaCookieDetails*)details {
  if ((self = [super init])) {
    details_.reset([details retain]);
  }
  return self;
}

- (CocoaCookieDetails*)details {
  return details_.get();
}

@end
