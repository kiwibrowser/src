// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_RENDERER_HOST_AW_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define ANDROID_WEBVIEW_BROWSER_RENDERER_HOST_AW_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include <map>
#include <memory>
#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace content {
class ResourceContext;
struct ResourceResponse;
}  // namespace content

namespace android_webview {

class IoThreadClientThrottle;

class AwResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  static void ResourceDispatcherHostCreated();

  // Overriden methods from ResourceDispatcherHostDelegate.
  void RequestBeginning(net::URLRequest* request,
                        content::ResourceContext* resource_context,
                        content::AppCacheService* appcache_service,
                        content::ResourceType resource_type,
                        std::vector<std::unique_ptr<content::ResourceThrottle>>*
                            throttles) override;
  void DownloadStarting(net::URLRequest* request,
                        content::ResourceContext* resource_context,
                        bool is_content_initiated,
                        bool must_download,
                        bool is_new_request,
                        std::vector<std::unique_ptr<content::ResourceThrottle>>*
                            throttles) override;
  void OnResponseStarted(net::URLRequest* request,
                         content::ResourceContext* resource_context,
                         network::ResourceResponse* response) override;

  void OnRequestRedirected(const GURL& redirect_url,
                           net::URLRequest* request,
                           content::ResourceContext* resource_context,
                           network::ResourceResponse* response) override;

  void RequestComplete(net::URLRequest* request) override;

  void RemovePendingThrottleOnIoThread(IoThreadClientThrottle* throttle);

  static void OnIoThreadClientReady(int new_render_process_id,
                                    int new_render_frame_id);
  static void AddPendingThrottle(int render_process_id,
                                 int render_frame_id,
                                 IoThreadClientThrottle* pending_throttle);

 private:
  friend struct base::LazyInstanceTraitsBase<AwResourceDispatcherHostDelegate>;
  AwResourceDispatcherHostDelegate();
  ~AwResourceDispatcherHostDelegate() override;

  // These methods must be called on IO thread.
  void OnIoThreadClientReadyInternal(int new_render_process_id,
                                     int new_render_frame_id);
  void AddPendingThrottleOnIoThread(int render_process_id,
                                    int render_frame_id,
                                    IoThreadClientThrottle* pending_throttle);
  void AddExtraHeadersIfNeeded(net::URLRequest* request,
                               content::ResourceContext* resource_context);

  // Pair of render_process_id and render_frame_id.
  typedef std::pair<int, int> FrameRouteIDPair;
  typedef std::map<FrameRouteIDPair, IoThreadClientThrottle*>
      PendingThrottleMap;

  // Only accessed on the IO thread.
  PendingThrottleMap pending_throttles_;

  DISALLOW_COPY_AND_ASSIGN(AwResourceDispatcherHostDelegate);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_RENDERER_HOST_AW_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
