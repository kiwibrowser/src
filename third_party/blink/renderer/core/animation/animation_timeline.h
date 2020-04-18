// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_TIMELINE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_TIMELINE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class CORE_EXPORT AnimationTimeline : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~AnimationTimeline() override = default;

  virtual double currentTime(bool&) = 0;

  virtual bool IsDocumentTimeline() const { return false; }
  virtual bool IsScrollTimeline() const { return false; }
};

}  // namespace blink

#endif
