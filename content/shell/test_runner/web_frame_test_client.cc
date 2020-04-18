// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/web_frame_test_client.h"

#include <memory>

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/test/test_runner_support.h"
#include "content/shell/test_runner/accessibility_controller.h"
#include "content/shell/test_runner/event_sender.h"
#include "content/shell/test_runner/mock_screen_orientation_client.h"
#include "content/shell/test_runner/mock_web_speech_recognizer.h"
#include "content/shell/test_runner/test_common.h"
#include "content/shell/test_runner/test_interfaces.h"
#include "content/shell/test_runner/test_plugin.h"
#include "content/shell/test_runner/test_runner.h"
#include "content/shell/test_runner/web_frame_test_proxy.h"
#include "content/shell/test_runner/web_test_delegate.h"
#include "content/shell/test_runner/web_view_test_proxy.h"
#include "content/shell/test_runner/web_widget_test_proxy.h"
#include "net/base/net_errors.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_navigation_policy.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#include "third_party/blink/public/web/web_user_gesture_indicator.h"
#include "third_party/blink/public/web/web_view.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace test_runner {

namespace {

void PrintFrameDescription(WebTestDelegate* delegate,
                           blink::WebLocalFrame* frame) {
  std::string name = content::GetFrameNameForLayoutTests(frame);
  if (frame == frame->View()->MainFrame()) {
    DCHECK(name.empty());
    delegate->PrintMessage("main frame");
    return;
  }
  if (name.empty()) {
    delegate->PrintMessage("frame (anonymous)");
    return;
  }
  delegate->PrintMessage(std::string("frame \"") + name + "\"");
}

void PrintFrameuserGestureStatus(WebTestDelegate* delegate,
                                 blink::WebLocalFrame* frame,
                                 const char* msg) {
  bool is_user_gesture =
      blink::WebUserGestureIndicator::IsProcessingUserGesture(frame);
  delegate->PrintMessage(std::string("Frame with user gesture \"") +
                         (is_user_gesture ? "true" : "false") + "\"" + msg);
}

// Used to write a platform neutral file:/// URL by taking the
// filename and its directory. (e.g., converts
// "file:///tmp/foo/bar.txt" to just "bar.txt").
std::string DescriptionSuitableForTestResult(const std::string& url) {
  if (url.empty() || std::string::npos == url.find("file://"))
    return url;

  size_t pos = url.rfind('/');
  if (pos == std::string::npos || !pos)
    return "ERROR:" + url;
  pos = url.rfind('/', pos - 1);
  if (pos == std::string::npos)
    return "ERROR:" + url;

  return url.substr(pos + 1);
}

void PrintResponseDescription(WebTestDelegate* delegate,
                              const blink::WebURLResponse& response) {
  if (response.IsNull()) {
    delegate->PrintMessage("(null)");
    return;
  }
  delegate->PrintMessage(base::StringPrintf(
      "<NSURLResponse %s, http status code %d>",
      DescriptionSuitableForTestResult(response.Url().GetString().Utf8())
          .c_str(),
      response.HttpStatusCode()));
}

void BlockRequest(blink::WebURLRequest& request) {
  request.SetURL(GURL("255.255.255.255"));
}

bool IsLocalHost(const std::string& host) {
  return host == "127.0.0.1" || host == "localhost" || host == "[::1]";
}

bool IsTestHost(const std::string& host) {
  return base::EndsWith(host, ".test", base::CompareCase::INSENSITIVE_ASCII);
}

bool HostIsUsedBySomeTestsToGenerateError(const std::string& host) {
  return host == "255.255.255.255";
}

// Used to write a platform neutral file:/// URL by only taking the filename
// (e.g., converts "file:///tmp/foo.txt" to just "foo.txt").
std::string URLSuitableForTestResult(const std::string& url) {
  if (url.empty() || std::string::npos == url.find("file://"))
    return url;

  size_t pos = url.rfind('/');
  if (pos == std::string::npos) {
#ifdef WIN32
    pos = url.rfind('\\');
    if (pos == std::string::npos)
      pos = 0;
#else
    pos = 0;
#endif
  }
  std::string filename = url.substr(pos + 1);
  if (filename.empty())
    return "file:";  // A WebKit test has this in its expected output.
  return filename;
}

// WebNavigationType debugging strings taken from PolicyDelegate.mm.
const char* kLinkClickedString = "link clicked";
const char* kFormSubmittedString = "form submitted";
const char* kBackForwardString = "back/forward";
const char* kReloadString = "reload";
const char* kFormResubmittedString = "form resubmitted";
const char* kOtherString = "other";
const char* kIllegalString = "illegal value";

// Get a debugging string from a WebNavigationType.
const char* WebNavigationTypeToString(blink::WebNavigationType type) {
  switch (type) {
    case blink::kWebNavigationTypeLinkClicked:
      return kLinkClickedString;
    case blink::kWebNavigationTypeFormSubmitted:
      return kFormSubmittedString;
    case blink::kWebNavigationTypeBackForward:
      return kBackForwardString;
    case blink::kWebNavigationTypeReload:
      return kReloadString;
    case blink::kWebNavigationTypeFormResubmitted:
      return kFormResubmittedString;
    case blink::kWebNavigationTypeOther:
      return kOtherString;
  }
  return kIllegalString;
}

}  // namespace

