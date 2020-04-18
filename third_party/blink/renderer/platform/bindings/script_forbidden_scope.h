// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_FORBIDDEN_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_FORBIDDEN_SCOPE_H_

#include "base/auto_reset.h"
#include "base/macros.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/stack_util.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {

// Scoped disabling of script execution.
class PLATFORM_EXPORT ScriptForbiddenScope final {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(ScriptForbiddenScope);

 public:
  ScriptForbiddenScope() { Enter(); }
  ~ScriptForbiddenScope() { Exit(); }

  class PLATFORM_EXPORT AllowUserAgentScript final {
    STACK_ALLOCATED();
    DISALLOW_COPY_AND_ASSIGN(AllowUserAgentScript);

   public:
    AllowUserAgentScript() : saved_counter_(&GetMutableCounter(), 0) {}
    ~AllowUserAgentScript() { DCHECK(!IsScriptForbidden()); }

   private:
    base::AutoReset<unsigned> saved_counter_;
  };

  static bool IsScriptForbidden() {
    if (LIKELY(!WTF::MayNotBeMainThread()))
      return g_main_thread_counter_ > 0;
    return GetMutableCounter() > 0;
  }

  // DO NOT USE THESE FUNCTIONS FROM OUTSIDE OF THIS CLASS.
  static void Enter() {
    if (LIKELY(!WTF::MayNotBeMainThread())) {
      ++g_main_thread_counter_;
    } else {
      ++GetMutableCounter();
    }
  }
  static void Exit() {
    DCHECK(IsScriptForbidden());
    if (LIKELY(!WTF::MayNotBeMainThread())) {
      --g_main_thread_counter_;
    } else {
      --GetMutableCounter();
    }
  }

 private:
  static unsigned& GetMutableCounter();
  static unsigned g_main_thread_counter_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_FORBIDDEN_SCOPE_H_
