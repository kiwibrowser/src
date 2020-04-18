// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_ACTIVE_SCRIPT_WRAPPABLE_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_ACTIVE_SCRIPT_WRAPPABLE_BASE_H_

#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "v8/include/v8.h"

namespace blink {

class ScriptWrappable;
class ScriptWrappableVisitor;

/**
 * Classes deriving from ActiveScriptWrappable will be registered in a
 * thread-specific list. They keep their wrappers and dependant objects alive
 * as long as they have pending activity.
 */
class PLATFORM_EXPORT ActiveScriptWrappableBase : public GarbageCollectedMixin {
  WTF_MAKE_NONCOPYABLE(ActiveScriptWrappableBase);

 public:
  static void TraceActiveScriptWrappables(v8::Isolate*,
                                          ScriptWrappableVisitor*);

  virtual ~ActiveScriptWrappableBase() = default;

 protected:
  ActiveScriptWrappableBase();

  virtual bool IsContextDestroyed() const = 0;
  virtual bool DispatchHasPendingActivity() const = 0;
  virtual ScriptWrappable* ToScriptWrappable() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_ACTIVE_SCRIPT_WRAPPABLE_BASE_H_