WebFrameTestClient::WebFrameTestClient(
    WebTestDelegate* delegate,
    WebViewTestProxyBase* web_view_test_proxy_base,
    WebFrameTestProxyBase* web_frame_test_proxy_base)
    : delegate_(delegate),
      web_view_test_proxy_base_(web_view_test_proxy_base),
      web_frame_test_proxy_base_(web_frame_test_proxy_base) {
  DCHECK(delegate_);
  DCHECK(web_frame_test_proxy_base_);
  DCHECK(web_view_test_proxy_base_);
}

WebFrameTestClient::~WebFrameTestClient() {}

void WebFrameTestClient::RunModalAlertDialog(const blink::WebString& message) {
  if (!test_runner()->ShouldDumpJavaScriptDialogs())
    return;
  delegate_->PrintMessage(std::string("ALERT: ") + message.Utf8().data() +
                          "\n");
}

bool WebFrameTestClient::RunModalConfirmDialog(
    const blink::WebString& message) {
  if (!test_runner()->ShouldDumpJavaScriptDialogs())
    return true;
  delegate_->PrintMessage(std::string("CONFIRM: ") + message.Utf8().data() +
                          "\n");
  return true;
}

bool WebFrameTestClient::RunModalPromptDialog(
    const blink::WebString& message,
    const blink::WebString& default_value,
    blink::WebString* actual_value) {
  if (!test_runner()->ShouldDumpJavaScriptDialogs())
    return true;
  delegate_->PrintMessage(std::string("PROMPT: ") + message.Utf8().data() +
                          ", default text: " + default_value.Utf8().data() +
                          "\n");
  return true;
}

bool WebFrameTestClient::RunModalBeforeUnloadDialog(bool is_reload) {
  if (test_runner()->ShouldDumpJavaScriptDialogs())
    delegate_->PrintMessage(std::string("CONFIRM NAVIGATION\n"));
  return !test_runner()->shouldStayOnPageAfterHandlingBeforeUnload();
}

