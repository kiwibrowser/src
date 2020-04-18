// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/navigation_tracker.h"

#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/test/chromedriver/chrome/browser_info.h"
#include "chrome/test/chromedriver/chrome/devtools_client.h"
#include "chrome/test/chromedriver/chrome/javascript_dialog_manager.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/net/timeout.h"

namespace {

const char kDummyFrameName[] = "chromedriver dummy frame";
const char kDummyFrameUrl[] = "about:blank";

const char kUnreachableWebDataURL[] = "chrome-error://chromewebdata/";

const char kAutomationExtensionBackgroundPage[] =
    "chrome-extension://aapnijgdinlhnhlmodcfapnahmbfebeb/"
    "_generated_background_page.html";

Status MakeNavigationCheckFailedStatus(Status command_status) {
  if (command_status.code() == kUnexpectedAlertOpen)
    return Status(kUnexpectedAlertOpen);
  else if (command_status.code() == kTimeout)
    return Status(kTimeout);
  else
    return Status(kUnknownError, "cannot determine loading status",
                  command_status);
}

}  // namespace

NavigationTracker::NavigationTracker(
    DevToolsClient* client,
    const BrowserInfo* browser_info,
    const JavaScriptDialogManager* dialog_manager)
    : client_(client),
      loading_state_(kUnknown),
      browser_info_(browser_info),
      dialog_manager_(dialog_manager),
      dummy_execution_context_id_(0),
      load_event_fired_(true),
      timed_out_(false) {
  client_->AddListener(this);
}

NavigationTracker::NavigationTracker(
    DevToolsClient* client,
    LoadingState known_state,
    const BrowserInfo* browser_info,
    const JavaScriptDialogManager* dialog_manager)
    : client_(client),
      loading_state_(known_state),
      browser_info_(browser_info),
      dialog_manager_(dialog_manager),
      dummy_execution_context_id_(0),
      load_event_fired_(true),
      timed_out_(false) {
  client_->AddListener(this);
}

NavigationTracker::~NavigationTracker() {}

