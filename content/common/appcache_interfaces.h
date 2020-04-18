// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_APPCACHE_INTERFACES_H_
#define CONTENT_COMMON_APPCACHE_INTERFACES_H_

#include <stdint.h>

#include <string>

#include "base/files/file_path.h"
#include "content/common/appcache.mojom.h"
#include "content/public/common/appcache_info.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace net {
class URLRequest;
}

namespace content {

// Defines constants, types, and abstract classes used in the main
// process and in child processes.

using mojom::AppCacheEventID;
using mojom::AppCacheErrorReason;
using mojom::AppCacheResourceInfo;
using mojom::AppCacheErrorDetails;

// Temporarily renumber them in wierd way, to help remove LOG_TIP from WebKit
enum AppCacheLogLevel {
  APPCACHE_LOG_VERBOSE,
  APPCACHE_LOG_INFO,
  APPCACHE_LOG_WARNING,
  APPCACHE_LOG_ERROR
};

enum AppCacheNamespaceType {
  APPCACHE_FALLBACK_NAMESPACE,
  APPCACHE_INTERCEPT_NAMESPACE,
  APPCACHE_NETWORK_NAMESPACE
};

typedef std::vector<AppCacheResourceInfo> AppCacheResourceInfoVector;

struct CONTENT_EXPORT AppCacheNamespace {
  AppCacheNamespace();  // Type is APPCACHE_FALLBACK_NAMESPACE by default.
  AppCacheNamespace(AppCacheNamespaceType type, const GURL& url,
      const GURL& target, bool is_pattern);
  ~AppCacheNamespace();

  bool IsMatch(const GURL& url) const;

  AppCacheNamespaceType type;
  GURL namespace_url;
  GURL target_url;
  bool is_pattern;
};

typedef std::vector<AppCacheNamespace> AppCacheNamespaceVector;

// Interface used by backend (browser-process) to talk to frontend (renderer).
class CONTENT_EXPORT AppCacheFrontend {
 public:
  virtual void OnCacheSelected(
      int host_id, const AppCacheInfo& info) = 0;
  virtual void OnStatusChanged(const std::vector<int>& host_ids,
                               AppCacheStatus status) = 0;
  virtual void OnEventRaised(const std::vector<int>& host_ids,
                             AppCacheEventID event_id) = 0;
  virtual void OnProgressEventRaised(const std::vector<int>& host_ids,
                                     const GURL& url,
                                     int num_total, int num_complete) = 0;
  virtual void OnErrorEventRaised(
      const std::vector<int>& host_ids,
      const AppCacheErrorDetails& details) = 0;
  virtual void OnContentBlocked(int host_id,
                                const GURL& manifest_url) = 0;
  virtual void OnLogMessage(int host_id, AppCacheLogLevel log_level,
                            const std::string& message) = 0;
  // In the network service world, we pass the URLLoaderFactory instance to be
  // used to issue subresource requeste in the |loader_factory_pipe_handle|
  // parameter.
  virtual void OnSetSubresourceFactory(
      int host_id,
      network::mojom::URLLoaderFactoryPtr url_loader_factory) = 0;

  virtual ~AppCacheFrontend() {}
};

// Interface used by frontend (renderer) to talk to backend (browser-process).
class CONTENT_EXPORT AppCacheBackend {
 public:
  virtual void RegisterHost(int host_id) = 0;
  virtual void UnregisterHost(int host_id) = 0;
  virtual void SetSpawningHostId(int host_id, int spawning_host_id) = 0;
  virtual void SelectCache(int host_id,
                           const GURL& document_url,
                           const int64_t cache_document_was_loaded_from,
                           const GURL& manifest_url) = 0;
  virtual void SelectCacheForSharedWorker(int host_id, int64_t appcache_id) = 0;
  virtual void MarkAsForeignEntry(int host_id,
                                  const GURL& document_url,
                                  int64_t cache_document_was_loaded_from) = 0;
  virtual AppCacheStatus GetStatus(int host_id) = 0;
  virtual bool StartUpdate(int host_id) = 0;
  virtual bool SwapCache(int host_id) = 0;
  virtual void GetResourceList(
      int host_id, std::vector<AppCacheResourceInfo>* resource_infos) = 0;

 protected:
  virtual ~AppCacheBackend() {}
};

// Useful string constants.
CONTENT_EXPORT extern const char kHttpGETMethod[];
CONTENT_EXPORT extern const char kHttpHEADMethod[];

// base::CommandLine flag to turn this experimental feature on.
CONTENT_EXPORT extern const char kEnableExecutableHandlers[];

CONTENT_EXPORT bool IsSchemeSupportedForAppCache(const GURL& url);
CONTENT_EXPORT bool IsMethodSupportedForAppCache(
    const std::string& method);

CONTENT_EXPORT extern const base::FilePath::CharType
    kAppCacheDatabaseName[];

}  // namespace

#endif  // CONTENT_COMMON_APPCACHE_INTERFACES_H_