void WebFrameTestClient::PostAccessibilityEvent(const blink::WebAXObject& obj,
                                                blink::WebAXEvent event) {
  // Only hook the accessibility events occured during the test run.
  // This check prevents false positives in BlinkLeakDetector.
  // The pending tasks in browser/renderer message queue may trigger
  // accessibility events,
  // and AccessibilityController will hold on to their target nodes if we don't
  // ignore them here.
  if (!test_runner()->TestIsRunning())
    return;

  const char* event_name = nullptr;
  switch (event) {
    case blink::kWebAXEventActiveDescendantChanged:
      event_name = "ActiveDescendantChanged";
      break;
    case blink::kWebAXEventAriaAttributeChanged:
      event_name = "AriaAttributeChanged";
      break;
    case blink::kWebAXEventAutocorrectionOccured:
      event_name = "AutocorrectionOccured";
      break;
    case blink::kWebAXEventBlur:
      event_name = "Blur";
      break;
    case blink::kWebAXEventCheckedStateChanged:
      event_name = "CheckedStateChanged";
      break;
    case blink::kWebAXEventChildrenChanged:
      event_name = "ChildrenChanged";
      break;
    case blink::kWebAXEventClicked:
      event_name = "Clicked";
      break;
    case blink::kWebAXEventDocumentSelectionChanged:
      event_name = "DocumentSelectionChanged";
      break;
    case blink::kWebAXEventFocus:
      event_name = "Focus";
      break;
    case blink::kWebAXEventHide:
      event_name = "Hide";
      break;
    case blink::kWebAXEventHover:
      event_name = "Hover";
      break;
    case blink::kWebAXEventInvalidStatusChanged:
      event_name = "InvalidStatusChanged";
      break;
    case blink::kWebAXEventLayoutComplete:
      event_name = "LayoutComplete";
      break;
    case blink::kWebAXEventLiveRegionChanged:
      event_name = "LiveRegionChanged";
      break;
    case blink::kWebAXEventLoadComplete:
      event_name = "LoadComplete";
      break;
    case blink::kWebAXEventLocationChanged:
      event_name = "LocationChanged";
      break;
    case blink::kWebAXEventMenuListItemSelected:
      event_name = "MenuListItemSelected";
      break;
    case blink::kWebAXEventMenuListItemUnselected:
      event_name = "MenuListItemUnselected";
      break;
    case blink::kWebAXEventMenuListValueChanged:
      event_name = "MenuListValueChanged";
      break;
    case blink::kWebAXEventRowCollapsed:
      event_name = "RowCollapsed";
      break;
    case blink::kWebAXEventRowCountChanged:
      event_name = "RowCountChanged";
      break;
    case blink::kWebAXEventRowExpanded:
      event_name = "RowExpanded";
      break;
    case blink::kWebAXEventScrollPositionChanged:
      event_name = "ScrollPositionChanged";
      break;
    case blink::kWebAXEventScrolledToAnchor:
      event_name = "ScrolledToAnchor";
      break;
    case blink::kWebAXEventSelectedChildrenChanged:
      event_name = "SelectedChildrenChanged";
      break;
    case blink::kWebAXEventSelectedTextChanged:
      event_name = "SelectedTextChanged";
      break;
    case blink::kWebAXEventShow:
      event_name = "Show";
      break;
    case blink::kWebAXEventTextChanged:
      event_name = "TextChanged";
      break;
    case blink::kWebAXEventValueChanged:
      event_name = "ValueChanged";
      break;
    default:
      event_name = "Unknown";
      break;
  }

  AccessibilityController* accessibility_controller =
      web_view_test_proxy_base_->accessibility_controller();
  accessibility_controller->NotificationReceived(obj, event_name);
  if (accessibility_controller->ShouldLogAccessibilityEvents()) {
    std::string message("AccessibilityNotification - ");
    message += event_name;

    blink::WebNode node = obj.GetNode();
    if (!node.IsNull() && node.IsElementNode()) {
      blink::WebElement element = node.To<blink::WebElement>();
      if (element.HasAttribute("id")) {
        message += " - id:";
        message += element.GetAttribute("id").Utf8().data();
      }
    }

    delegate_->PrintMessage(message + "\n");
  }
}

void WebFrameTestClient::DidChangeSelection(bool is_empty_callback) {
  if (test_runner()->shouldDumpEditingCallbacks())
    delegate_->PrintMessage(
        "EDITING DELEGATE: "
        "webViewDidChangeSelection:WebViewDidChangeSelectionNotification\n");
}

void WebFrameTestClient::DidChangeContents() {
  if (test_runner()->shouldDumpEditingCallbacks())
    delegate_->PrintMessage(
        "EDITING DELEGATE: webViewDidChange:WebViewDidChangeNotification\n");
}

blink::WebPlugin* WebFrameTestClient::CreatePlugin(
    const blink::WebPluginParams& params) {
  blink::WebLocalFrame* frame = web_frame_test_proxy_base_->web_frame();
  if (TestPlugin::IsSupportedMimeType(params.mime_type))
    return TestPlugin::Create(params, delegate_, frame);
  return delegate_->CreatePluginPlaceholder(params);
}

void WebFrameTestClient::ShowContextMenu(
    const blink::WebContextMenuData& context_menu_data) {
  delegate_->GetWebWidgetTestProxyBase(web_frame_test_proxy_base_->web_frame())
      ->event_sender()
      ->SetContextMenuData(context_menu_data);
}

