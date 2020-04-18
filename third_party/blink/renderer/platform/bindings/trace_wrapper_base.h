// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_TRACE_WRAPPER_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_TRACE_WRAPPER_BASE_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class ScriptWrappableVisitor;

class PLATFORM_EXPORT TraceWrapperBase {
  WTF_MAKE_NONCOPYABLE(TraceWrapperBase);

 public:
  TraceWrapperBase() = default;
  ~TraceWrapperBase() = default;
  virtual bool IsScriptWrappable() const { return false; }

  virtual void TraceWrappers(ScriptWrappableVisitor*) const = 0;

  // Human-readable name of this object. The DevTools heap snapshot uses
  // this method to show the object.
  virtual const char* NameInHeapSnapshot() const = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_TRACE_WRAPPER_BASE_H_
