// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MOCK_CONSTRAINT_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MOCK_CONSTRAINT_FACTORY_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "third_party/blink/public/platform/web_media_constraints.h"

namespace content {

class MockConstraintFactory {
 public:
  MockConstraintFactory();
  ~MockConstraintFactory();

  blink::WebMediaConstraints CreateWebMediaConstraints() const;
  blink::WebMediaTrackConstraintSet& basic() { return basic_; }
  blink::WebMediaTrackConstraintSet& AddAdvanced();

  void DisableDefaultAudioConstraints();
  void DisableAecAudioConstraints();
  void Reset();

 private:
  blink::WebMediaTrackConstraintSet basic_;
  std::vector<blink::WebMediaTrackConstraintSet> advanced_;

  DISALLOW_COPY_AND_ASSIGN(MockConstraintFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MOCK_CONSTRAINT_FACTORY_H_
