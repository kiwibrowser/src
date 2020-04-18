// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_RESOURCE_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_RESOURCE_VALUE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/css/cssom/css_style_value.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"

namespace blink {

class CORE_EXPORT CSSResourceValue : public CSSStyleValue {
 public:
  ~CSSResourceValue() override = default;

  const String state() const {
    switch (Status()) {
      case ResourceStatus::kNotStarted:
        return "unloaded";
      case ResourceStatus::kPending:
        return "loading";
      case ResourceStatus::kCached:
        return "loaded";
      case ResourceStatus::kLoadError:
      case ResourceStatus::kDecodeError:
        return "error";
      default:
        NOTREACHED();
        return "";
    }
  }

  void Trace(blink::Visitor* visitor) override {
    CSSStyleValue::Trace(visitor);
  }

 protected:
  CSSResourceValue() = default;

  virtual ResourceStatus Status() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CSSResourceValue);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_RESOURCE_VALUE_H_
