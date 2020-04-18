/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PLUGINS_DOM_MIME_TYPE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PLUGINS_DOM_MIME_TYPE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/plugins/plugin_data.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class DOMPlugin;
class LocalFrame;

class DOMMimeType final : public ScriptWrappable, public ContextClient {
  USING_GARBAGE_COLLECTED_MIXIN(DOMMimeType);
  DEFINE_WRAPPERTYPEINFO();

 public:
  static DOMMimeType* Create(LocalFrame* frame,
                             const MimeClassInfo& mime_class_info) {
    return new DOMMimeType(frame, mime_class_info);
  }

  const String& type() const;
  String suffixes() const;
  const String& description() const;
  DOMPlugin* enabledPlugin() const;

  void Trace(blink::Visitor*) override;

 private:
  DOMMimeType(LocalFrame*, const MimeClassInfo&);

  Member<const MimeClassInfo> mime_class_info_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PLUGINS_DOM_MIME_TYPE_H_
