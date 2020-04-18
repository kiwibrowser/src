/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/timing/performance_navigation.h"

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_object_builder.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader_types.h"

// Legacy support for NT1(https://www.w3.org/TR/navigation-timing/).
namespace blink {

PerformanceNavigation::PerformanceNavigation(LocalFrame* frame)
    : DOMWindowClient(frame) {}

unsigned short PerformanceNavigation::type() const {
  if (!GetFrame())
    return kTypeNavigate;

  DocumentLoader* document_loader = GetFrame()->Loader().GetDocumentLoader();
  if (!document_loader)
    return kTypeNavigate;

  switch (document_loader->GetNavigationType()) {
    case kNavigationTypeReload:
      return kTypeReload;
    case kNavigationTypeBackForward:
      return kTypeBackForward;
    default:
      return kTypeNavigate;
  }
}

unsigned short PerformanceNavigation::redirectCount() const {
  if (!GetFrame())
    return 0;

  DocumentLoader* loader = GetFrame()->Loader().GetDocumentLoader();
  if (!loader)
    return 0;

  const DocumentLoadTiming& timing = loader->GetTiming();
  if (timing.HasCrossOriginRedirect())
    return 0;

  return timing.RedirectCount();
}

ScriptValue PerformanceNavigation::toJSONForBinding(
    ScriptState* script_state) const {
  V8ObjectBuilder result(script_state);
  result.AddNumber("type", type());
  result.AddNumber("redirectCount", redirectCount());
  return result.GetScriptValue();
}

void PerformanceNavigation::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  DOMWindowClient::Trace(visitor);
}

}  // namespace blink
