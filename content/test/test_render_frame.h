// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_RENDER_FRAME_H_
#define CONTENT_TEST_TEST_RENDER_FRAME_H_

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "content/common/frame.mojom.h"
#include "content/renderer/render_frame_impl.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"

namespace blink {
class WebHistoryItem;
}

namespace content {

struct CommonNavigationParams;
class MockFrameHost;
struct RequestNavigationParams;

// A test class to use in RenderViewTests.
class TestRenderFrame : public RenderFrameImpl {
 public:
  static RenderFrameImpl* CreateTestRenderFrame(
      RenderFrameImpl::CreateParams params);
  ~TestRenderFrame() override;

  const blink::WebHistoryItem& current_history_item() {
    return current_history_item_;
  }

  // Overrides the URL in the next WebURLRequest originating from the frame.
  // This will also short-circuit browser-side navigation for main resource
  // loads, FrameLoader will always carry out the load renderer-side.
  void SetURLOverrideForNextWebURLRequest(const GURL& url);

  void WillSendRequest(blink::WebURLRequest& request) override;
  void Navigate(const CommonNavigationParams& common_params,
                const RequestNavigationParams& request_params);
  void SwapOut(int proxy_routing_id,
               bool is_loading,
               const FrameReplicationState& replicated_frame_state);
  void SetEditableSelectionOffsets(int start, int end);
  void ExtendSelectionAndDelete(int before, int after);
  void DeleteSurroundingText(int before, int after);
  void DeleteSurroundingTextInCodePoints(int before, int after);
  void CollapseSelection();
  void SetAccessibilityMode(ui::AXMode new_mode);
  void SetCompositionFromExistingText(
      int start,
      int end,
      const std::vector<ui::ImeTextSpan>& ime_text_spans);

  blink::WebNavigationPolicy DecidePolicyForNavigation(
      const blink::WebFrameClient::NavigationPolicyInfo& info) override;

  std::unique_ptr<FrameHostMsg_DidCommitProvisionalLoad_Params>
  TakeLastCommitParams();

  service_manager::mojom::InterfaceProviderRequest
  TakeLastInterfaceProviderRequest();

 private:
  explicit TestRenderFrame(RenderFrameImpl::CreateParams params);

  mojom::FrameHost* GetFrameHost() override;

  mojom::FrameInputHandler* GetFrameInputHandler();

  std::unique_ptr<MockFrameHost> mock_frame_host_;
  base::Optional<GURL> next_request_url_override_;
  mojom::FrameInputHandlerPtr frame_input_handler_;

  DISALLOW_COPY_AND_ASSIGN(TestRenderFrame);
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_RENDER_FRAME_H_
