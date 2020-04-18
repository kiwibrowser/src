// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_MEDIA_CAPABILITIES_INFO_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_MEDIA_CAPABILITIES_INFO_H_

#include <memory>

#include "third_party/blink/public/platform/modules/media_capabilities/web_media_capabilities_info.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class ScriptPromiseResolver;

// Implementation of the MediaCapabilitiesInfo interface.
class MediaCapabilitiesInfo final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using WebType = std::unique_ptr<WebMediaCapabilitiesInfo>;
  static MediaCapabilitiesInfo* Take(ScriptPromiseResolver*,
                                     std::unique_ptr<WebMediaCapabilitiesInfo>);

  bool supported() const;
  bool smooth() const;
  bool powerEfficient() const;

 private:
  MediaCapabilitiesInfo() = delete;
  explicit MediaCapabilitiesInfo(std::unique_ptr<WebMediaCapabilitiesInfo>);

  std::unique_ptr<WebMediaCapabilitiesInfo> web_media_capabilities_info_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CAPABILITIES_MEDIA_CAPABILITIES_INFO_H_