Status NavigationTracker::IsPendingNavigation(const std::string& frame_id,
                                              const Timeout* timeout,
                                              bool* is_pending) {
  if (dialog_manager_->IsDialogOpen()) {
    // The render process is paused while modal dialogs are open, so
    // Runtime.evaluate will block and time out if we attempt to call it. In
    // this case we can consider the page to have loaded, so that we return
    // control back to the test and let it dismiss the dialog.
    *is_pending = false;
    return Status(kOk);
  }

  // Some DevTools commands (e.g. Input.dispatchMouseEvent) are handled in the
  // browser process, and may cause the renderer process to start a new
  // navigation. We need to call Runtime.evaluate to force a roundtrip to the
  // renderer process, and make sure that we notice any pending navigations
  // (see crbug.com/524079).
  base::DictionaryValue params;
  params.SetString("expression", "1");
  std::unique_ptr<base::DictionaryValue> result;
  Status status = client_->SendCommandAndGetResultWithTimeout(
      "Runtime.evaluate", params, timeout, &result);
  int value = 0;
  if (status.code() == kDisconnected) {
    // If we receive a kDisconnected status code from Runtime.evaluate, don't
    // wait for pending navigations to complete, since we won't see any more
    // events from it until we reconnect.
    *is_pending = false;
    return Status(kOk);
  } else if (status.code() == kUnexpectedAlertOpen) {
    // The JS event loop is paused while modal dialogs are open, so return
    // control to the test so that it can dismiss the dialog.
    *is_pending = false;
    return Status(kOk);
  } else if (status.IsError() ||
             !result->GetInteger("result.value", &value) ||
             value != 1) {
    return MakeNavigationCheckFailedStatus(status);
  }

  if (loading_state_ == kUnknown) {
    // In the case that a http request is sent to server to fetch the page
    // content and the server hasn't responded at all, a dummy page is created
    // for the new window. In such case, the baseURL will be 'about:blank'.
    base::DictionaryValue empty_params;
    std::unique_ptr<base::DictionaryValue> result;
    Status status = client_->SendCommandAndGetResultWithTimeout(
        "DOM.getDocument", empty_params, timeout, &result);
    std::string base_url;
    std::string doc_url;
    if (status.IsError() || !result->GetString("root.baseURL", &base_url) ||
        !result->GetString("root.documentURL", &doc_url))
      return MakeNavigationCheckFailedStatus(status);

    if (doc_url != "about:blank" && base_url == "about:blank") {
      *is_pending = true;
      loading_state_ = kLoading;
      return Status(kOk);
    }

    // If we're loading the ChromeDriver automation extension background page,
    // look for a known function to determine the loading status.
    if (base_url == kAutomationExtensionBackgroundPage) {
      bool function_exists = false;
      status = CheckFunctionExists(timeout, &function_exists);
      if (status.IsError())
        return MakeNavigationCheckFailedStatus(status);
      loading_state_ = function_exists ? kNotLoading : kLoading;
    }

    // If the loading state is unknown (which happens after first connecting),
    // force loading to start and set the state to loading. This will cause a
    // frame start event to be received, and the frame stop event will not be
    // received until all frames are loaded.  Loading is forced to start by
    // attaching a temporary iframe.
    const std::string kStartLoadingIfMainFrameNotLoading = base::StringPrintf(
        "var frame = document.createElement('iframe');"
        "frame.name = '%s';"
        "frame.src = '%s';"
        "document.body.appendChild(frame);"
        "window.setTimeout(function() {"
        "  document.body.removeChild(frame);"
        "}, 0);",
        kDummyFrameName, kDummyFrameUrl);
    base::DictionaryValue params;
    params.SetString("expression", kStartLoadingIfMainFrameNotLoading);
    status = client_->SendCommandAndGetResultWithTimeout(
        "Runtime.evaluate", params, timeout, &result);
    if (status.IsError())
      return MakeNavigationCheckFailedStatus(status);

    // Between the time the JavaScript is evaluated and
    // SendCommandAndGetResult returns, OnEvent may have received info about
    // the loading state.  This is only possible during a nested command. Only
    // set the loading state if the loading state is still unknown.
    if (loading_state_ == kUnknown)
      loading_state_ = kLoading;
  }
  *is_pending = loading_state_ == kLoading;

  if (frame_id.empty()) {
    *is_pending |= scheduled_frame_set_.size() > 0;
    *is_pending |= pending_frame_set_.size() > 0;
  } else {
    *is_pending |= scheduled_frame_set_.count(frame_id) > 0;
    *is_pending |= pending_frame_set_.count(frame_id) > 0;
  }
  return Status(kOk);
}

Status NavigationTracker::CheckFunctionExists(const Timeout* timeout,
                                              bool* exists) {
  base::DictionaryValue params;
  params.SetString("expression", "typeof(getWindowInfo)");
  std::unique_ptr<base::DictionaryValue> result;
  Status status = client_->SendCommandAndGetResultWithTimeout(
      "Runtime.evaluate", params, timeout, &result);
  std::string type;
  if (status.IsError() || !result->GetString("result.value", &type))
    return MakeNavigationCheckFailedStatus(status);
  *exists = type == "function";
  return Status(kOk);
}

void NavigationTracker::set_timed_out(bool timed_out) {
  timed_out_ = timed_out;
}

bool NavigationTracker::IsNonBlocking() const {
  return false;
}

Status NavigationTracker::OnConnected(DevToolsClient* client) {
  ResetLoadingState(kUnknown);

  // Enable page domain notifications to allow tracking navigation state.
  base::DictionaryValue empty_params;
  return client_->SendCommand("Page.enable", empty_params);
}

