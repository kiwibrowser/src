// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_CONTENT_SETTING_CALLBACKS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_CONTENT_SETTING_CALLBACKS_H_

#include <memory>
#include "base/callback.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class PLATFORM_EXPORT ContentSettingCallbacks {
  USING_FAST_MALLOC(ContentSettingCallbacks);
  WTF_MAKE_NONCOPYABLE(ContentSettingCallbacks);

 public:
  static std::unique_ptr<ContentSettingCallbacks> Create(
      base::OnceClosure allowed,
      base::OnceClosure denied);
  virtual ~ContentSettingCallbacks() = default;

  void OnAllowed() { std::move(allowed_).Run(); }
  void OnDenied() { std::move(denied_).Run(); }

 private:
  ContentSettingCallbacks(base::OnceClosure allowed, base::OnceClosure denied);

  base::OnceClosure allowed_;
  base::OnceClosure denied_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_CONTENT_SETTING_CALLBACKS_H_
