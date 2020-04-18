// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_APPCACHE_SERVICE_H_
#define CONTENT_PUBLIC_BROWSER_APPCACHE_SERVICE_H_

#include <map>
#include <memory>
#include <set>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/common/appcache_info.h"
#include "net/base/completion_callback.h"
#include "url/origin.h"

namespace content {

// Refcounted container to avoid copying the collection in callbacks.
struct CONTENT_EXPORT AppCacheInfoCollection
    : public base::RefCountedThreadSafe<AppCacheInfoCollection> {
  AppCacheInfoCollection();

  std::map<url::Origin, AppCacheInfoVector> infos_by_origin;

 private:
  friend class base::RefCountedThreadSafe<AppCacheInfoCollection>;
  virtual ~AppCacheInfoCollection();
};

// Exposes a limited interface to the AppCacheService.
// Call these methods only on the IO thread.
class CONTENT_EXPORT AppCacheService {
 public:
  // Populates 'collection' with info about all of the appcaches stored
  // within the service, 'callback' is invoked upon completion. The service
  // acquires a reference to the 'collection' until completion.
  // This method always completes asynchronously.
  virtual void GetAllAppCacheInfo(AppCacheInfoCollection* collection,
                                  OnceCompletionCallback callback) = 0;

  // Deletes the group identified by 'manifest_url', 'callback' is
  // invoked upon completion. Upon completion, the cache group and
  // any resources within the group are no longer loadable and all
  // subresource loads for pages associated with a deleted group
  // will fail. This method always completes asynchronously.
  virtual void DeleteAppCacheGroup(const GURL& manifest_url,
                                   const net::CompletionCallback& callback) = 0;

 protected:
  virtual ~AppCacheService() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_APPCACHE_SERVICE_H_
