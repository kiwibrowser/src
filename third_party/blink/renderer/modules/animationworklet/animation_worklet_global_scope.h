// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_GLOBAL_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_GLOBAL_SCOPE_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/workers/threaded_worklet_global_scope.h"
#include "third_party/blink/renderer/modules/animationworklet/animator.h"
#include "third_party/blink/renderer/modules/animationworklet/animator_definition.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/graphics/compositor_animators_state.h"

namespace blink {

class ExceptionState;

// Represents the animation worklet global scope and implements all methods that
// the global scope exposes to user script (See
// |AnimationWorkletGlobalScope.idl|). The instances of this class live on the
// worklet thread but have a corresponding proxy on the main thread which is
// accessed by the animation worklet instance. User scripts can register
// animator definitions with the global scope (via |registerAnimator| method).
// The scope keeps a map of these animator definitions and can look them up
// based on their name. The scope also owns a list of active animators that it
// animates.
class MODULES_EXPORT AnimationWorkletGlobalScope
    : public ThreadedWorkletGlobalScope {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static AnimationWorkletGlobalScope* Create(
      std::unique_ptr<GlobalScopeCreationParams>,
      v8::Isolate*,
      WorkerThread*);
  ~AnimationWorkletGlobalScope() override;
  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  void Dispose() override;
  bool IsAnimationWorkletGlobalScope() const final { return true; }

  Animator* CreateInstance(const String& name);
  // Invokes the |animate| function of all of its active animators.
  std::unique_ptr<CompositorMutatorOutputState> Mutate(
      const CompositorMutatorInputState&);

  // Registers a animator definition with the given name and constructor.
  void registerAnimator(const String& name,
                        const ScriptValue& constructor_value,
                        ExceptionState&);

  AnimatorDefinition* FindDefinitionForTest(const String& name);
  unsigned GetAnimatorsSizeForTest() { return animators_.size(); }

 private:
  AnimationWorkletGlobalScope(std::unique_ptr<GlobalScopeCreationParams>,
                              v8::Isolate*,
                              WorkerThread*);

  void RegisterWithProxyClientIfNeeded();
  Animator* GetAnimatorFor(int animation_id, const String& name);
  typedef HeapHashMap<String, TraceWrapperMember<AnimatorDefinition>>
      DefinitionMap;
  DefinitionMap animator_definitions_;

  typedef HeapHashMap<int, TraceWrapperMember<Animator>> AnimatorMap;
  AnimatorMap animators_;

  bool registered_ = false;
};

DEFINE_TYPE_CASTS(AnimationWorkletGlobalScope,
                  ExecutionContext,
                  context,
                  context->IsAnimationWorkletGlobalScope(),
                  context.IsAnimationWorkletGlobalScope());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_GLOBAL_SCOPE_H_
