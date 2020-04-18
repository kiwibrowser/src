// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_SCHEDULING_TYPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCRIPT_SCRIPT_SCHEDULING_TYPE_H_

namespace blink {

// Type of <script>'s scheduling.
//
// In the spec, this is determined which clause of Step 25 of
// https://html.spec.whatwg.org/multipage/scripting.html#prepare-a-script
// is taken.
enum class ScriptSchedulingType {
  // Because the sheduling type is determined slightly after PendingScript
  // creation, it is set to kNotSet before ScriptLoader::TakePendingScript()
  // is called. kNotSet should not be observed.
  kNotSet,

  // Deferred scripts controlled by HTMLParserScriptRunner.
  //
  // Spec: 1st Clause.
  //
  // Examples:
  // - <script defer> (parser inserted)
  // - <script type="module"> (parser inserted)
  kDefer,

  // Parser-blocking external scripts controlled by XML/HTMLParserScriptRunner.
  //
  // Spec: 2nd Clause.
  //
  // Examples:
  // - <script> (parser inserted)
  kParserBlocking,

  // Parser-blocking inline scripts controlled by XML/HTMLParserScriptRunner.
  //
  // Spec: 5th Clause.
  kParserBlockingInline,

  // In-order scripts controlled by ScriptRunner.
  //
  // Spec: 3rd Clause.
  //
  // Examples (either classic or module):
  // - Dynamically inserted <script>s with s.async = false
  kInOrder,

  // Async scripts controlled by ScriptRunner.
  //
  // Spec: 4nd Clause.
  //
  // Examples (either classic or module):
  // - <script async> and <script async defer> (parser inserted)
  // - Dynamically inserted <script>s
  // - Dynamically inserted <script>s with s.async = true
  kAsync,

  // Inline <script> executed immediately within prepare-a-script.
  kImmediate
};

}  // namespace blink

#endif
