// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_WEB_VIEW_TEST_PROXY_H_
#define CONTENT_SHELL_TEST_RUNNER_WEB_VIEW_TEST_PROXY_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "content/shell/test_runner/test_runner_export.h"
#include "content/shell/test_runner/web_view_test_client.h"
#include "content/shell/test_runner/web_widget_test_client.h"
#include "content/shell/test_runner/web_widget_test_proxy.h"
#include "third_party/blink/public/platform/web_drag_operation.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_screen_info.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_dom_message_event.h"
#include "third_party/blink/public/web/web_history_commit_type.h"
#include "third_party/blink/public/web/web_navigation_policy.h"
#include "third_party/blink/public/web/web_text_direction.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/public/web/web_widget_client.h"

namespace blink {
class WebDragData;
class WebImage;
class WebLocalFrame;
class WebString;
class WebView;
class WebWidget;
struct WebPoint;
struct WebWindowFeatures;
}

namespace test_runner {

class AccessibilityController;
class TestInterfaces;
class TestRunnerForSpecificView;
class TextInputController;
class WebTestDelegate;
class WebTestInterfaces;

// WebViewTestProxyBase is the "brain" of WebViewTestProxy in the sense that
// WebViewTestProxy does the bridge between RenderViewImpl and
// WebViewTestProxyBase and when it requires a behavior to be different from the
// usual, it will call WebViewTestProxyBase that implements the expected
// behavior. See WebViewTestProxy class comments for more information.
class TEST_RUNNER_EXPORT WebViewTestProxyBase : public WebWidgetTestProxyBase {
 public:
  blink::WebView* web_view() { return web_view_; }
  void set_web_view(blink::WebView* view) {
    DCHECK(view);
    DCHECK(!web_view_);
    web_view_ = view;
  }

  void set_view_test_client(
      std::unique_ptr<WebViewTestClient> view_test_client) {
    DCHECK(view_test_client);
    DCHECK(!view_test_client_);
    view_test_client_ = std::move(view_test_client);
  }

  WebTestDelegate* delegate() { return delegate_; }
  void set_delegate(WebTestDelegate* delegate) {
    DCHECK(delegate);
    DCHECK(!delegate_);
    delegate_ = delegate;
  }

  TestInterfaces* test_interfaces() { return test_interfaces_; }
  void SetInterfaces(WebTestInterfaces* web_test_interfaces);

  AccessibilityController* accessibility_controller() {
    return accessibility_controller_.get();
  }

  TestRunnerForSpecificView* view_test_runner() {
    return view_test_runner_.get();
  }

  void Reset();
  void BindTo(blink::WebLocalFrame* frame);

  void GetScreenOrientationForTesting(blink::WebScreenInfo&);

 protected:
  WebViewTestProxyBase();
  ~WebViewTestProxyBase();

  blink::WebViewClient* view_test_client() { return view_test_client_.get(); }

 private:
  TestInterfaces* test_interfaces_;
  WebTestDelegate* delegate_;
  blink::WebView* web_view_;
  blink::WebWidget* web_widget_;
  std::unique_ptr<WebViewTestClient> view_test_client_;
  std::unique_ptr<AccessibilityController> accessibility_controller_;
  std::unique_ptr<TextInputController> text_input_controller_;
  std::unique_ptr<TestRunnerForSpecificView> view_test_runner_;

  DISALLOW_COPY_AND_ASSIGN(WebViewTestProxyBase);
};

// WebViewTestProxy is used during LayoutTests and always instantiated, at time
// of writing with Base=RenderViewImpl. It does not directly inherit from it for
// layering purposes.
// The intent of that class is to wrap RenderViewImpl for tests purposes in
// order to reduce the amount of test specific code in the production code.
// WebViewTestProxy is only doing the glue between RenderViewImpl and
// WebViewTestProxyBase, that means that there is no logic living in this class
// except deciding which base class should be called (could be both).
//
// Examples of usage:
//  * when a fooClient has a mock implementation, WebViewTestProxy can override
//    the fooClient() call and have WebViewTestProxyBase return the mock
//    implementation.
//  * when a value needs to be overridden by LayoutTests, WebViewTestProxy can
//    override RenderViewImpl's getter and call a getter from
//    WebViewTestProxyBase instead. In addition, WebViewTestProxyBase will have
//    a public setter that could be called from the TestRunner.
#if defined(OS_WIN)
// WebViewTestProxy is a diamond-shaped hierarchy, with WebWidgetClient at the
// root. VS warns when we inherit the WebWidgetClient method implementations
// from RenderWidget. It's safe to ignore that warning.
#pragma warning(disable : 4250)
#endif
template <class Base, typename... Args>
class WebViewTestProxy : public Base, public WebViewTestProxyBase {
 public:
  explicit WebViewTestProxy(Args... args) : Base(args...) {}

  // WebWidgetClient implementation.
  blink::WebScreenInfo GetScreenInfo() override {
    blink::WebScreenInfo info = Base::GetScreenInfo();
    blink::WebScreenInfo test_info = widget_test_client()->GetScreenInfo();
    if (test_info.orientation_type != blink::kWebScreenOrientationUndefined) {
      info.orientation_type = test_info.orientation_type;
      info.orientation_angle = test_info.orientation_angle;
    }
    return info;
  }
  void ScheduleAnimation() override {
    widget_test_client()->ScheduleAnimation();
  }
  bool RequestPointerLock() override {
    return widget_test_client()->RequestPointerLock();
  }
  void RequestPointerUnlock() override {
    widget_test_client()->RequestPointerUnlock();
  }
  bool IsPointerLocked() override {
    return widget_test_client()->IsPointerLocked();
  }
  void DidFocus(blink::WebLocalFrame* calling_frame) override {
    view_test_client()->DidFocus(calling_frame);
    Base::DidFocus(calling_frame);
  }
  void SetToolTipText(const blink::WebString& text,
                      blink::WebTextDirection hint) override {
    widget_test_client()->SetToolTipText(text, hint);
    Base::SetToolTipText(text, hint);
  }

  // WebViewClient implementation.
  void StartDragging(blink::WebReferrerPolicy policy,
                     const blink::WebDragData& data,
                     blink::WebDragOperationsMask mask,
                     const blink::WebImage& image,
                     const blink::WebPoint& point) override {
    widget_test_client()->StartDragging(policy, data, mask, image, point);
    // Don't forward this call to Base because we don't want to do a real
    // drag-and-drop.
  }
  blink::WebView* CreateView(blink::WebLocalFrame* creator,
                             const blink::WebURLRequest& request,
                             const blink::WebWindowFeatures& features,
                             const blink::WebString& frame_name,
                             blink::WebNavigationPolicy policy,
                             bool suppress_opener,
                             blink::WebSandboxFlags sandbox_flags) override {
    if (!view_test_client()->CreateView(creator, request, features, frame_name,
                                        policy, suppress_opener, sandbox_flags))
      return nullptr;
    return Base::CreateView(creator, request, features, frame_name, policy,
                            suppress_opener, sandbox_flags);
  }
  void PrintPage(blink::WebLocalFrame* frame) override {
    view_test_client()->PrintPage(frame);
  }
  blink::WebString AcceptLanguages() override {
    return view_test_client()->AcceptLanguages();
  }

 private:
  virtual ~WebViewTestProxy() {}

  DISALLOW_COPY_AND_ASSIGN(WebViewTestProxy);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_WEB_VIEW_TEST_PROXY_H_
