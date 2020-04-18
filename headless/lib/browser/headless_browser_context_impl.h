// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_BROWSER_CONTEXT_IMPL_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_BROWSER_CONTEXT_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/files/file_path.h"
#include "base/unguessable_token.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/resource_context.h"
#include "headless/lib/browser/headless_browser_context_options.h"
#include "headless/lib/browser/headless_network_conditions.h"
#include "headless/lib/browser/headless_url_request_context_getter.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_browser_context.h"
#include "headless/public/headless_export.h"

namespace headless {
class HeadlessBrowserImpl;
class HeadlessResourceContext;
class HeadlessWebContentsImpl;

class HEADLESS_EXPORT HeadlessBrowserContextImpl final
    : public HeadlessBrowserContext,
      public content::BrowserContext {
 public:
  ~HeadlessBrowserContextImpl() override;

  static HeadlessBrowserContextImpl* From(
      HeadlessBrowserContext* browser_context);
  static HeadlessBrowserContextImpl* From(
      content::BrowserContext* browser_context);

  static std::unique_ptr<HeadlessBrowserContextImpl> Create(
      HeadlessBrowserContext::Builder* builder);

  // HeadlessBrowserContext implementation:
  HeadlessWebContents::Builder CreateWebContentsBuilder() override;
  std::vector<HeadlessWebContents*> GetAllWebContents() override;
  HeadlessWebContents* GetWebContentsForDevToolsAgentHostId(
      const std::string& devtools_agent_host_id) override;
  void Close() override;
  const std::string& Id() const override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  void SetDevToolsFrameToken(int render_process_id,
                             int render_frame_routing_id,
                             const base::UnguessableToken& devtools_frame_token,
                             int frame_tree_node_id);

  void RemoveDevToolsFrameToken(int render_process_id,
                                int render_frame_routing_id,
                                int frame_tree_node_id);

  // BrowserContext implementation:
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() const override;
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::PermissionManager* GetPermissionManager() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override;

  HeadlessWebContents* CreateWebContents(HeadlessWebContents::Builder* builder);
  // Register web contents which were created not through Headless API
  // (calling window.open() is a best example for this).
  void RegisterWebContents(
      std::unique_ptr<HeadlessWebContentsImpl> web_contents);
  void DestroyWebContents(HeadlessWebContentsImpl* web_contents);

  HeadlessBrowserImpl* browser() const;
  const HeadlessBrowserContextOptions* options() const;

  // Returns the DevTools frame token for the corresponding RenderFrameHost or
  // null if can't be found. Can be called on any thread.
  const base::UnguessableToken* GetDevToolsFrameToken(
      int render_process_id,
      int render_frame_id) const;

  // Returns the DevTools frame token for the corresponding frame tree node id
  // or null if can't be found. Can be called on any thread.
  const base::UnguessableToken* GetDevToolsFrameTokenForFrameTreeNodeId(
      int frame_tree_node_id) const;

  void SetRemoveHeaders(bool should_remove_headers);
  bool ShouldRemoveHeaders() const;

  void NotifyChildContentsCreated(HeadlessWebContentsImpl* parent,
                                  HeadlessWebContentsImpl* child);

  // This will be called on the IO thread.
  void NotifyUrlRequestFailed(net::URLRequest* request,
                              int net_error,
                              DevToolsStatus devtools_status);

  void NotifyMetadataForResource(const GURL& url,
                                 net::IOBuffer* buf,
                                 int buf_len);

  void SetNetworkConditions(HeadlessNetworkConditions conditions);
  HeadlessNetworkConditions GetNetworkConditions() override;

 private:
  HeadlessBrowserContextImpl(
      HeadlessBrowserImpl* browser,
      std::unique_ptr<HeadlessBrowserContextOptions> context_options);

  // Performs initialization of the HeadlessBrowserContextImpl while IO is still
  // allowed on the current thread.
  void InitWhileIOAllowed();

  HeadlessBrowserImpl* browser_;  // Not owned.
  std::unique_ptr<HeadlessBrowserContextOptions> context_options_;
  std::unique_ptr<HeadlessResourceContext> resource_context_;
  scoped_refptr<HeadlessURLRequestContextGetter> url_request_getter_;
  base::FilePath path_;
  base::Lock observers_lock_;
  base::ObserverList<Observer> observers_;
  bool should_remove_headers_;

  std::unordered_map<std::string, std::unique_ptr<HeadlessWebContents>>
      web_contents_map_;

  // Guards |devtools_frame_token_map_| from being concurrently written on the
  // UI thread and read on the IO thread.
  // TODO(alexclarke): Remove if we can add DevTools frame token ID to
  // ResourceRequestInfo. See https://crbug.com/715541
  mutable base::Lock devtools_frame_token_map_lock_;
  base::flat_map<std::pair<int, int>, base::UnguessableToken>
      devtools_frame_token_map_;
  base::flat_map<int, base::UnguessableToken>
      frame_tree_node_id_to_devtools_frame_token_map_;

  std::unique_ptr<content::PermissionManager> permission_manager_;

  HeadlessNetworkConditions network_conditions_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessBrowserContextImpl);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_BROWSER_CONTEXT_IMPL_H_
