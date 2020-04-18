// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORT_H_

#include "third_party/blink/renderer/core/frame/report_body.h"

namespace blink {

class CORE_EXPORT Report : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  Report(const String& type, const String& url, ReportBody* body)
      : type_(type), url_(url), body_(body) {}

  ~Report() override = default;

  String type() const { return type_; }
  String url() const { return url_; }
  ReportBody* body() const { return body_; }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(body_);
    ScriptWrappable::Trace(visitor);
  }

 private:
  const String type_;
  const String url_;
  Member<ReportBody> body_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORT_H_
