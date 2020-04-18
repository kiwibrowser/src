// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_EFFECT_PROXY_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_EFFECT_PROXY_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class MODULES_EXPORT EffectProxy : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  EffectProxy() = default;

  void setLocalTime(double time_ms) {
    // Convert double to TimeDelta because cc/animation expects TimeDelta.
    //
    // Note on precision loss: TimeDelta has microseconds precision which is
    // also the precision recommended by the web animation specification as well
    // [1]. If the input time value has a bigger precision then the conversion
    // causes precision loss. Doing the conversion here ensures that reading the
    // value back provides the actual value we use in further computation which
    // is the least surprising path.
    // [1] https://drafts.csswg.org/web-animations/#precision-of-time-values
    local_time_ = WTF::TimeDelta::FromMillisecondsD(time_ms);
  }

  double localTime() const { return local_time_.InMillisecondsF(); }

  WTF::TimeDelta GetLocalTime() const { return local_time_; }

 private:
  WTF::TimeDelta local_time_;
};

}  // namespace blink

#endif
