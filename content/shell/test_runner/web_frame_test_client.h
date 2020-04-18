// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_WEB_FRAME_TEST_CLIENT_H_
#define CONTENT_SHELL_TEST_RUNNER_WEB_FRAME_TEST_CLIENT_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "third_party/blink/public/web/web_frame_client.h"

namespace test_runner {

class TestRunner;
class WebFrameTestProxyBase;
class WebTestDelegate;
class WebViewTestProxyBase;

// WebFrameTestClient implements WebFrameClient interface, providing behavior
// expected by tests.  WebFrameTestClient ends up used by WebFrameTestProxy
// which coordinates forwarding WebFrameClient calls either to
// WebFrameTestClient or to the product code (i.e. to RenderFrameImpl).
class WebFrameTestClient : public blink::WebFrameClient {
 public:
  // Caller has to ensure that all arguments (|delegate|,
  // |web_view_test_proxy_base_| and so forth) live longer than |this|.
  WebFrameTestClient(WebTestDelegate* delegate,
                     WebViewTestProxyBase* web_view_test_proxy_base,
                     WebFrameTestProxyBase* web_frame_test_proxy_base);

  ~WebFrameTestClient() override;

  // WebFrameClient overrides needed by WebFrameTestProxy.
  void RunModalAlertDialog(const blink::WebString& message) override;
  bool RunModalConfirmDialog(const blink::WebString& message) override;
  bool RunModalPromptDialog(const blink::WebString& message,
                            const blink::WebString& default_value,
                            blink::WebString* actual_value) override;
  bool RunModalBeforeUnloadDialog(bool is_reload) override;
  void PostAccessibilityEvent(const blink::WebAXObject& object,
                              blink::WebAXEvent event) override;
  void DidChangeSelection(bool is_selection_empty) override;
  void DidChangeContents() override;
  blink::WebPlugin* CreatePlugin(const blink::WebPluginParams& params) override;
  void ShowContextMenu(
      const blink::WebContextMenuData& context_menu_data) override;
  void DidAddMessageToConsole(const blink::WebConsoleMessage& message,
                              const blink::WebString& source_name,
                              unsigned source_line,
                              const blink::WebString& stack_trace) override;
  void DownloadURL(const blink::WebURLRequest& request,
                   mojo::ScopedMessagePipeHandle blob_url_token) override;
  void LoadErrorPage(int reason) override;
  void DidStartProvisionalLoad(blink::WebDocumentLoader* loader,
                               blink::WebURLRequest& request) override;
  void DidReceiveServerRedirectForProvisionalLoad() override;
  void DidFailProvisionalLoad(const blink::WebURLError& error,
                              blink::WebHistoryCommitType commit_type) override;
  void DidCommitProvisionalLoad(const blink::WebHistoryItem& history_item,
                                blink::WebHistoryCommitType history_type,
                                blink::WebGlobalObjectReusePolicy) override;
  void DidFinishSameDocumentNavigation(
      const blink::WebHistoryItem& history_item,
      blink::WebHistoryCommitType history_type,
      bool content_initiated) override;
  void DidReceiveTitle(const blink::WebString& title,
                       blink::WebTextDirection direction) override;
  void DidChangeIcon(blink::WebIconURL::Type icon_type) override;
  void DidFinishDocumentLoad() override;
  void DidHandleOnloadEvents() override;
  void DidFailLoad(const blink::WebURLError& error,
                   blink::WebHistoryCommitType commit_type) override;
  void DidFinishLoad() override;
  void DidStopLoading() override;
  void DidDetectXSS(const blink::WebURL& insecure_url,
                    bool did_block_entire_page) override;
  void DidDispatchPingLoader(const blink::WebURL& url) override;
  void WillSendRequest(blink::WebURLRequest& request) override;
  void DidReceiveResponse(const blink::WebURLResponse& response) override;
  blink::WebNavigationPolicy DecidePolicyForNavigation(
      const blink::WebFrameClient::NavigationPolicyInfo& info) override;
  void CheckIfAudioSinkExistsAndIsAuthorized(
      const blink::WebString& sink_id,
      blink::WebSetSinkIdCallbacks* web_callbacks) override;
  blink::WebSpeechRecognizer* SpeechRecognizer() override;
  void DidClearWindowObject() override;
  bool RunFileChooser(const blink::WebFileChooserParams& params,
                      blink::WebFileChooserCompletion* completion) override;
  blink::WebEffectiveConnectionType GetEffectiveConnectionType() override;

 private:
  TestRunner* test_runner();

  // Borrowed pointers to other parts of Layout Tests state.
  WebTestDelegate* delegate_;
  WebViewTestProxyBase* web_view_test_proxy_base_;
  WebFrameTestProxyBase* web_frame_test_proxy_base_;

  DISALLOW_COPY_AND_ASSIGN(WebFrameTestClient);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_WEB_FRAME_TEST_CLIENT_H_
