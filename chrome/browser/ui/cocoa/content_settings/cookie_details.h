// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_DETAILS_H_
#define CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_DETAILS_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/browsing_data/browsing_data_cache_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_service_worker_helper.h"
#include "content/public/browser/appcache_service.h"

class CookieTreeNode;

namespace net {
class CanonicalCookie;
}

// This enum specifies the type of information contained in the
// cookie details.
enum CocoaCookieDetailsType {
  // Represents grouping of cookie data, used in the cookie tree.
  kCocoaCookieDetailsTypeFolder = 0,

  // Detailed information about a cookie, used both in the cookie
  // tree and the cookie prompt.
  kCocoaCookieDetailsTypeCookie,

  // Detailed information about a web database used for
  // display in the cookie tree.
  kCocoaCookieDetailsTypeTreeDatabase,

  // Detailed information about local storage used for
  // display in the cookie tree.
  kCocoaCookieDetailsTypeTreeLocalStorage,

  // Detailed information about an appcache used for display in the
  // cookie tree.
  kCocoaCookieDetailsTypeTreeAppCache,

  // Detailed information about an IndexedDB used for display in the
  // cookie tree.
  kCocoaCookieDetailsTypeTreeIndexedDB,

  // Detailed information about a Service Worker used for display in the
  // cookie tree.
  kCocoaCookieDetailsTypeTreeServiceWorker,

  // Detailed information about Cache Storage used for display in the
  // cookie tree.
  kCocoaCookieDetailsTypeTreeCacheStorage,

  // Detailed information about a web database used for display
  // in the cookie prompt dialog.
  kCocoaCookieDetailsTypePromptDatabase,

  // Detailed information about local storage used for display
  // in the cookie prompt dialog.
  kCocoaCookieDetailsTypePromptLocalStorage,

  // Detailed information about app caches used for display
  // in the cookie prompt dialog.
  kCocoaCookieDetailsTypePromptAppCache
};

// This class contains all of the information that can be displayed in
// a cookie details view. Because the view uses bindings to display
// the cookie information, the methods that provide that information
// for display must be implemented directly on this class and not on any
// of its subclasses.
// If this system is rewritten to not use bindings, this class should be
// subclassed and specialized, rather than using an enum to determine type.
@interface CocoaCookieDetails : NSObject {
 @private
  CocoaCookieDetailsType type_;

  // Used for type kCocoaCookieDetailsTypeCookie to indicate whether
  // it should be possible to edit the expiration.
  BOOL canEditExpiration_;

  // Indicates whether a cookie has an explcit expiration. If not
  // it will expire with the session.
  BOOL hasExpiration_;

  // Only set for type kCocoaCookieDetailsTypeCookie.
  base::scoped_nsobject<NSString> content_;
  base::scoped_nsobject<NSString> path_;
  base::scoped_nsobject<NSString> sendFor_;
  // Stringifed dates.
  base::scoped_nsobject<NSString> expires_;

  // Only set for type kCocoaCookieDetailsTypeCookie and
  // kCocoaCookieDetailsTypeTreeAppCache nodes.
  base::scoped_nsobject<NSString> created_;

  // Only set for types kCocoaCookieDetailsTypeCookie, and
  // kCocoaCookieDetailsTypePromptDatabase nodes.
  base::scoped_nsobject<NSString> name_;

  // Only set for type kCocoaCookieDetailsTypeTreeLocalStorage,
  // kCocoaCookieDetailsTypeTreeDatabase,
  // kCocoaCookieDetailsTypePromptDatabase,
  // kCocoaCookieDetailsTypeTreeIndexedDB,
  // kCocoaCookieDetailsTypeTreeServiceWorker,
  // kCocoaCookieDetailsTypeTreeCacheStorage, and
  // kCocoaCookieDetailsTypeTreeAppCache nodes.
  base::scoped_nsobject<NSString> fileSize_;

  // Only set for types kCocoaCookieDetailsTypeTreeLocalStorage,
  // kCocoaCookieDetailsTypeTreeDatabase,
  // kCocoaCookieDetailsTypeTreeServiceWorker,
  // kCocoaCookieDetailsTypeTreeCacheStorage, and
  // kCocoaCookieDetailsTypeTreeIndexedDB nodes.
  base::scoped_nsobject<NSString> lastModified_;

  // Only set for type kCocoaCookieDetailsTypeTreeAppCache nodes.
  base::scoped_nsobject<NSString> lastAccessed_;

  // Only set for type kCocoaCookieDetailsTypeCookie,
  // kCocoaCookieDetailsTypePromptDatabase,
  // kCocoaCookieDetailsTypePromptLocalStorage,
  // kCocoaCookieDetailsTypePromptServiceWorker,
  // kCocoaCookieDetailsTypePromptCacheStorage, and
  // kCocoaCookieDetailsTypeTreeIndexedDB nodes.
  base::scoped_nsobject<NSString> domain_;

  // Only set for type kCocoaCookieTreeNodeTypeDatabaseStorage and
  // kCocoaCookieDetailsTypePromptDatabase nodes.
  base::scoped_nsobject<NSString> databaseDescription_;

  // Only set for type kCocoaCookieDetailsTypePromptLocalStorage.
  base::scoped_nsobject<NSString> localStorageKey_;
  base::scoped_nsobject<NSString> localStorageValue_;

