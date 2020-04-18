/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"

#include <utility>

#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_thread.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_tree_scope_type.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/exported/web_remote_frame_impl.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {
namespace FrameTestHelpers {

namespace {

// The frame test helpers coordinate frame loads in a carefully choreographed
// dance. Since the parser is threaded, simply spinning the run loop once is not
// enough to ensure completion of a load. Instead, the following pattern is
// used to ensure that tests see the final state:
// 1. Starts a load.
// 2. Enter the run loop.
// 3. Posted task triggers the load, and starts pumping pending resource
//    requests using runServeAsyncRequestsTask().
// 4. TestWebFrameClient watches for didStartLoading/didStopLoading calls,
//    keeping track of how many loads it thinks are in flight.
// 5. While runServeAsyncRequestsTask() observes TestWebFrameClient to still
//    have loads in progress, it posts itself back to the run loop.
// 6. When runServeAsyncRequestsTask() notices there are no more loads in
//    progress, it exits the run loop.
// 7. At this point, all parsing, resource loads, and layout should be finished.

void RunServeAsyncRequestsTask() {
  // TODO(kinuko,toyoshim): Create a mock factory and use it instead of
  // getting the platform's one. (crbug.com/751425)
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  if (TestWebFrameClient::IsLoading()) {
    Platform::Current()->CurrentThread()->GetTaskRunner()->PostTask(
        FROM_HERE, WTF::Bind(&RunServeAsyncRequestsTask));
  } else {
    test::ExitRunLoop();
  }
}

// Helper to create a default test client if the supplied client pointer is
// null.
template <typename T>
std::unique_ptr<T> CreateDefaultClientIfNeeded(T*& client) {
  if (client)
    return nullptr;
  auto owned_client = std::make_unique<T>();
  client = owned_client.get();
  return owned_client;
}

}  // namespace

void LoadFrame(WebLocalFrame* frame, const std::string& url) {
  WebURLRequest url_request(URLTestHelpers::ToKURL(url));
  if (url_request.Url().ProtocolIs("javascript")) {
    frame->LoadJavaScriptURL(url_request.Url());
  } else {
    frame->LoadRequest(url_request);
  }
  PumpPendingRequestsForFrameToLoad(frame);
}

void LoadHTMLString(WebLocalFrame* frame,
                    const std::string& html,
                    const WebURL& base_url) {
  frame->LoadHTMLString(WebData(html.data(), html.size()), base_url);
  PumpPendingRequestsForFrameToLoad(frame);
}

void LoadHistoryItem(WebLocalFrame* frame,
                     const WebHistoryItem& item,
                     WebHistoryLoadType load_type,
                     mojom::FetchCacheMode cache_mode) {
  HistoryItem* history_item = item;
  frame->CommitNavigation(
      WrappedResourceRequest(history_item->GenerateResourceRequest(cache_mode)),
      WebFrameLoadType::kBackForward, item, kWebHistoryDifferentDocumentLoad,
      /*is_client_redirect=*/false, base::UnguessableToken::Create());
  PumpPendingRequestsForFrameToLoad(frame);
}

void ReloadFrame(WebLocalFrame* frame) {
  frame->Reload(WebFrameLoadType::kReload);
  PumpPendingRequestsForFrameToLoad(frame);
}

void ReloadFrameBypassingCache(WebLocalFrame* frame) {
  frame->Reload(WebFrameLoadType::kReloadBypassingCache);
  PumpPendingRequestsForFrameToLoad(frame);
}

void PumpPendingRequestsForFrameToLoad(WebFrame* frame) {
  Platform::Current()->CurrentThread()->GetTaskRunner()->PostTask(
      FROM_HERE, WTF::Bind(&RunServeAsyncRequestsTask));
  test::EnterRunLoop();
}

WebMouseEvent CreateMouseEvent(WebInputEvent::Type type,
                               WebMouseEvent::Button button,
                               const IntPoint& point,
                               int modifiers) {
  WebMouseEvent result(type, modifiers,
                       WebInputEvent::GetStaticTimeStampForTests());
  result.pointer_type = WebPointerProperties::PointerType::kMouse;
  result.SetPositionInWidget(point.X(), point.Y());
  result.SetPositionInScreen(point.X(), point.Y());
  result.button = button;
  result.click_count = 1;
  return result;
}

WebLocalFrameImpl* CreateLocalChild(WebLocalFrame& parent,
                                    WebTreeScopeType scope,
                                    TestWebFrameClient* client) {
  auto owned_client = CreateDefaultClientIfNeeded(client);
  WebLocalFrameImpl* frame =
      ToWebLocalFrameImpl(parent.CreateLocalChild(scope, client, nullptr));
  client->Bind(frame, std::move(owned_client));
  return frame;
}

WebLocalFrameImpl* CreateLocalChild(
    WebLocalFrame& parent,
    WebTreeScopeType scope,
    std::unique_ptr<TestWebFrameClient> self_owned) {
  DCHECK(self_owned);
  TestWebFrameClient* client = self_owned.get();
  WebLocalFrameImpl* frame =
      ToWebLocalFrameImpl(parent.CreateLocalChild(scope, client, nullptr));
  client->Bind(frame, std::move(self_owned));
  return frame;
}

WebLocalFrameImpl* CreateProvisional(WebRemoteFrame& old_frame,
                                     TestWebFrameClient* client) {
  auto owned_client = CreateDefaultClientIfNeeded(client);
  WebLocalFrameImpl* frame =
      ToWebLocalFrameImpl(WebLocalFrame::CreateProvisional(
          client, nullptr, &old_frame, WebSandboxFlags::kNone,
          ParsedFeaturePolicy()));
  client->Bind(frame, std::move(owned_client));
  // Create a local root, if necessary.
  std::unique_ptr<TestWebWidgetClient> owned_widget_client;
  if (!frame->Parent()) {
    // TODO(dcheng): The main frame widget currently has a special case.
    // Eliminate this once WebView is no longer a WebWidget.
    owned_widget_client = std::make_unique<TestWebViewWidgetClient>(
        *static_cast<TestWebViewClient*>(frame->ViewImpl()->Client()));
  } else if (frame->Parent()->IsWebRemoteFrame()) {
    owned_widget_client = std::make_unique<TestWebWidgetClient>();
  }
  if (owned_widget_client) {
    WebFrameWidget::Create(owned_widget_client.get(), frame);
    // Set an initial size for subframes.
    if (frame->Parent())
      frame->FrameWidget()->Resize(WebSize());
    client->BindWidgetClient(std::move(owned_widget_client));
  }
  return frame;
}

WebRemoteFrameImpl* CreateRemote(TestWebRemoteFrameClient* client) {
  auto owned_client = CreateDefaultClientIfNeeded(client);
  auto* frame = WebRemoteFrameImpl::Create(WebTreeScopeType::kDocument, client);
  client->Bind(frame, std::move(owned_client));
  return frame;
}

WebLocalFrameImpl* CreateLocalChild(WebRemoteFrame& parent,
                                    const WebString& name,
                                    const WebFrameOwnerProperties& properties,
                                    WebFrame* previous_sibling,
                                    TestWebFrameClient* client,
                                    TestWebWidgetClient* widget_client) {
  auto owned_client = CreateDefaultClientIfNeeded(client);
  auto* frame = ToWebLocalFrameImpl(parent.CreateLocalChild(
      WebTreeScopeType::kDocument, name, WebSandboxFlags::kNone, client,
      nullptr, previous_sibling, ParsedFeaturePolicy(), properties, nullptr));
  client->Bind(frame, std::move(owned_client));

  auto owned_widget_client = CreateDefaultClientIfNeeded(widget_client);
  WebFrameWidget::Create(widget_client, frame);
  // Set an initial size for subframes.
  if (frame->Parent())
    frame->FrameWidget()->Resize(WebSize());
  client->BindWidgetClient(std::move(owned_widget_client));
  return frame;
}

WebRemoteFrameImpl* CreateRemoteChild(
    WebRemoteFrame& parent,
    const WebString& name,
    scoped_refptr<SecurityOrigin> security_origin,
    TestWebRemoteFrameClient* client) {
  auto owned_client = CreateDefaultClientIfNeeded(client);
  auto* frame = ToWebRemoteFrameImpl(parent.CreateRemoteChild(
      WebTreeScopeType::kDocument, name, WebSandboxFlags::kNone,
      ParsedFeaturePolicy(), client, nullptr));
  client->Bind(frame, std::move(owned_client));
  if (!security_origin)
    security_origin = SecurityOrigin::CreateUnique();
  frame->GetFrame()->GetSecurityContext()->SetReplicatedOrigin(
      std::move(security_origin));
  return frame;
}

WebViewHelper::WebViewHelper() : web_view_(nullptr) {}

WebViewHelper::~WebViewHelper() {
  Reset();
}

WebViewImpl* WebViewHelper::InitializeWithOpener(
    WebFrame* opener,
    TestWebFrameClient* web_frame_client,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client,
    void (*update_settings_func)(WebSettings*)) {
  Reset();

  InitializeWebView(web_view_client, opener ? opener->View() : nullptr);
  if (update_settings_func)
    update_settings_func(web_view_->GetSettings());

  auto owned_web_frame_client = CreateDefaultClientIfNeeded(web_frame_client);
  WebLocalFrame* frame = WebLocalFrame::CreateMainFrame(
      web_view_, web_frame_client, nullptr, opener);
  web_frame_client->Bind(frame, std::move(owned_web_frame_client));

  // TODO(dcheng): The main frame widget currently has a special case.
  // Eliminate this once WebView is no longer a WebWidget.
  std::unique_ptr<TestWebWidgetClient> owned_web_widget_client;
  if (!web_widget_client) {
    owned_web_widget_client =
        std::make_unique<TestWebViewWidgetClient>(*test_web_view_client_);
    web_widget_client = owned_web_widget_client.get();
  }
  blink::WebFrameWidget::Create(web_widget_client, frame);
  // Set an initial size for subframes.
  if (frame->Parent())
    frame->FrameWidget()->Resize(WebSize());
  web_frame_client->BindWidgetClient(std::move(owned_web_widget_client));

  return web_view_;
}

WebViewImpl* WebViewHelper::Initialize(
    TestWebFrameClient* web_frame_client,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client,
    void (*update_settings_func)(WebSettings*)) {
  return InitializeWithOpener(nullptr, web_frame_client, web_view_client,
                              web_widget_client, update_settings_func);
}

WebViewImpl* WebViewHelper::InitializeAndLoad(
    const std::string& url,
    TestWebFrameClient* web_frame_client,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client,
    void (*update_settings_func)(WebSettings*)) {
  Initialize(web_frame_client, web_view_client, web_widget_client,
             update_settings_func);

  LoadFrame(GetWebView()->MainFrameImpl(), url);

  return GetWebView();
}

WebViewImpl* WebViewHelper::InitializeRemote(
    TestWebRemoteFrameClient* web_remote_frame_client,
    scoped_refptr<SecurityOrigin> security_origin,
    TestWebViewClient* web_view_client) {
  Reset();

  InitializeWebView(web_view_client, nullptr);

  auto owned_web_remote_frame_client =
      CreateDefaultClientIfNeeded(web_remote_frame_client);
  WebRemoteFrameImpl* frame = WebRemoteFrameImpl::CreateMainFrame(
      web_view_, web_remote_frame_client, nullptr);
  web_remote_frame_client->Bind(frame,
                                std::move(owned_web_remote_frame_client));
  if (!security_origin)
    security_origin = SecurityOrigin::CreateUnique();
  frame->GetFrame()->GetSecurityContext()->SetReplicatedOrigin(
      std::move(security_origin));
  return web_view_;
}

void WebViewHelper::LoadAhem() {
  LocalFrame* local_frame =
      ToLocalFrame(WebFrame::ToCoreFrame(*LocalMainFrame()));
  DCHECK(local_frame);
  RenderingTest::LoadAhem(*local_frame);
}

void WebViewHelper::Reset() {
  if (web_view_) {
    DCHECK(!TestWebFrameClient::IsLoading());
    web_view_->Close();
    web_view_ = nullptr;
  }
}

WebLocalFrameImpl* WebViewHelper::LocalMainFrame() const {
  return ToWebLocalFrameImpl(web_view_->MainFrame());
}

WebRemoteFrameImpl* WebViewHelper::RemoteMainFrame() const {
  return ToWebRemoteFrameImpl(web_view_->MainFrame());
}

void WebViewHelper::SetViewportSize(const WebSize& viewport_size) {
  test_web_view_client_->GetLayerTreeViewForTesting()->SetViewportSize(
      viewport_size);
}

void WebViewHelper::Resize(WebSize size) {
  test_web_view_client_->ClearAnimationScheduled();
  GetWebView()->Resize(size);
  EXPECT_FALSE(test_web_view_client_->AnimationScheduled());
  test_web_view_client_->ClearAnimationScheduled();
}

void WebViewHelper::InitializeWebView(TestWebViewClient* web_view_client,
                                      class WebView* opener) {
  owned_test_web_view_client_ = CreateDefaultClientIfNeeded(web_view_client);
  web_view_ = static_cast<WebViewImpl*>(WebView::Create(
      web_view_client, mojom::PageVisibilityState::kVisible, opener));
  web_view_->GetSettings()->SetJavaScriptEnabled(true);
  web_view_->GetSettings()->SetPluginsEnabled(true);
  // Enable (mocked) network loads of image URLs, as this simplifies
  // the completion of resource loads upon test shutdown & helps avoid
  // dormant loads trigger Resource leaks for image loads.
  //
  // Consequently, all external image resources must be mocked.
  web_view_->GetSettings()->SetLoadsImagesAutomatically(true);
  web_view_->SetDeviceScaleFactor(
      web_view_client->GetScreenInfo().device_scale_factor);
  web_view_->SetDefaultPageScaleLimits(1, 4);

