// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_SET_SINK_ID_CALLBACKS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_SET_SINK_ID_CALLBACKS_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_set_sink_id_callbacks.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class HTMLMediaElement;
class ScriptPromiseResolver;

class SetSinkIdCallbacks final : public WebSetSinkIdCallbacks {
  // FIXME(tasak): When making public/platform classes to use PartitionAlloc,
  // the following macro should be moved to WebCallbacks defined in
  // public/platform/WebCallbacks.h.
  USING_FAST_MALLOC(SetSinkIdCallbacks);
  WTF_MAKE_NONCOPYABLE(SetSinkIdCallbacks);

 public:
  SetSinkIdCallbacks(ScriptPromiseResolver*,
                     HTMLMediaElement&,
                     const String& sink_id);
  ~SetSinkIdCallbacks() override;

  void OnSuccess() override;
  void OnError(WebSetSinkIdError) override;

 private:
  Persistent<ScriptPromiseResolver> resolver_;
  Persistent<HTMLMediaElement> element_;
  String sink_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_AUDIO_OUTPUT_DEVICES_SET_SINK_ID_CALLBACKS_H_
