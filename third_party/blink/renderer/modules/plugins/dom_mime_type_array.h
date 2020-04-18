/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PLUGINS_DOM_MIME_TYPE_ARRAY_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PLUGINS_DOM_MIME_TYPE_ARRAY_H_

#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/page/plugins_changed_observer.h"
#include "third_party/blink/renderer/modules/plugins/dom_mime_type.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ExceptionState;
class LocalFrame;
class PluginData;

class DOMMimeTypeArray final : public ScriptWrappable,
                               public ContextLifecycleObserver,
                               public PluginsChangedObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(DOMMimeTypeArray);

 public:
  static DOMMimeTypeArray* Create(LocalFrame* frame) {
    return new DOMMimeTypeArray(frame);
  }
  void UpdatePluginData();

  unsigned length() const;
  DOMMimeType* item(unsigned index);
  DOMMimeType* namedItem(const AtomicString& property_name);
  void NamedPropertyEnumerator(Vector<String>&, ExceptionState&) const;
  bool NamedPropertyQuery(const AtomicString&, ExceptionState&) const;

  // PluginsChangedObserver implementation.
  void PluginsChanged() override;

  void Trace(blink::Visitor*) override;

 private:
  explicit DOMMimeTypeArray(LocalFrame*);
  PluginData* GetPluginData() const;
  void ContextDestroyed(ExecutionContext*) override;

  HeapVector<Member<DOMMimeType>> dom_mime_types_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PLUGINS_DOM_MIME_TYPE_ARRAY_H_
