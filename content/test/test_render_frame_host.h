// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_RENDER_FRAME_HOST_H_
#define CONTENT_TEST_TEST_RENDER_FRAME_HOST_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/common/navigation_params.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_render_widget_host.h"
#include "ui/base/page_transition_types.h"

struct FrameHostMsg_DidCommitProvisionalLoad_Params;

namespace net {
class HostPortPair;
}

namespace content {

class TestRenderFrameHostCreationObserver : public WebContentsObserver {
 public:
  explicit TestRenderFrameHostCreationObserver(WebContents* web_contents);
  ~TestRenderFrameHostCreationObserver() override;

  // WebContentsObserver implementation.
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;

  RenderFrameHost* last_created_frame() const { return last_created_frame_; }

 private:
  RenderFrameHost* last_created_frame_;
};

class TestRenderFrameHost : public RenderFrameHostImpl,
                            public RenderFrameHostTester {
 public:
  TestRenderFrameHost(SiteInstance* site_instance,
                      RenderViewHostImpl* render_view_host,
                      RenderFrameHostDelegate* delegate,
                      RenderWidgetHostDelegate* rwh_delegate,
                      FrameTree* frame_tree,
                      FrameTreeNode* frame_tree_node,
                      int32_t routing_id,
                      int32_t widget_routing_id,
                      int flags);
  ~TestRenderFrameHost() override;

  // RenderFrameHostImpl overrides (same values, but in Test*/Mock* types)
  TestRenderViewHost* GetRenderViewHost() override;
  MockRenderProcessHost* GetProcess() override;
  TestRenderWidgetHost* GetRenderWidgetHost() override;
  void AddMessageToConsole(ConsoleMessageLevel level,
                           const std::string& message) override;

  // RenderFrameHostTester implementation.
  void InitializeRenderFrameIfNeeded() override;
  TestRenderFrameHost* AppendChild(const std::string& frame_name) override;
  void Detach() override;
  void SimulateNavigationStop() override;
  void SendNavigateWithTransition(int nav_entry_id,
                                  bool did_create_new_entry,
                                  const GURL& url,
                                  ui::PageTransition transition) override;
  void SetContentsMimeType(const std::string& mime_type) override;
  void SendBeforeUnloadACK(bool proceed) override;
  void SimulateSwapOutACK() override;
  // DEPRECATED: Use NavigationSimulator::NavigateAndCommitFromDocument().
  void NavigateAndCommitRendererInitiated(bool did_create_new_entry,
                                          const GURL& url) override;
  void SimulateFeaturePolicyHeader(
      blink::mojom::FeaturePolicyFeature feature,
      const std::vector<url::Origin>& whitelist) override;
  const std::vector<std::string>& GetConsoleMessages() override;

  void SendNavigateWithReplacement(int nav_entry_id,
                                   bool did_create_new_entry,
                                   const GURL& url);

  using ModificationCallback =
      base::Callback<void(FrameHostMsg_DidCommitProvisionalLoad_Params*)>;

  void SendNavigate(int nav_entry_id,
                    bool did_create_new_entry,
                    const GURL& url);
  void SendNavigateWithModificationCallback(
      int nav_entry_id,
      bool did_create_new_entry,
      const GURL& url,
      const ModificationCallback& callback);
  void SendNavigateWithParams(
      FrameHostMsg_DidCommitProvisionalLoad_Params* params,
      bool was_within_same_document);
  void SendNavigateWithParamsAndInterfaceProvider(
      FrameHostMsg_DidCommitProvisionalLoad_Params* params,
      service_manager::mojom::InterfaceProviderRequest request,
      bool was_within_same_document);

  // Simulates a navigation to |url| failing with the error code |error_code|.
  // DEPRECATED: use NavigationSimulator instead.
  void SimulateNavigationError(const GURL& url, int error_code);

  // Simulates the commit of an error page following a navigation failure.
  // DEPRECATED: use NavigationSimulator instead.
  void SimulateNavigationErrorPageCommit();

  // With the current navigation logic this method is a no-op.
  // Simulates a renderer-initiated navigation to |url| starting in the
  // RenderFrameHost.
  // DEPRECATED: use NavigationSimulator instead.
  void SimulateNavigationStart(const GURL& url);

  // Simulates a redirect to |new_url| for the navigation in the
  // RenderFrameHost.
  // DEPRECATED: use NavigationSimulator instead.
  void SimulateRedirect(const GURL& new_url);

  // Simulates a navigation to |url| committing in the RenderFrameHost.
  // DEPRECATED: use NavigationSimulator instead.
  void SimulateNavigationCommit(const GURL& url);

  // PlzNavigate: this method simulates receiving a BeginNavigation IPC.
  void SendRendererInitiatedNavigationRequest(const GURL& url,
                                              bool has_user_gesture);

  void DidChangeOpener(int opener_routing_id);

  void DidEnforceInsecureRequestPolicy(blink::WebInsecureRequestPolicy policy);

  // If set, navigations will appear to have cleared the history list in the
  // RenderFrame
  // (FrameHostMsg_DidCommitProvisionalLoad_Params::history_list_was_cleared).
  // False by default.
  void set_simulate_history_list_was_cleared(bool cleared) {
    simulate_history_list_was_cleared_ = cleared;
  }

  // Advances the RenderFrameHost (and through it the RenderFrameHostManager) to
  // a state where a new navigation can be committed by a renderer. Currently,
  // this simulates a BeforeUnload ACK from the renderer.
  // PlzNavigate: this simulates a BeforeUnload ACK from the renderer, and the
  // interaction with the IO thread up until the response is ready to commit.
  void PrepareForCommit();

  // Like PrepareForCommit, but with the socket address when needed.
  // TODO(clamy): Have NavigationSimulator make the relevant calls directly and
  // remove this function.
  void PrepareForCommitWithSocketAddress(
      const net::HostPortPair& socket_address);

  // This method does the same as PrepareForCommit.
  // PlzNavigate: Beyond doing the same as PrepareForCommit, this method will
  // also simulate a server redirect to |redirect_url|. If the URL is empty the
  // redirect step is ignored.
  void PrepareForCommitWithServerRedirect(const GURL& redirect_url);

  // If we are doing a cross-site navigation, this simulates the current
  // RenderFrameHost notifying that BeforeUnload has executed so the pending
  // RenderFrameHost is resumed and can navigate.
  // PlzNavigate: This simulates a BeforeUnload ACK from the renderer, and the
  // interaction with the IO thread up until the response is ready to commit.
  void PrepareForCommitIfNecessary();

  // Send a message with the sandbox flags and feature policy
  void SendFramePolicy(blink::WebSandboxFlags sandbox_flags,
                       const blink::ParsedFeaturePolicy& declared_policy);

  // Creates a WebBluetooth Service with a dummy InterfaceRequest.
  WebBluetoothServiceImpl* CreateWebBluetoothServiceForTesting();

  bool last_commit_was_error_page() const {
    return last_commit_was_error_page_;
  }

  // Exposes the interface registry to be manipulated for testing.
  service_manager::BinderRegistry& binder_registry() { return *registry_; }

  // Returns a pending InterfaceProvider request that is safe to bind to an
  // implementation, but will never receive any interface requests.
  static service_manager::mojom::InterfaceProviderRequest
  CreateStubInterfaceProviderRequest();

 private:
  void SendNavigateWithParameters(int nav_entry_id,
                                  bool did_create_new_entry,
                                  bool should_replace_entry,
                                  const GURL& url,
                                  ui::PageTransition transition,
                                  int response_code,
                                  const ModificationCallback& callback);

  void PrepareForCommitInternal(const GURL& redirect_url,
                                const net::HostPortPair& socket_address);

  // Computes the page ID for a pending navigation in this RenderFrameHost;
  int32_t ComputeNextPageID();

  // Keeps a running vector of messages sent to AddMessageToConsole.
  std::vector<std::string> console_messages_;

  TestRenderFrameHostCreationObserver child_creation_observer_;

  std::string contents_mime_type_;

  // See set_simulate_history_list_was_cleared() above.
  bool simulate_history_list_was_cleared_;

  // The last commit was for an error page.
  bool last_commit_was_error_page_;

  DISALLOW_COPY_AND_ASSIGN(TestRenderFrameHost);
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_RENDER_FRAME_HOST_H_