  test_web_view_client_ = web_view_client;
}

int TestWebFrameClient::loads_in_progress_ = 0;

TestWebFrameClient::TestWebFrameClient()
    : interface_provider_(new service_manager::InterfaceProvider()) {}

void TestWebFrameClient::Bind(WebLocalFrame* frame,
                              std::unique_ptr<TestWebFrameClient> self_owned) {
  DCHECK(!frame_);
  DCHECK(!self_owned || self_owned.get() == this);
  frame_ = frame;
  self_owned_ = std::move(self_owned);
}

void TestWebFrameClient::BindWidgetClient(
    std::unique_ptr<TestWebWidgetClient> client) {
  DCHECK(!owned_widget_client_);
  owned_widget_client_ = std::move(client);
}

void TestWebFrameClient::FrameDetached(DetachType type) {
  if (frame_->FrameWidget()) {
    frame_->FrameWidget()->WillCloseLayerTreeView();
    frame_->FrameWidget()->Close();
  }

  owned_widget_client_.reset();
  frame_->Close();
  self_owned_.reset();
}

WebLocalFrame* TestWebFrameClient::CreateChildFrame(
    WebLocalFrame* parent,
    WebTreeScopeType scope,
    const WebString& name,
    const WebString& fallback_name,
    WebSandboxFlags sandbox_flags,
    const ParsedFeaturePolicy& container_policy,
    const WebFrameOwnerProperties& frame_owner_properties) {
  return CreateLocalChild(*parent, scope);
}

void TestWebFrameClient::DidStartLoading(bool) {
  ++loads_in_progress_;
}

void TestWebFrameClient::DidStopLoading() {
  DCHECK_GT(loads_in_progress_, 0);
  --loads_in_progress_;
}

void TestWebFrameClient::DidCreateDocumentLoader(
    WebDocumentLoader* document_loader) {
  base::TimeTicks redirect_start;
  base::TimeTicks redirect_end;
  base::TimeTicks fetch_start = base::TimeTicks::Now();
  bool has_redirect = false;
  document_loader->UpdateNavigation(redirect_start, redirect_end, fetch_start,
                                    has_redirect);
}

TestWebRemoteFrameClient::TestWebRemoteFrameClient() = default;

void TestWebRemoteFrameClient::Bind(
    WebRemoteFrame* frame,
    std::unique_ptr<TestWebRemoteFrameClient> self_owned) {
  DCHECK(!frame_);
  DCHECK(!self_owned || self_owned.get() == this);
  frame_ = frame;
  self_owned_ = std::move(self_owned);
}

void TestWebRemoteFrameClient::FrameDetached(DetachType type) {
  frame_->Close();
  self_owned_.reset();
}

WebLayerTreeViewImplForTesting*
TestWebViewClient::GetLayerTreeViewForTesting() {
  return layer_tree_view_.get();
}

WebLayerTreeView* TestWebViewClient::InitializeLayerTreeView() {
  layer_tree_view_ = std::make_unique<WebLayerTreeViewImplForTesting>();
  return layer_tree_view_.get();
}

WebLayerTreeView* TestWebViewWidgetClient::InitializeLayerTreeView() {
  return test_web_view_client_.InitializeLayerTreeView();
}

WebLayerTreeView* TestWebWidgetClient::InitializeLayerTreeView() {
  layer_tree_view_ = std::make_unique<WebLayerTreeViewImplForTesting>();
  return layer_tree_view_.get();
}

void TestWebViewWidgetClient::ScheduleAnimation() {
  test_web_view_client_.ScheduleAnimation();
}

void TestWebViewWidgetClient::DidMeaningfulLayout(
    WebMeaningfulLayout layout_type) {
  test_web_view_client_.DidMeaningfulLayout(layout_type);
}

}  // namespace FrameTestHelpers
}  // namespace blink