Status NavigationTracker::OnEvent(DevToolsClient* client,
                                  const std::string& method,
                                  const base::DictionaryValue& params) {
  if (method == "Page.frameStartedLoading") {
    std::string frame_id;
    if (!params.GetString("frameId", &frame_id))
      return Status(kUnknownError, "missing or invalid 'frameId'");
    pending_frame_set_.insert(frame_id);
    loading_state_ = kLoading;

    if (browser_info_->major_version >= 63 &&
        browser_info_->major_version < 67) {
      // Check if the document is really loading.
      base::DictionaryValue params;
      params.SetString("expression", "document.readyState");
      std::unique_ptr<base::DictionaryValue> result;
      Status status =
          client_->SendCommandAndGetResult("Runtime.evaluate", params, &result);
      std::string value;
      if (status.IsError() || !result->GetString("result.value", &value)) {
        LOG(ERROR) << "Unable to retrieve document state " << status.message();
        return status;
      }
      if (value == "complete") {
        pending_frame_set_.erase(frame_id);
        loading_state_ = kNotLoading;
      }
    }
  } else if (method == "Page.frameStoppedLoading") {
    std::string frame_id;
    if (!params.GetString("frameId", &frame_id))
      return Status(kUnknownError, "missing or invalid 'frameId'");

    scheduled_frame_set_.erase(frame_id);
    pending_frame_set_.erase(frame_id);
    if (pending_frame_set_.empty() &&
        (load_event_fired_ || timed_out_ || execution_context_set_.empty()))
      loading_state_ = kNotLoading;
  } else if (method == "Page.frameScheduledNavigation") {
    double delay;
    if (!params.GetDouble("delay", &delay))
      return Status(kUnknownError, "missing or invalid 'delay'");

    std::string frame_id;
    if (!params.GetString("frameId", &frame_id))
      return Status(kUnknownError, "missing or invalid 'frameId'");

    // WebDriver spec says to ignore redirects over 1s.
    if (delay > 1)
      return Status(kOk);
    scheduled_frame_set_.insert(frame_id);

    // A normal Page.loadEventFired event isn't expected after a scheduled
    // navigation, so set load_event_fired_ flag.
    load_event_fired_ = true;
  } else if (method == "Page.frameClearedScheduledNavigation") {
    std::string frame_id;
    if (!params.GetString("frameId", &frame_id))
      return Status(kUnknownError, "missing or invalid 'frameId'");

    scheduled_frame_set_.erase(frame_id);
  } else if (method == "Page.frameNavigated") {
    // Note: in some cases Page.frameNavigated may be received for subframes
    // without a frameStoppedLoading (for example cnn.com).

    const base::Value* unused_value;
    if (!params.Get("frame.parentId", &unused_value)) {
      // Discard pending and scheduled frames, except for the root frame,
      // which just navigated (and which we should consider pending until we
      // receive a Page.frameStoppedLoading event for it).
      std::string frame_id;
      if (!params.GetString("frame.id", &frame_id))
        return Status(kUnknownError, "missing or invalid 'frame.id'");
      bool frame_was_pending = pending_frame_set_.count(frame_id) > 0;
      pending_frame_set_.clear();
      scheduled_frame_set_.clear();
      if (frame_was_pending)
        pending_frame_set_.insert(frame_id);
      // If the URL indicates that the web page is unreachable (the sad tab
      // page) then discard all pending navigations.
      std::string frame_url;
      if (!params.GetString("frame.url", &frame_url))
        return Status(kUnknownError, "missing or invalid 'frame.url'");
      if (frame_url == kUnreachableWebDataURL)
        pending_frame_set_.clear();
    } else {
      // If a child frame just navigated, check if it is the dummy frame that
      // was attached by IsPendingNavigation(). We don't want to track execution
      // contexts created and destroyed for this dummy frame.
      std::string name;
      if (!params.GetString("frame.name", &name))
        // https://bugs.chromium.org/p/chromium/issues/detail?id=823579
        // OOPIF frames might not have names. Ignore them.
        return Status(kOk);
      std::string url;
      if (!params.GetString("frame.url", &url))
        return Status(kUnknownError, "missing or invalid 'frame.url'");
      if (name == kDummyFrameName && url == kDummyFrameUrl)
        params.GetString("frame.id", &dummy_frame_id_);
    }
  } else if (method == "Runtime.executionContextsCleared") {
    execution_context_set_.clear();
    load_event_fired_ = false;
    // As of crrev.com/382211, DevTools sends an executionContextsCleared
    // event right before the first execution context is created, but after
    // Page.loadEventFired. Set the loading state to loading, but do not
    // clear the pending and scheduled frame sets, since they may contain
    // frames that we're still waiting for.
    loading_state_ = kLoading;
  } else if (method == "Runtime.executionContextCreated") {
    int execution_context_id;
    if (!params.GetInteger("context.id", &execution_context_id))
      return Status(kUnknownError, "missing or invalid 'context.id'");
    std::string frame_id;
    if (!params.GetString("context.auxData.frameId", &frame_id)) {
      return Status(kUnknownError,
                    "missing or invalid 'context.auxData.frameId'");
    }
    if (frame_id == dummy_frame_id_)
      dummy_execution_context_id_ = execution_context_id;
    else
      execution_context_set_.insert(execution_context_id);
  } else if (method == "Runtime.executionContextDestroyed") {
    int execution_context_id;
    if (!params.GetInteger("executionContextId", &execution_context_id))
      return Status(kUnknownError, "missing or invalid 'context.id'");
    execution_context_set_.erase(execution_context_id);
    if (execution_context_id != dummy_execution_context_id_) {
      if (execution_context_set_.empty()) {
        loading_state_ = kLoading;
        load_event_fired_ = false;
        dummy_frame_id_ = std::string();
        dummy_execution_context_id_ = 0;
      }
    }
  } else if (method == "Page.loadEventFired") {
    load_event_fired_ = true;
  } else if (method == "Inspector.targetCrashed") {
    ResetLoadingState(kNotLoading);
  }
  return Status(kOk);
}

