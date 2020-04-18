/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FULLSCREEN_FULLSCREEN_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FULLSCREEN_FULLSCREEN_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ComputedStyle;
class FullscreenOptions;
class LayoutFullScreen;

// The Fullscreen class implements most of the Fullscreen API Standard,
// https://fullscreen.spec.whatwg.org/, especially its algorithms. It is a
// Document supplement as each document has some fullscreen state, and to
// actually enter and exit fullscreen it (indirectly) uses FullscreenController.
class CORE_EXPORT Fullscreen final
    : public GarbageCollectedFinalized<Fullscreen>,
      public Supplement<Document>,
      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(Fullscreen);

 public:
  static const char kSupplementName[];

  virtual ~Fullscreen();
  static Fullscreen& From(Document&);
  static Fullscreen* FromIfExists(Document&);
  static Element* FullscreenElementFrom(Document&);
  static Element* FullscreenElementForBindingFrom(TreeScope&);
  static size_t FullscreenElementStackSizeFrom(Document&);
  static bool IsFullscreenElement(const Element&);
  static bool IsInFullscreenElementStack(const Element&);

  enum class RequestType {
    // Element.requestFullscreen()
    kUnprefixed,
    // Element.webkitRequestFullscreen()/webkitRequestFullScreen() and
    // HTMLVideoElement.webkitEnterFullscreen()/webkitEnterFullScreen()
    kPrefixed,
    // For WebRemoteFrameImpl to notify that a cross-process descendant frame
    // has requested and is about to enter fullscreen.
    kPrefixedForCrossProcessDescendant,
  };

  static void RequestFullscreen(Element&);
  static void RequestFullscreen(Element&,
                                const FullscreenOptions&,
                                RequestType);

  static void FullyExitFullscreen(Document&);
  static void ExitFullscreen(Document&);

  static bool FullscreenEnabled(Document&);
  Element* FullscreenElement() const {
    return !fullscreen_element_stack_.IsEmpty()
               ? fullscreen_element_stack_.back().first.Get()
               : nullptr;
  }

  // Called by FullscreenController to notify that we've entered or exited
  // fullscreen. All frames are notified, so there may be no pending request.
  void DidEnterFullscreen();
  void DidExitFullscreen();

  void SetFullScreenLayoutObject(LayoutFullScreen*);
  LayoutFullScreen* FullScreenLayoutObject() const {
    return full_screen_layout_object_;
  }
  void FullScreenLayoutObjectDestroyed();

  void ElementRemoved(Element&);

  // ContextLifecycleObserver:
  void ContextDestroyed(ExecutionContext*) override;

  void Trace(blink::Visitor*) override;

 private:
  static Fullscreen* FromIfExistsSlow(Document&);

  explicit Fullscreen(Document&);

  Document* GetDocument();

  static void ContinueRequestFullscreen(Document&,
                                        Element&,
                                        RequestType,
                                        bool error);

  static void ContinueExitFullscreen(Document*, bool resize);

  void ClearFullscreenElementStack();
  void PopFullscreenElementStack();
  void PushFullscreenElementStack(Element&, RequestType);
  void FullscreenElementChanged(Element* old_element,
                                Element* new_element,
                                RequestType new_request_type);

  using ElementStackEntry = std::pair<Member<Element>, RequestType>;
  using ElementStack = HeapVector<ElementStackEntry>;
  ElementStack pending_requests_;
  ElementStack fullscreen_element_stack_;

  LayoutFullScreen* full_screen_layout_object_;
  LayoutRect saved_placeholder_frame_rect_;
  scoped_refptr<ComputedStyle> saved_placeholder_computed_style_;
};

inline Fullscreen* Fullscreen::FromIfExists(Document& document) {
  if (!document.HasFullscreenSupplement())
    return nullptr;
  return FromIfExistsSlow(document);
}

inline bool Fullscreen::IsFullscreenElement(const Element& element) {
  if (Fullscreen* found = FromIfExists(element.GetDocument()))
    return found->FullscreenElement() == &element;
  return false;
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FULLSCREEN_FULLSCREEN_H_
