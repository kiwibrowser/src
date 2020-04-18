// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLASS_TRACE_WRAPPERS_BASES_H_
#define CLASS_TRACE_WRAPPERS_BASES_H_

#include "heap/stubs.h"

namespace blink {

class EmptyScriptWrappable : public ScriptWrappable {
  // No TraceWrappers required.
};

class CustomScriptWrappable : public ScriptWrappable {
 public:
  void TraceWrappers(const ScriptWrappableVisitor*) const override;
};

class MixinClassWithTraceWrappers1 {
 public:
  virtual void TraceWrappers(const ScriptWrappableVisitor*) const {}
};

class MixinClassWithTraceWrappers2 {
 public:
  virtual void TraceWrappers(const ScriptWrappableVisitor*) const {}
};

class MixinApplication : public TraceWrapperBase,
                         public MixinClassWithTraceWrappers1,
                         public MixinClassWithTraceWrappers2 {
 public:
  void TraceWrappers(const ScriptWrappableVisitor*) const override;
};

}  // namespace blink

#endif  // CLASS_TRACE_WRAPPERS_BASES_H_