  // Only set for type kCocoaCookieDetailsTypeTreeAppCache and
  // kCocoaCookieDetailsTypePromptAppCache.
  base::scoped_nsobject<NSString> manifestURL_;

  // Only set for type kCocoaCookieDetailsTypeTreeServiceWorker nodes.
  base::scoped_nsobject<NSString> scopes_;
}

@property(nonatomic, readonly) BOOL canEditExpiration;
@property(nonatomic) BOOL hasExpiration;
@property(nonatomic, readonly) CocoaCookieDetailsType type;

// The following methods are used in the bindings of subviews inside
// the cookie detail view. Note that the method that tests the
// visibility of the subview for cookie-specific information has a different
// polarity than the other visibility testing methods. This ensures that
// this subview is shown when there is no selection in the cookie tree,
// because a hidden value of |false| is generated when the key value binding
// is evaluated through a nil object. The other methods are bound using a
// |NSNegateBoolean| transformer, so that when there is a empty selection the
// hidden value is |true|.
- (BOOL)shouldHideCookieDetailsView;
- (BOOL)shouldShowLocalStorageTreeDetailsView;
- (BOOL)shouldShowLocalStoragePromptDetailsView;
- (BOOL)shouldShowDatabaseTreeDetailsView;
- (BOOL)shouldShowDatabasePromptDetailsView;
- (BOOL)shouldShowAppCachePromptDetailsView;
- (BOOL)shouldShowAppCacheTreeDetailsView;
- (BOOL)shouldShowIndexedDBTreeDetailsView;
- (BOOL)shouldShowServiceWorkerTreeDetailsView;
- (BOOL)shouldShowCacheStorageTreeDetailsView;

- (NSString*)name;
- (NSString*)content;
- (NSString*)domain;
- (NSString*)path;
- (NSString*)sendFor;
- (NSString*)created;
- (NSString*)expires;
- (NSString*)fileSize;
- (NSString*)lastModified;
- (NSString*)lastAccessed;
- (NSString*)databaseDescription;
- (NSString*)localStorageKey;
- (NSString*)localStorageValue;
- (NSString*)manifestURL;
- (NSString*)scopes;

// Used for folders in the cookie tree.
- (id)initAsFolder;

// Used for cookie details in both the cookie tree and the cookie prompt dialog.
- (id)initWithCookie:(const net::CanonicalCookie*)treeNode
   canEditExpiration:(BOOL)canEditExpiration;

// Used for database details in the cookie tree.
- (id)initWithDatabase:
    (const BrowsingDataDatabaseHelper::DatabaseInfo*)databaseInfo;

// Used for local storage details in the cookie tree.
- (id)initWithLocalStorage:
    (const BrowsingDataLocalStorageHelper::LocalStorageInfo*)localStorageInfo;

// Used for database details in the cookie prompt dialog.
- (id)initWithDatabase:(const std::string&)domain
          databaseName:(const base::string16&)databaseName
   databaseDescription:(const base::string16&)databaseDescription
              fileSize:(unsigned long)fileSize;

// -initWithAppCacheInfo: creates a cookie details with the manifest URL plus
// all of this additional information that is available after an appcache is
// actually created, including its creation date, size and last accessed time.
- (id)initWithAppCacheInfo:(const content::AppCacheInfo*)appcacheInfo;

// Used for local storage details in the cookie prompt dialog.
- (id)initWithLocalStorage:(const std::string&)domain
                       key:(const base::string16&)key
                     value:(const base::string16&)value;

// -initWithAppCacheManifestURL: is called when the cookie prompt is displayed
// for an appcache, at that time only the manifest URL of the appcache is known.
- (id)initWithAppCacheManifestURL:(const std::string&)manifestURL;

// Used for IndexedDB details in the cookie tree.
- (id)initWithIndexedDBInfo:
    (const content::IndexedDBInfo*)indexedDB;

// Used for ServiceWorker details in the cookie tree.
- (id)initWithServiceWorkerUsageInfo:
    (const content::ServiceWorkerUsageInfo*)serviceWorker;

// Used for CacheStorage details in the cookie tree.
- (id)initWithCacheStorageUsageInfo:
    (const content::CacheStorageUsageInfo*)cacheStorage;

// A factory method to create a configured instance given a node from
// the cookie tree in |treeNode|.
+ (CocoaCookieDetails*)createFromCookieTreeNode:(CookieTreeNode*)treeNode;

@end

// The subpanes of the cookie details view expect to be able to bind to methods
// through a key path in the form |content.details.xxxx|. This class serves as
// an adapter that simply wraps a |CocoaCookieDetails| object. An instance of
// this class is set as the content object for cookie details view's object
// controller so that key paths are properly resolved through to the
// |CocoaCookieDetails| object for the cookie prompt.
@interface CookiePromptContentDetailsAdapter : NSObject {
 @private
  base::scoped_nsobject<CocoaCookieDetails> details_;
}

- (CocoaCookieDetails*)details;

// The adapter assumes ownership of the details object
// in its initializer.
- (id)initWithDetails:(CocoaCookieDetails*)details;
@end

#endif  // CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_COOKIE_DETAILS_H_
