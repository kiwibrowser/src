/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/web/web_hit_test_result.h"

#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class WebHitTestResultPrivate
    : public GarbageCollectedFinalized<WebHitTestResultPrivate> {
 public:
  static WebHitTestResultPrivate* Create(const HitTestResult&);
  static WebHitTestResultPrivate* Create(const WebHitTestResultPrivate&);
  void Trace(blink::Visitor* visitor) { visitor->Trace(result_); }
  const HitTestResult& Result() const { return result_; }

 private:
  WebHitTestResultPrivate(const HitTestResult&);
  WebHitTestResultPrivate(const WebHitTestResultPrivate&);

  HitTestResult result_;
};

inline WebHitTestResultPrivate::WebHitTestResultPrivate(
    const HitTestResult& result)
    : result_(result) {}

inline WebHitTestResultPrivate::WebHitTestResultPrivate(
    const WebHitTestResultPrivate& result)
    : result_(result.result_) {}

WebHitTestResultPrivate* WebHitTestResultPrivate::Create(
    const HitTestResult& result) {
  return new WebHitTestResultPrivate(result);
}

WebHitTestResultPrivate* WebHitTestResultPrivate::Create(
    const WebHitTestResultPrivate& result) {
  return new WebHitTestResultPrivate(result);
}

WebNode WebHitTestResult::GetNode() const {
  return WebNode(private_->Result().InnerNode());
}

WebPoint WebHitTestResult::LocalPoint() const {
  return RoundedIntPoint(private_->Result().LocalPoint());
}

WebElement WebHitTestResult::UrlElement() const {
  return WebElement(private_->Result().URLElement());
}

WebURL WebHitTestResult::AbsoluteImageURL() const {
  return private_->Result().AbsoluteImageURL();
}

WebURL WebHitTestResult::AbsoluteLinkURL() const {
  return private_->Result().AbsoluteLinkURL();
}

bool WebHitTestResult::IsContentEditable() const {
  return private_->Result().IsContentEditable();
}

WebHitTestResult::WebHitTestResult(const HitTestResult& result)
    : private_(WebHitTestResultPrivate::Create(result)) {}

WebHitTestResult& WebHitTestResult::operator=(const HitTestResult& result) {
  private_ = WebHitTestResultPrivate::Create(result);
  return *this;
}

bool WebHitTestResult::IsNull() const {
  return !private_.Get();
}

void WebHitTestResult::Assign(const WebHitTestResult& info) {
  if (info.IsNull())
    private_.Reset();
  else
    private_ = WebHitTestResultPrivate::Create(*info.private_.Get());
}

void WebHitTestResult::Reset() {
  private_.Reset();
}

}  // namespace blink