void WebFrameTestClient::DownloadURL(
    const blink::WebURLRequest& request,
    mojo::ScopedMessagePipeHandle blob_url_token) {
  if (test_runner()->shouldWaitUntilExternalURLLoad()) {
    delegate_->PrintMessage(std::string("Download started\n"));
    delegate_->TestFinished();
  }
}

void WebFrameTestClient::LoadErrorPage(int reason) {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    delegate_->PrintMessage(base::StringPrintf(
        "- loadErrorPage: %s\n", net::ErrorToString(reason).c_str()));
  }
}

void WebFrameTestClient::DidStartProvisionalLoad(
    blink::WebDocumentLoader* document_loader,
    blink::WebURLRequest& request) {
  // PlzNavigate
  // A provisional load notification is received when a frame navigation is
  // sent to the browser. We don't want to log it again during commit.
  if (delegate_->IsNavigationInitiatedByRenderer(request))
    return;

  test_runner()->tryToSetTopLoadingFrame(
      web_frame_test_proxy_base_->web_frame());

  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didStartProvisionalLoadForFrame\n");
  }

  if (test_runner()->shouldDumpUserGestureInFrameLoadCallbacks()) {
    PrintFrameuserGestureStatus(delegate_,
                                web_frame_test_proxy_base_->web_frame(),
                                " - in didStartProvisionalLoadForFrame\n");
  }
}

void WebFrameTestClient::DidReceiveServerRedirectForProvisionalLoad() {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(
        " - didReceiveServerRedirectForProvisionalLoadForFrame\n");
  }
}

void WebFrameTestClient::DidFailProvisionalLoad(
    const blink::WebURLError& error,
    blink::WebHistoryCommitType commit_type) {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didFailProvisionalLoadWithError\n");
  }
}

void WebFrameTestClient::DidCommitProvisionalLoad(
    const blink::WebHistoryItem& history_item,
    blink::WebHistoryCommitType history_type,
    blink::WebGlobalObjectReusePolicy) {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didCommitLoadForFrame\n");
  }
}

void WebFrameTestClient::DidFinishSameDocumentNavigation(
    const blink::WebHistoryItem& history_item,
    blink::WebHistoryCommitType history_type,
    bool content_initiated) {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didCommitLoadForFrame\n");
  }
}

void WebFrameTestClient::DidReceiveTitle(const blink::WebString& title,
                                         blink::WebTextDirection direction) {
  if (test_runner()->shouldDumpFrameLoadCallbacks() &&
      web_frame_test_proxy_base_->web_frame()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(std::string(" - didReceiveTitle: ") + title.Utf8() +
                            "\n");
  }

  if (test_runner()->shouldDumpTitleChanges())
    delegate_->PrintMessage(std::string("TITLE CHANGED: '") + title.Utf8() +
                            "'\n");
}

void WebFrameTestClient::DidChangeIcon(blink::WebIconURL::Type icon_type) {
  if (test_runner()->shouldDumpIconChanges()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(std::string(" - didChangeIcons\n"));
  }
}

void WebFrameTestClient::DidFinishDocumentLoad() {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didFinishDocumentLoadForFrame\n");
  }
}

void WebFrameTestClient::DidHandleOnloadEvents() {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didHandleOnloadEventsForFrame\n");
  }
}

void WebFrameTestClient::DidFailLoad(const blink::WebURLError& error,
                                     blink::WebHistoryCommitType commit_type) {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didFailLoadWithError\n");
  }
}

void WebFrameTestClient::DidFinishLoad() {
  if (test_runner()->shouldDumpFrameLoadCallbacks()) {
    PrintFrameDescription(delegate_, web_frame_test_proxy_base_->web_frame());
    delegate_->PrintMessage(" - didFinishLoadForFrame\n");
  }
}

void WebFrameTestClient::DidStopLoading() {
  test_runner()->tryToClearTopLoadingFrame(
      web_frame_test_proxy_base_->web_frame());
}

void WebFrameTestClient::DidDetectXSS(const blink::WebURL& insecure_url,
                                      bool did_block_entire_page) {
  if (test_runner()->shouldDumpFrameLoadCallbacks())
    delegate_->PrintMessage("didDetectXSS\n");
}

