// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_RESOURCE_REQUESTER_INFO_H_
#define CONTENT_BROWSER_LOADER_RESOURCE_REQUESTER_INFO_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"

namespace net {
class URLRequestContext;
}  // namespace net

namespace storage {
class FileSystemContext;
}  // namespace storage

namespace content {
class ChromeAppCacheService;
class ChromeBlobStorageContext;
class ResourceContext;
class ResourceMessageFilter;
class ServiceWorkerContextWrapper;

// This class represents the type of resource requester.
// Currently there are four types:
// - Requesters that request resources from renderer processes.
// - Requesters that request resources within the browser process for browser
//   side navigation (aka PlzNavigate).
// - Requesters that request resources for download or page save.
// - Requesters that request service worker navigation preload requests.
class CONTENT_EXPORT ResourceRequesterInfo
    : public base::RefCountedThreadSafe<ResourceRequesterInfo> {
 public:
  typedef base::Callback<void(ResourceType resource_type,
                              ResourceContext**,
                              net::URLRequestContext**)>
      GetContextsCallback;

  // Creates a ResourceRequesterInfo for a requester that requests resources
  // from the renderer process.
  static scoped_refptr<ResourceRequesterInfo> CreateForRenderer(
      int child_id,
      ChromeAppCacheService* appcache_service,
      ChromeBlobStorageContext* blob_storage_context,
      storage::FileSystemContext* file_system_context,
      ServiceWorkerContextWrapper* service_worker_context,
      const GetContextsCallback& get_contexts_callback);

  // Creates a ResourceRequesterInfo for a requester that requests resources
  // from the renderer process for testing.
  static scoped_refptr<ResourceRequesterInfo> CreateForRendererTesting(
      int child_id);

  // Creates a ResourceRequesterInfo for a requester that requests resources
  // within the browser process for browser side navigation (aka PlzNavigate).
  static scoped_refptr<ResourceRequesterInfo> CreateForBrowserSideNavigation(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context);

  // Creates a ResourceRequesterInfo for a requester that requests resources for
  // download or page save.
  static scoped_refptr<ResourceRequesterInfo> CreateForDownloadOrPageSave(
      int child_id);

  // Creates a ResourceRequesterInfo for a service worker navigation preload
  // request. When PlzNavigate is enabled, |original_request_info| must be
  // browser side navigation type. Otherwise it must be renderer type.
  static scoped_refptr<ResourceRequesterInfo> CreateForNavigationPreload(
      ResourceRequesterInfo* original_request_info);

  // Creates a ResourceRequesterInfo for a requester that requests certificates
  // for signed exchange.
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html
  static scoped_refptr<ResourceRequesterInfo>
  CreateForCertificateFetcherForSignedExchange(
      const GetContextsCallback& get_contexts_callback);

  bool IsRenderer() const { return type_ == RequesterType::RENDERER; }
  bool IsBrowserSideNavigation() const {
    return type_ == RequesterType::BROWSER_SIDE_NAVIGATION;
  }
  bool IsNavigationPreload() const {
    return type_ == RequesterType::NAVIGATION_PRELOAD;
  }
  bool IsCertificateFetcherForSignedExchange() const {
    return type_ == RequesterType::CERTIFICATE_FETCHER_FOR_SIGNED_EXCHANGE;
  }

  // Returns the renderer process ID associated with the requester. Returns -1
  // for browser side navigation requester. Even if the ResourceMessageFilter
  // has been destroyed, this method of renderer type requester info returns the
  // valid process ID which was assigned to the renderer process of the filter.
  int child_id() const { return child_id_; }

  // Sets the ResourceMessageFilter of the renderer type requester info.
  void set_filter(base::WeakPtr<ResourceMessageFilter> filter);

  // Returns the filter for sending IPC messages to the renderer process. This
  // may return null if the process exited. This method is available only for
  // renderer type requester.
  ResourceMessageFilter* filter() { return filter_.get(); }

  // Returns the ResourceContext and URLRequestContext associated to the
  // requester. Currently this method is available only for renderer type
  // requester and service worker navigation preload type.
  void GetContexts(ResourceType resource_type,
                   ResourceContext** resource_context,
                   net::URLRequestContext** request_context) const;

  // Returns the ChromeAppCacheService associated with the requester. Currently
  // this method is available only for renderer type requester.
  ChromeAppCacheService* appcache_service() { return appcache_service_.get(); }

  // Returns the ChromeBlobStorageContext associated with the requester.
  // Currently this method is available only for renderer type requester.
  ChromeBlobStorageContext* blob_storage_context() {
    return blob_storage_context_.get();
  }

  // Returns the FileSystemContext associated with the requester. Currently this
  // method is available only for renderer type requester.
  storage::FileSystemContext* file_system_context() {
    return file_system_context_.get();
  }

  // Returns the ServiceWorkerContext associated with the requester. Currently
  // this method is available for renderer type requester, browser side
  // navigation type requester and service worker navigation preload type
  // requester.
  ServiceWorkerContextWrapper* service_worker_context() {
    return service_worker_context_.get();
  }

 private:
  friend class base::RefCountedThreadSafe<ResourceRequesterInfo>;

  enum class RequesterType {
    RENDERER,
    BROWSER_SIDE_NAVIGATION,
    DOWNLOAD_OR_PAGE_SAVE,
    NAVIGATION_PRELOAD,
    CERTIFICATE_FETCHER_FOR_SIGNED_EXCHANGE
  };

  ResourceRequesterInfo(RequesterType type,
                        int child_id,
                        ChromeAppCacheService* appcache_service,
                        ChromeBlobStorageContext* blob_storage_context,
                        storage::FileSystemContext* file_system_context,
                        ServiceWorkerContextWrapper* service_worker_context,
                        const GetContextsCallback& get_contexts_callback);
  ~ResourceRequesterInfo();

  const RequesterType type_;
  // The filter might be deleted if the process exited.
  base::WeakPtr<ResourceMessageFilter> filter_;
  const int child_id_;
  const scoped_refptr<ChromeAppCacheService> appcache_service_;
  const scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;
  const scoped_refptr<storage::FileSystemContext> file_system_context_;
  const scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;
  const GetContextsCallback get_contexts_callback_;

  DISALLOW_COPY_AND_ASSIGN(ResourceRequesterInfo);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_RESOURCE_REQUESTER_INFO_H_
