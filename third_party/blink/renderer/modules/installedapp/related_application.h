// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_INSTALLEDAPP_RELATED_APPLICATION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_INSTALLEDAPP_RELATED_APPLICATION_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class RelatedApplication final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static RelatedApplication* Create(const String& platform,
                                    const String& url,
                                    const String& id) {
    return new RelatedApplication(platform, url, id);
  }

  ~RelatedApplication() override = default;

  String platform() const { return platform_; }
  String url() const { return url_; }
  String id() const { return id_; }

 private:
  RelatedApplication(const String& platform,
                     const String& url,
                     const String& id)
      : platform_(platform), url_(url), id_(id) {}

  const String platform_;
  const String url_;
  const String id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_INSTALLEDAPP_RELATED_APPLICATION_H_