void WebFrameTestClient::DidDispatchPingLoader(const blink::WebURL& url) {
  if (test_runner()->shouldDumpPingLoaderCallbacks())
    delegate_->PrintMessage(std::string("PingLoader dispatched to '") +
                            URLDescription(url).c_str() + "'.\n");
}

void WebFrameTestClient::WillSendRequest(blink::WebURLRequest& request) {
  // PlzNavigate
  // Navigation requests initiated by the renderer will have been logged when
  // the navigation was sent to the browser. Please see
  // the RenderFrameImpl::BeginNavigation() function.
  if (delegate_->IsNavigationInitiatedByRenderer(request))
    return;
  // Need to use GURL for host() and SchemeIs()
  GURL url = request.Url();
  std::string request_url = url.possibly_invalid_spec();

  GURL main_document_url = request.SiteForCookies();

  if (test_runner()->shouldDumpResourceLoadCallbacks()) {
    delegate_->PrintMessage(DescriptionSuitableForTestResult(request_url));
    delegate_->PrintMessage(" - willSendRequest <NSURLRequest URL ");
    delegate_->PrintMessage(
        DescriptionSuitableForTestResult(request_url).c_str());
    delegate_->PrintMessage(", main document URL ");
    delegate_->PrintMessage(URLDescription(main_document_url).c_str());
    delegate_->PrintMessage(", http method ");
    delegate_->PrintMessage(request.HttpMethod().Utf8().data());
    delegate_->PrintMessage(">\n");
  }

  if (test_runner()->httpHeadersToClear()) {
    for (const std::string& header : *test_runner()->httpHeadersToClear())
      request.ClearHTTPHeaderField(blink::WebString::FromUTF8(header));
  }

  std::string host = url.host();
  if (!host.empty() &&
      (url.SchemeIs(url::kHttpScheme) || url.SchemeIs(url::kHttpsScheme))) {
    if (!IsLocalHost(host) && !IsTestHost(host) &&
        !HostIsUsedBySomeTestsToGenerateError(host) &&
        ((!main_document_url.SchemeIs(url::kHttpScheme) &&
          !main_document_url.SchemeIs(url::kHttpsScheme)) ||
         IsLocalHost(main_document_url.host())) &&
        !delegate_->AllowExternalPages()) {
      delegate_->PrintMessage(std::string("Blocked access to external URL ") +
                              request_url + "\n");
      BlockRequest(request);
      return;
    }
  }

  // Set the new substituted URL.
  request.SetURL(delegate_->RewriteLayoutTestsURL(
      request.Url().GetString().Utf8(),
      test_runner()->is_web_platform_tests_mode()));
}

void WebFrameTestClient::DidReceiveResponse(
    const blink::WebURLResponse& response) {
  if (test_runner()->shouldDumpResourceLoadCallbacks()) {
    delegate_->PrintMessage(DescriptionSuitableForTestResult(
        GURL(response.Url()).possibly_invalid_spec()));
    delegate_->PrintMessage(" - didReceiveResponse ");
    PrintResponseDescription(delegate_, response);
    delegate_->PrintMessage("\n");
  }
  if (test_runner()->shouldDumpResourceResponseMIMETypes()) {
    GURL url = response.Url();
    blink::WebString mime_type = response.MimeType();
    delegate_->PrintMessage(url.ExtractFileName());
    delegate_->PrintMessage(" has MIME type ");
    // Simulate NSURLResponse's mapping of empty/unknown MIME types to
    // application/octet-stream
    delegate_->PrintMessage(mime_type.IsEmpty() ? "application/octet-stream"
                                                : mime_type.Utf8().data());
    delegate_->PrintMessage("\n");
  }
}