Status NavigationTracker::OnCommandSuccess(
    DevToolsClient* client,
    const std::string& method,
    const base::DictionaryValue& result,
    const Timeout& command_timeout) {
  // Check for start of navigation. In some case response to navigate is delayed
  // until after the command has already timed out, in which case it has already
  // been cancelled or will be cancelled soon, and should be ignored.
  if ((method == "Page.navigate" || method == "Page.navigateToHistoryEntry") &&
      loading_state_ != kLoading && !command_timeout.IsExpired()) {
    // At this point the browser has initiated the navigation, but besides that,
    // it is unknown what will happen.
    //
    // There are a few cases (perhaps more):
    // 1 The RenderFrameHost has already queued FrameMsg_Navigate and loading
    //   will start shortly.
    // 2 The RenderFrameHost has already queued FrameMsg_Navigate and loading
    //   will never start because it is just an in-page fragment navigation.
    // 3 The RenderFrameHost is suspended and hasn't queued FrameMsg_Navigate
    //   yet. This happens for cross-site navigations. The RenderFrameHost
    //   will not queue FrameMsg_Navigate until it is ready to unload the
    //   previous page (after running unload handlers and such).
    // TODO(nasko): Revisit case 3, since now unload handlers are run in the
    // background. http://crbug.com/323528.
    //
    // To determine whether a load is expected, do a round trip to the
    // renderer to ask what the URL is.
    // If case #1, by the time the command returns, the frame started to load
    // event will also have been received, since the DevTools command will
    // be queued behind FrameMsg_Navigate.
    // If case #2, by the time the command returns, the navigation will
    // have already happened, although no frame start/stop events will have
    // been received.
    // If case #3, the URL will be blank if the navigation hasn't been started
    // yet. In that case, expect a load to happen in the future.
    loading_state_ = kUnknown;
    base::DictionaryValue params;
    params.SetString("expression", "document.URL");
    std::unique_ptr<base::DictionaryValue> result;
    Status status = client_->SendCommandAndGetResultWithTimeout(
        "Runtime.evaluate", params, &command_timeout, &result);
    std::string url;
    if (status.IsError() || !result->GetString("result.value", &url))
      return MakeNavigationCheckFailedStatus(status);
    if (loading_state_ == kUnknown && url.empty())
      loading_state_ = kLoading;
  }
  return Status(kOk);
}

void NavigationTracker::ResetLoadingState(LoadingState loading_state) {
  loading_state_ = loading_state;
  pending_frame_set_.clear();
  scheduled_frame_set_.clear();
}
