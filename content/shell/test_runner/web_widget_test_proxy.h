// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_WEB_WIDGET_TEST_PROXY_H_
#define CONTENT_SHELL_TEST_RUNNER_WEB_WIDGET_TEST_PROXY_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "content/shell/test_runner/test_runner_export.h"
#include "content/shell/test_runner/web_widget_test_client.h"
#include "third_party/blink/public/web/web_widget_client.h"

namespace blink {
class WebLocalFrame;
class WebString;
class WebWidget;
}

namespace test_runner {

class EventSender;
class WebViewTestProxyBase;

class TEST_RUNNER_EXPORT WebWidgetTestProxyBase {
 public:
  blink::WebWidget* web_widget() { return web_widget_; }
  void set_web_widget(blink::WebWidget* widget) {
    DCHECK(widget);
    DCHECK(!web_widget_);
    web_widget_ = widget;
  }

  void set_widget_test_client(
      std::unique_ptr<WebWidgetTestClient> widget_test_client) {
    DCHECK(widget_test_client);
    DCHECK(!widget_test_client_);
    widget_test_client_ = std::move(widget_test_client);
  }

  WebViewTestProxyBase* web_view_test_proxy_base() const {
    return web_view_test_proxy_base_;
  }
  void set_web_view_test_proxy_base(
      WebViewTestProxyBase* web_view_test_proxy_base) {
    DCHECK(web_view_test_proxy_base);
    DCHECK(!web_view_test_proxy_base_);
    web_view_test_proxy_base_ = web_view_test_proxy_base;
  }

  EventSender* event_sender() { return event_sender_.get(); }

  void Reset();
  void BindTo(blink::WebLocalFrame* frame);

 protected:
  WebWidgetTestProxyBase();
  ~WebWidgetTestProxyBase();

  blink::WebWidgetClient* widget_test_client() {
    return widget_test_client_.get();
  }

 private:
  blink::WebWidget* web_widget_;
  WebViewTestProxyBase* web_view_test_proxy_base_;
  std::unique_ptr<WebWidgetTestClient> widget_test_client_;
  std::unique_ptr<EventSender> event_sender_;

  DISALLOW_COPY_AND_ASSIGN(WebWidgetTestProxyBase);
};

// WebWidgetTestProxy is used during LayoutTests and always instantiated, at
// time of writing with Base=RenderWidget. It does not directly inherit from it
// for layering purposes.
// The intent of that class is to wrap RenderWidget for tests purposes in
// order to reduce the amount of test specific code in the production code.
// WebWidgetTestProxy is only doing the glue between RenderWidget and
// WebWidgetTestProxyBase, that means that there is no logic living in this
// class except deciding which base class should be called (could be both).
//
// Examples of usage:
//  * when a fooClient has a mock implementation, WebWidgetTestProxy can
//    override the fooClient() call and have WebWidgetTestProxyBase return the
//    mock implementation.
//  * when a value needs to be overridden by LayoutTests, WebWidgetTestProxy can
//    override RenderViewImpl's getter and call a getter from
//    WebWidgetTestProxyBase instead. In addition, WebWidgetTestProxyBase will
//    have a public setter that could be called from the TestRunner.
template <class Base, typename... Args>
class WebWidgetTestProxy : public Base, public WebWidgetTestProxyBase {
 public:
  explicit WebWidgetTestProxy(Args... args) : Base(args...) {}

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
    Base::ScheduleAnimation();
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
  void SetToolTipText(const blink::WebString& text,
                      blink::WebTextDirection hint) override {
    Base::SetToolTipText(text, hint);
    widget_test_client()->SetToolTipText(text, hint);
  }
  void StartDragging(blink::WebReferrerPolicy policy,
                     const blink::WebDragData& data,
                     blink::WebDragOperationsMask mask,
                     const blink::WebImage& image,
                     const blink::WebPoint& point) override {
    widget_test_client()->StartDragging(policy, data, mask, image, point);
    // Don't forward this call to Base because we don't want to do a real
    // drag-and-drop.
  }

 private:
  virtual ~WebWidgetTestProxy() {}

  DISALLOW_COPY_AND_ASSIGN(WebWidgetTestProxy);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_WEB_WIDGET_TEST_PROXY_H_
