// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/private/frame_client_hints_preferences_context.h"

#include "third_party/blink/renderer/core/frame/use_counter.h"

namespace blink {

namespace {

// Mapping from WebClientHintsType to WebFeature. The ordering should match the
// ordering of enums in WebClientHintsType.
static constexpr WebFeature kWebFeatureMapping[] = {
    WebFeature::kClientHintsDeviceMemory,
    WebFeature::kClientHintsDPR,
    WebFeature::kClientHintsResourceWidth,
    WebFeature::kClientHintsViewportWidth,
    WebFeature::kClientHintsRtt,
    WebFeature::kClientHintsDownlink,
    WebFeature::kClientHintsEct,
};

static_assert(static_cast<int>(mojom::WebClientHintsType::kMaxValue) + 1 ==
                  arraysize(kWebFeatureMapping),
              "unhandled client hint type");

}  // namespace

FrameClientHintsPreferencesContext::FrameClientHintsPreferencesContext(
    LocalFrame* frame)
    : frame_(frame) {}

void FrameClientHintsPreferencesContext::CountClientHints(
    mojom::WebClientHintsType type) {
  UseCounter::Count(frame_, kWebFeatureMapping[static_cast<int32_t>(type)]);
}

void FrameClientHintsPreferencesContext::CountPersistentClientHintHeaders() {
  UseCounter::Count(frame_, WebFeature::kPersistentClientHintHeader);
}

}  // namespace blink
