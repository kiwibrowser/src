// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_MEDIA_CAPABILITIES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_MEDIA_CAPABILITIES_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

class MediaDecodingConfiguration;
class MediaEncodingConfiguration;
class ScriptPromise;
class ScriptState;

class MediaCapabilities final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  MediaCapabilities();

  ScriptPromise decodingInfo(ScriptState*, const MediaDecodingConfiguration&);
  ScriptPromise encodingInfo(ScriptState*, const MediaEncodingConfiguration&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_MEDIA_CAPABILITIES_H_
