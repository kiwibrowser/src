/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCATION_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_string_list.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class Document;
class ExceptionState;
class KURL;
class LocalDOMWindow;
class USVStringOrTrustedURL;

// This class corresponds to the Location interface. Location is the only
// interface besides Window that is accessible cross-origin and must handle
// remote frames.
//
// HTML standard: https://whatwg.org/C/browsers.html#the-location-interface
class CORE_EXPORT Location final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Location* Create(DOMWindow* dom_window) {
    return new Location(dom_window);
  }

  DOMWindow* DomWindow() const { return dom_window_.Get(); }

  void setHref(LocalDOMWindow* current_window,
               LocalDOMWindow* entered_window,
               const USVStringOrTrustedURL&,
               ExceptionState&);
  void href(USVStringOrTrustedURL&) const;

  void assign(LocalDOMWindow* current_window,
              LocalDOMWindow* entered_window,
              const String&,
              ExceptionState&);
  void replace(LocalDOMWindow* current_window,
               LocalDOMWindow* entered_window,
               const String&,
               ExceptionState&);
  void reload(LocalDOMWindow* current_window);

  void setProtocol(LocalDOMWindow* current_window,
                   LocalDOMWindow* entered_window,
                   const String&,
                   ExceptionState&);
  String protocol() const;
  void setHost(LocalDOMWindow* current_window,
               LocalDOMWindow* entered_window,
               const String&,
               ExceptionState&);
  String host() const;
  void setHostname(LocalDOMWindow* current_window,
                   LocalDOMWindow* entered_window,
                   const String&,
                   ExceptionState&);
  String hostname() const;
  void setPort(LocalDOMWindow* current_window,
               LocalDOMWindow* entered_window,
               const String&,
               ExceptionState&);
  String port() const;
  void setPathname(LocalDOMWindow* current_window,
                   LocalDOMWindow* entered_window,
                   const String&,
                   ExceptionState&);
  String pathname() const;
  void setSearch(LocalDOMWindow* current_window,
                 LocalDOMWindow* entered_window,
                 const String&,
                 ExceptionState&);
  String search() const;
  void setHash(LocalDOMWindow* current_window,
               LocalDOMWindow* entered_window,
               const String&,
               ExceptionState&);
  String hash() const;
  String origin() const;

  DOMStringList* ancestorOrigins() const;

  // Just return the |this| object the way the normal valueOf function on the
  // Object prototype would.  The valueOf function is only added to make sure
  // that it cannot be overwritten on location objects, since that would provide
  // a hook to change the string conversion behavior of location objects.
  ScriptValue valueOf(const ScriptValue& this_object) { return this_object; }

  String toString() const;

  void Trace(blink::Visitor*) override;

 private:
  explicit Location(DOMWindow*);

  // Note: it is only valid to call this if this is a Location object for a
  // LocalDOMWindow.
  Document* GetDocument() const;

  // Returns true if the associated Window is the active Window in the frame.
  bool IsAttached() const;

  enum class SetLocationPolicy { kNormal, kReplaceThisFrame };
  void SetLocation(const String&,
                   LocalDOMWindow* current_window,
                   LocalDOMWindow* entered_window,
                   ExceptionState* = nullptr,
                   SetLocationPolicy = SetLocationPolicy::kNormal);

  const KURL& Url() const;

  const Member<DOMWindow> dom_window_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCATION_H_