void WebFrameTestClient::DidAddMessageToConsole(
    const blink::WebConsoleMessage& message,
    const blink::WebString& source_name,
    unsigned source_line,
    const blink::WebString& stack_trace) {
  if (!test_runner()->ShouldDumpConsoleMessages())
    return;
  std::string level;
  switch (message.level) {
    case blink::WebConsoleMessage::kLevelVerbose:
      level = "DEBUG";
      break;
    case blink::WebConsoleMessage::kLevelInfo:
      level = "MESSAGE";
      break;
    case blink::WebConsoleMessage::kLevelWarning:
      level = "WARNING";
      break;
    case blink::WebConsoleMessage::kLevelError:
      level = "ERROR";
      break;
    default:
      level = "MESSAGE";
  }
  std::string console_message(std::string("CONSOLE ") + level + ": ");
  if (source_line) {
    console_message += base::StringPrintf("line %d: ", source_line);
  }
  // Console messages shouldn't be included in the expected output for
  // web-platform-tests because they may create non-determinism not
  // intended by the test author. They are still included in the stderr
  // output for debug purposes.
  bool dump_to_stderr = test_runner()->is_web_platform_tests_mode();
  if (!message.text.IsEmpty()) {
    std::string new_message;
    new_message = message.text.Utf8();
    size_t file_protocol = new_message.find("file://");
    if (file_protocol != std::string::npos) {
      new_message = new_message.substr(0, file_protocol) +
                    URLSuitableForTestResult(new_message.substr(file_protocol));
    }
    console_message += new_message;
  }
  console_message += "\n";

  if (dump_to_stderr) {
    delegate_->PrintMessageToStderr(console_message);
  } else {
    delegate_->PrintMessage(console_message);
  }
}

blink::WebNavigationPolicy WebFrameTestClient::DecidePolicyForNavigation(
    const blink::WebFrameClient::NavigationPolicyInfo& info) {
  // PlzNavigate
  // Navigation requests initiated by the renderer have checked navigation
  // policy when the navigation was sent to the browser. Some layout tests
  // expect that navigation policy is only checked once.
  if (delegate_->IsNavigationInitiatedByRenderer(info.url_request))
    return info.default_policy;

  if (test_runner()->shouldDumpNavigationPolicy()) {
    delegate_->PrintMessage("Default policy for navigation to '" +
                            URLDescription(info.url_request.Url()) + "' is '" +
                            WebNavigationPolicyToString(info.default_policy) +
                            "'\n");
  }

  blink::WebNavigationPolicy result;
  if (!test_runner()->policyDelegateEnabled())
    return info.default_policy;

  delegate_->PrintMessage(
      std::string("Policy delegate: attempt to load ") +
      URLDescription(info.url_request.Url()) + " with navigation type '" +
      WebNavigationTypeToString(info.navigation_type) + "'\n");
  if (test_runner()->policyDelegateIsPermissive())
    result = blink::kWebNavigationPolicyCurrentTab;
  else
    result = blink::kWebNavigationPolicyIgnore;

  if (test_runner()->policyDelegateShouldNotifyDone()) {
    test_runner()->policyDelegateDone();
    result = blink::kWebNavigationPolicyIgnore;
  }

  return result;
}

void WebFrameTestClient::CheckIfAudioSinkExistsAndIsAuthorized(
    const blink::WebString& sink_id,
    blink::WebSetSinkIdCallbacks* web_callbacks) {
  std::unique_ptr<blink::WebSetSinkIdCallbacks> callback(web_callbacks);
  std::string device_id = sink_id.Utf8();
  if (device_id == "valid" || device_id.empty())
    callback->OnSuccess();
  else if (device_id == "unauthorized")
    callback->OnError(blink::WebSetSinkIdError::kNotAuthorized);
  else
    callback->OnError(blink::WebSetSinkIdError::kNotFound);
}

blink::WebSpeechRecognizer* WebFrameTestClient::SpeechRecognizer() {
  return test_runner()->getMockWebSpeechRecognizer();
}

void WebFrameTestClient::DidClearWindowObject() {
  blink::WebLocalFrame* frame = web_frame_test_proxy_base_->web_frame();
  web_view_test_proxy_base_->test_interfaces()->BindTo(frame);
  web_view_test_proxy_base_->BindTo(frame);
  delegate_->GetWebWidgetTestProxyBase(frame)->BindTo(frame);
}

bool WebFrameTestClient::RunFileChooser(
    const blink::WebFileChooserParams& params,
    blink::WebFileChooserCompletion* completion) {
  delegate_->PrintMessage("Mock: Opening a file chooser.\n");
  // FIXME: Add ability to set file names to a file upload control.
  return false;
}

blink::WebEffectiveConnectionType
WebFrameTestClient::GetEffectiveConnectionType() {
  return test_runner()->effective_connection_type();
}

TestRunner* WebFrameTestClient::test_runner() {
  return web_view_test_proxy_base_->test_interfaces()->GetTestRunner();
}

}  // namespace test_runner
