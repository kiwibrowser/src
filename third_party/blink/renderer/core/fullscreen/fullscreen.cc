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

#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"

#include "base/macros.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/frame/hosts_using_features.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen_options.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/html_element_type_helpers.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/layout_full_screen.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/svg/svg_svg_element.h"
#include "third_party/blink/renderer/platform/bindings/microtask.h"
#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"
#include "third_party/blink/renderer/platform/scoped_orientation_change_indicator.h"

namespace blink {

namespace {

// https://html.spec.whatwg.org/multipage/embedded-content.html#allowed-to-use
bool AllowedToUseFullscreen(const Frame* frame) {
  // To determine whether a Document object |document| is allowed to use the
  // feature indicated by attribute name |allowattribute|, run these steps:

  // 1. If |document| has no browsing context, then return false.
  if (!frame)
    return false;

  if (!IsSupportedInFeaturePolicy(mojom::FeaturePolicyFeature::kFullscreen)) {
    // 2. If |document|'s browsing context is a top-level browsing context, then
    // return true.
    if (frame->IsMainFrame())
      return true;

    // 3. If |document|'s browsing context has a browsing context container that
    // is an iframe element with an |allowattribute| attribute specified, and
    // whose node document is allowed to use the feature indicated by
    // |allowattribute|, then return true.
    if (frame->Owner() && frame->Owner()->AllowFullscreen())
      return AllowedToUseFullscreen(frame->Tree().Parent());

    // 4. Return false.
    return false;
  }

  // 2. If Feature Policy is enabled, return the policy for "fullscreen"
  // feature.
  return frame->IsFeatureEnabled(mojom::FeaturePolicyFeature::kFullscreen);
}

bool AllowedToRequestFullscreen(Document& document) {
  // An algorithm is allowed to request fullscreen if one of the following is
  // true:

  //  The algorithm is triggered by a user activation.
  if (Frame::HasTransientUserActivation(document.GetFrame()))
    return true;

  //  The algorithm is triggered by a user generated orientation change.
  if (ScopedOrientationChangeIndicator::ProcessingOrientationChange()) {
    UseCounter::Count(document,
                      WebFeature::kFullscreenAllowedByOrientationChange);
    return true;
  }

  String message = ExceptionMessages::FailedToExecute(
      "requestFullscreen", "Element",
      "API can only be initiated by a user gesture.");
  document.AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kWarningMessageLevel, message));

  return false;
}

// https://fullscreen.spec.whatwg.org/#fullscreen-is-supported
bool FullscreenIsSupported(const Document& document) {
  LocalFrame* frame = document.GetFrame();
  if (!frame)
    return false;

  // Fullscreen is supported if there is no previously-established user
  // preference, security risk, or platform limitation.
  return !document.GetSettings() ||
         document.GetSettings()->GetFullscreenSupported();
}

// https://fullscreen.spec.whatwg.org/#fullscreen-element-ready-check
bool FullscreenElementReady(const Element& element) {
  // A fullscreen element ready check for an element |element| returns true if
  // all of the following are true, and false otherwise:

  // |element| is in a document.
  if (!element.isConnected())
    return false;

  // |element|'s node document is allowed to use the feature indicated by
  // attribute name allowfullscreen.
  if (!AllowedToUseFullscreen(element.GetDocument().GetFrame()))
    return false;

  // |element|'s node document's fullscreen element stack is either empty or its
  // top element is an inclusive ancestor of |element|.
  if (const Element* top_element =
          Fullscreen::FullscreenElementFrom(element.GetDocument())) {
    if (!top_element->contains(&element))
      return false;
  }

  // |element| has no ancestor element whose local name is iframe and namespace
  // is the HTML namespace.
  if (Traversal<HTMLIFrameElement>::FirstAncestor(element))
    return false;

  // |element|'s node document's browsing context either has a browsing context
  // container and the fullscreen element ready check returns true for
  // |element|'s node document's browsing context's browsing context container,
  // or it has no browsing context container.
  if (const Element* owner = element.GetDocument().LocalOwner()) {
    if (!FullscreenElementReady(*owner))
      return false;
  }

  return true;
}

// https://fullscreen.spec.whatwg.org/#dom-element-requestfullscreen step 4:
bool RequestFullscreenConditionsMet(Element& pending, Document& document) {
  // |pending|'s namespace is the HTML namespace or |pending| is an SVG svg or
  // MathML math element. Note: MathML is not supported.
  if (!pending.IsHTMLElement() && !IsSVGSVGElement(pending))
    return false;

  // |pending| is not a dialog element.
  if (IsHTMLDialogElement(pending))
    return false;

  // The fullscreen element ready check for |pending| returns false.
  if (!FullscreenElementReady(pending))
    return false;

  // Fullscreen is supported.
  if (!FullscreenIsSupported(document))
    return false;

  // This algorithm is allowed to request fullscreen.
  if (!AllowedToRequestFullscreen(document))
    return false;

  return true;
}

// RequestFullscreenScope is allocated at the top of |RequestFullscreen()| and
// used to avoid synchronously changing any state within that method, by
// deferring changes in |DidEnterFullscreen()|.
class RequestFullscreenScope {
  STACK_ALLOCATED();

 public:
  RequestFullscreenScope() {
    DCHECK(!running_request_fullscreen_);
    running_request_fullscreen_ = true;
  }

  ~RequestFullscreenScope() {
    DCHECK(running_request_fullscreen_);
    running_request_fullscreen_ = false;
  }

  static bool RunningRequestFullscreen() { return running_request_fullscreen_; }

 private:
  static bool running_request_fullscreen_;
  DISALLOW_COPY_AND_ASSIGN(RequestFullscreenScope);
};

bool RequestFullscreenScope::running_request_fullscreen_ = false;

// Walks the frame tree and returns the first local ancestor frame, if any.
LocalFrame* NextLocalAncestor(Frame& frame) {
  Frame* parent = frame.Tree().Parent();
  if (!parent)
    return nullptr;
  if (parent->IsLocalFrame())
    return ToLocalFrame(parent);
  return NextLocalAncestor(*parent);
}

// Walks the document's frame tree and returns the document of the first local
// ancestor frame, if any.
Document* NextLocalAncestor(Document& document) {
  LocalFrame* frame = document.GetFrame();
  if (!frame)
    return nullptr;
  LocalFrame* next = NextLocalAncestor(*frame);
  if (!next)
    return nullptr;
  DCHECK(next->GetDocument());
  return next->GetDocument();
}

// Helper to walk the ancestor chain and return the Document of the topmost
// local ancestor frame. Note that this is not the same as the topmost frame's
// Document, which might be unavailable in OOPIF scenarios. For example, with
// OOPIFs, when called on the bottom frame's Document in a A-B-C-B hierarchy in
// process B, this will skip remote frame C and return this frame: A-[B]-C-B.
Document& TopmostLocalAncestor(Document& document) {
  if (Document* next = NextLocalAncestor(document))
    return TopmostLocalAncestor(*next);
  return document;
}

// https://fullscreen.spec.whatwg.org/#simple-fullscreen-document
bool IsSimpleFullscreenDocument(Document& document) {
  return Fullscreen::FullscreenElementStackSizeFrom(document) == 1;
}

// https://fullscreen.spec.whatwg.org/#collect-documents-to-unfullscreen
HeapVector<Member<Document>> CollectDocumentsToUnfullscreen(Document& doc) {
  // 1. Let |docs| be an ordered set consisting of |doc|.
  HeapVector<Member<Document>> docs;
  docs.push_back(&doc);

  // 2. While true:
  for (Document* lastDoc = &doc;;) {
    // 2.1. Let |lastDoc| be |docs|'s last document.

    // 2.2. Assert: |lastDoc|'s fullscreen element is not null.
    DCHECK(Fullscreen::FullscreenElementFrom(*lastDoc));

    // 2.3. If |lastDoc| is not a simple fullscreen document, break.
    if (!IsSimpleFullscreenDocument(*lastDoc))
      break;

    // 2.4. Let |container| be |lastDoc|'s browsing context container, if any,
    // and otherwise break.
    //
    // OOPIF: Skip over remote frames, assuming that they have exactly one
    // element in their fullscreen element stacks, thereby erring on the side of
    // exiting fullscreen. TODO(alexmos): Deal with nested fullscreen cases, see
    // https://crbug.com/617369.
    lastDoc = NextLocalAncestor(*lastDoc);
    if (!lastDoc)
      break;

    // 2.5. If |container|'s iframe fullscreen flag is set, break.
    // TODO(foolip): Support the iframe fullscreen flag.
    // https://crbug.com/644695

    // 2.6. Append |container|'s node document to |docs|.
    docs.push_back(lastDoc);
  }

  // 3. Return |docs|.
  return docs;
}

// https://fullscreen.spec.whatwg.org/#run-the-fullscreen-rendering-steps
void FireEvent(const AtomicString& type, Element* element, Document* document) {
  DCHECK(document);
  DCHECK(element);

  // |Document::EnqueueAnimationFrameTask()| is used instead of a "list of
  // pending fullscreen events", so only the body of the "run the fullscreen
  // rendering steps" loop appears here:

  // 3.1. Let |target| be |element| if |element| is connected and its node
  // document is |document|, and otherwise let |target| be |document|.
  EventTarget* target =
      element->isConnected() && &element->GetDocument() == document
          ? static_cast<EventTarget*>(element)
          : static_cast<EventTarget*>(document);

  // 3.2. Fire an event named |type|, with its bubbles and composed attributes
  // set to true, at |target|.
  Event* event = Event::CreateBubble(type);
  event->SetComposed(true);
  target->DispatchEvent(event);
}

const AtomicString& AdjustEventType(const AtomicString& type,
                                    Fullscreen::RequestType request_type) {
  DCHECK(type == EventTypeNames::fullscreenchange ||
         type == EventTypeNames::fullscreenerror);

  if (request_type == Fullscreen::RequestType::kUnprefixed)
    return type;
  return type == EventTypeNames::fullscreenchange
             ? EventTypeNames::webkitfullscreenchange
             : EventTypeNames::webkitfullscreenerror;
}

void EnqueueEvent(const AtomicString& type,
                  Element& element,
                  Document& document,
                  Fullscreen::RequestType request_type) {
  const AtomicString& adjusted_type = AdjustEventType(type, request_type);
  document.EnqueueAnimationFrameTask(WTF::Bind(FireEvent, adjusted_type,
                                               WrapPersistent(&element),
                                               WrapPersistent(&document)));
}

}  // anonymous namespace

const char Fullscreen::kSupplementName[] = "Fullscreen";

Fullscreen& Fullscreen::From(Document& document) {
  Fullscreen* fullscreen = FromIfExists(document);
  if (!fullscreen) {
    fullscreen = new Fullscreen(document);
    ProvideTo(document, fullscreen);
  }

  return *fullscreen;
}

Fullscreen* Fullscreen::FromIfExistsSlow(Document& document) {
  return Supplement<Document>::From<Fullscreen>(document);
}

Element* Fullscreen::FullscreenElementFrom(Document& document) {
  if (Fullscreen* found = FromIfExists(document))
    return found->FullscreenElement();
  return nullptr;
}

size_t Fullscreen::FullscreenElementStackSizeFrom(Document& document) {
  if (Fullscreen* found = FromIfExists(document))
    return found->fullscreen_element_stack_.size();
  return 0;
}

Element* Fullscreen::FullscreenElementForBindingFrom(TreeScope& scope) {
  Element* element = FullscreenElementFrom(scope.GetDocument());
  if (!element || !RuntimeEnabledFeatures::FullscreenUnprefixedEnabled())
    return element;

  // TODO(kochi): Once V0 code is removed, we can use the same logic for
  // Document and ShadowRoot.
  if (!scope.RootNode().IsShadowRoot()) {
    // For Shadow DOM V0 compatibility: We allow returning an element in V0
    // shadow tree, even though it leaks the Shadow DOM.
    if (element->IsInV0ShadowTree()) {
      UseCounter::Count(scope.GetDocument(),
                        WebFeature::kDocumentFullscreenElementInV0Shadow);
      return element;
    }
  } else if (!ToShadowRoot(scope.RootNode()).IsV1()) {
    return nullptr;
  }
  return scope.AdjustedElement(*element);
}

bool Fullscreen::IsInFullscreenElementStack(const Element& element) {
  const Fullscreen* found = FromIfExists(element.GetDocument());
  if (!found)
    return false;
  for (size_t i = 0; i < found->fullscreen_element_stack_.size(); ++i) {
    if (found->fullscreen_element_stack_[i].first.Get() == &element)
      return true;
  }
  return false;
}

Fullscreen::Fullscreen(Document& document)
    : Supplement<Document>(document),
      ContextLifecycleObserver(&document),
      full_screen_layout_object_(nullptr) {
  document.SetHasFullscreenSupplement();
}

Fullscreen::~Fullscreen() = default;

Document* Fullscreen::GetDocument() {
  return ToDocument(LifecycleContext());
}

void Fullscreen::ContextDestroyed(ExecutionContext*) {
  if (full_screen_layout_object_)
    full_screen_layout_object_->Destroy();

  pending_requests_.clear();
  fullscreen_element_stack_.clear();
}

// https://fullscreen.spec.whatwg.org/#dom-element-requestfullscreen
void Fullscreen::RequestFullscreen(Element& pending) {
  // TODO(foolip): Make RequestType::Unprefixed the default when the unprefixed
  // API is enabled. https://crbug.com/383813
  RequestFullscreen(pending, FullscreenOptions(), RequestType::kPrefixed);
}

void Fullscreen::RequestFullscreen(Element& pending,
                                   const FullscreenOptions& options,
                                   RequestType request_type) {
  RequestFullscreenScope scope;

  // 1. Let |pending| be the context object.

  // 2. Let |pendingDoc| be |pending|'s node document.
  Document& document = pending.GetDocument();

  // 3. Let |promise| be a new promise.
  // TODO(foolip): Promises. https://crbug.com/644637

  // 4. If |pendingDoc| is not fully active, then reject |promise| with a
  // TypeError exception and return |promise|.
  if (!document.IsActive() || !document.GetFrame())
    return;

  bool for_cross_process_descendant =
      request_type == RequestType::kPrefixedForCrossProcessDescendant;

  // Use counters only need to be incremented in the process of the actual
  // fullscreen element.
  if (!for_cross_process_descendant) {
    if (document.IsSecureContext()) {
      UseCounter::Count(document, WebFeature::kFullscreenSecureOrigin);
    } else {
      UseCounter::Count(document, WebFeature::kFullscreenInsecureOrigin);
      HostsUsingFeatures::CountAnyWorld(
          document, HostsUsingFeatures::Feature::kFullscreenInsecureHost);
    }
  }

  // 5. Let |error| be false.
  bool error = false;

  // 6. If any of the following conditions are false, then set |error| to true:
  //
  // OOPIF: If |RequestFullscreen()| was already called in a descendant frame
  // and passed the checks, do not check again here.
  if (!for_cross_process_descendant &&
      !RequestFullscreenConditionsMet(pending, document))
    error = true;

  // 7. Return |promise|, and run the remaining steps in parallel.

  // 8. If |error| is false: Resize |pendingDoc|'s top-level browsing context's
  // document's viewport's dimensions to match the dimensions of the screen of
  // the output device. Optionally display a message how the end user can
  // revert this.
  if (!error) {
    if (From(document).pending_requests_.size()) {
      UseCounter::Count(document,
                        WebFeature::kFullscreenRequestWithPendingElement);
    }

    From(document).pending_requests_.push_back(
        std::make_pair(&pending, request_type));
    LocalFrame& frame = *document.GetFrame();
    frame.GetChromeClient().EnterFullscreen(frame, options);
  } else {
    // Note: Although we are past the "in parallel" point, it's OK to continue
    // synchronously because when |error| is true, |ContinueRequestFullscreen()|
    // will only queue a task and return. This is indistinguishable from, e.g.,
    // enqueueing a microtask to continue at step 9.
    ContinueRequestFullscreen(document, pending, request_type,
                              true /* error */);
  }
}

void Fullscreen::DidEnterFullscreen() {
  if (!GetDocument())
    return;

  // We may be called synchronously from within
  // |FullscreenController::EnterFullscreen()| if we were already fullscreen,
  // but must still not synchronously change the fullscreen element. Instead
  // enqueue a microtask to continue.
  if (RequestFullscreenScope::RunningRequestFullscreen()) {
    Microtask::EnqueueMicrotask(
        WTF::Bind(&Fullscreen::DidEnterFullscreen, WrapPersistent(this)));
    return;
  }

  ElementStack requests;
  requests.swap(pending_requests_);
  for (const ElementStackEntry& request : requests) {
    ContinueRequestFullscreen(*GetDocument(), *request.first, request.second,
                              false /* error */);
  }
}

void Fullscreen::ContinueRequestFullscreen(Document& document,
                                           Element& pending,
                                           RequestType request_type,
                                           bool error) {
  DCHECK(document.IsActive());
  DCHECK(document.GetFrame());

  // 9. If any of the following conditions are false, then set |error| to true:
  //     * |pending|'s node document is |pendingDoc|.
  //     * The fullscreen element ready check for |pending| returns true.
  if (pending.GetDocument() != document || !FullscreenElementReady(pending))
    error = true;

  // 10. If |error| is true:
  if (error) {
    // 10.1. Append (fullscreenerror, |pending|) to |pendingDoc|'s list of
    // pending fullscreen events.
    EnqueueEvent(EventTypeNames::fullscreenerror, pending, document,
                 request_type);

    // 10.2. Reject |promise| with a TypeError exception and terminate these
    // steps.
    return;
  }

  // 11. Let |fullscreenElements| be an ordered set initially consisting of
  // |pending|.
  HeapVector<Member<Element>> fullscreen_elements;
  fullscreen_elements.push_back(pending);

  // 12. While the first element in |fullscreenElements| is in a nested browsing
  // context: append its browsing context container to |fullscreenElements|.
  //
  // OOPIF: |fullscreenElements| will only contain elements for local ancestors,
  // and remote ancestors will be processed in their respective processes. This
  // preserves the spec's event firing order for local ancestors, but not for
  // remote ancestors. However, that difference shouldn't be observable in
  // practice: a fullscreenchange event handler would need to postMessage a
  // frame in another renderer process, where the message should be queued up
  // and processed after the IPC that dispatches fullscreenchange.
  for (Frame* frame = pending.GetDocument().GetFrame(); frame;
       frame = frame->Tree().Parent()) {
    if (!frame->Owner() || !frame->Owner()->IsLocal())
      continue;
    Element* element = ToHTMLFrameOwnerElement(frame->Owner());
    fullscreen_elements.push_back(element);
  }

  // 13. For each |element| in |fullscreenElements|:
  for (Element* element : fullscreen_elements) {
    // 13.1. Let |doc| be |element|'s node document.
    Document& doc = element->GetDocument();

    // 13.2. If |element| is |doc|'s fullscreen element, continue.
    if (element == FullscreenElementFrom(doc))
      continue;

    // 13.3. If |element| is |pending| and |pending| is an iframe element, set
    // |element|'s iframe fullscreen flag.
    // TODO(foolip): Support the iframe fullscreen flag.
    // https://crbug.com/644695

    // 13.4. Fullscreen |element| within |doc|.
    // TODO(foolip): Merge fullscreen element stack into top layer.
    // https://crbug.com/627790
    From(doc).PushFullscreenElementStack(*element, request_type);

    // 13.5. Append (fullscreenchange, |element|) to |doc|'s list of pending
    // fullscreen events.
    EnqueueEvent(EventTypeNames::fullscreenchange, *element, doc, request_type);
  }

  // 14. Resolve |promise| with undefined.
}

// https://fullscreen.spec.whatwg.org/#fully-exit-fullscreen
void Fullscreen::FullyExitFullscreen(Document& document) {
  // TODO(foolip): The spec used to have a first step saying "Let |doc| be the
  // top-level browsing context's document" which was removed in
  // https://github.com/whatwg/fullscreen/commit/3243119d027a8ff5b80998eb1f17f8eba148a346.
  // Remove it here as well.
  Document& doc = TopmostLocalAncestor(document);

  // 1. If |document|'s fullscreen element is null, terminate these steps.
  if (!FullscreenElementFrom(doc))
    return;

  // 2. Unfullscreen elements whose fullscreen flag is set, within
  // |document|'s top layer, except for |document|'s fullscreen element.
  size_t stack_size = From(doc).fullscreen_element_stack_.size();
  From(doc).fullscreen_element_stack_.EraseAt(0, stack_size - 1);
  DCHECK_EQ(From(doc).fullscreen_element_stack_.size(), 1u);

  // 3. Exit fullscreen |document|.
  ExitFullscreen(doc);
}

// https://fullscreen.spec.whatwg.org/#exit-fullscreen
void Fullscreen::ExitFullscreen(Document& doc) {
  // 1. Let |promise| be a new promise.
  // TODO(foolip): Promises. https://crbug.com/644637

  // 2. If |doc| is not fully active or |doc|'s fullscreen element is null, then
  // reject |promise| with a TypeError exception and return |promise|.
  if (!doc.IsActive() || !doc.GetFrame() || !FullscreenElementFrom(doc))
    return;

  // 3. Let |resize| be false.
  bool resize = false;

  // 4. Let |docs| be the result of collecting documents to unfullscreen given
  // |doc|.
  HeapVector<Member<Document>> docs = CollectDocumentsToUnfullscreen(doc);

  // 5. Let |topLevelDoc| be |doc|'s top-level browsing context's active
  // document.
  //
  // OOPIF: Let |topLevelDoc| be the topmost local ancestor instead. If the main
  // frame is in another process, we will still fully exit fullscreen even
  // though that's wrong if the main frame was in nested fullscreen.
  // TODO(alexmos): Deal with nested fullscreen cases, see
  // https://crbug.com/617369.
  Document& top_level_doc = TopmostLocalAncestor(doc);

  // 6. If |topLevelDoc| is in |docs|, and it is a simple fullscreen document,
  // then set |doc| to |topLevelDoc| and |resize| to true.
  //
  // Note: |doc| is not set here, but |doc| will be the topmost local ancestor
  // in |Fullscreen::ContinueExitFullscreen| if |resize| is true.
  if (!docs.IsEmpty() && docs.back() == &top_level_doc &&
      IsSimpleFullscreenDocument(top_level_doc)) {
    resize = true;
  }

  // 7. Return |promise|, and run the remaining steps in parallel.

  // 8. If |resize| is true, resize |doc|'s viewport to its "normal" dimensions.
  if (resize) {
    LocalFrame& frame = *doc.GetFrame();
    frame.GetChromeClient().ExitFullscreen(frame);
  } else {
    // Note: We are past the "in parallel" point, and |ContinueExitFullscreen()|
    // will change script-observable state (document.fullscreenElement)
    // synchronously, so we have to continue asynchronously.
    Microtask::EnqueueMicrotask(WTF::Bind(
        ContinueExitFullscreen, WrapWeakPersistent(&doc), false /* resize */));
  }
}

void Fullscreen::DidExitFullscreen() {
  ContinueExitFullscreen(GetDocument(), true /* resize */);
}

void Fullscreen::ContinueExitFullscreen(Document* doc, bool resize) {
  if (!doc || !doc->IsActive() || !doc->GetFrame())
    return;

  if (resize) {
    // See comment for step 6.
    DCHECK_EQ(nullptr, NextLocalAncestor(*doc));
  }

  // 9. If |doc|'s fullscreen element is null, then resolve |promise| with
  // undefined and terminate these steps.
  if (!FullscreenElementFrom(*doc))
    return;

  // 10. Let |exitDocs| be the result of collecting documents to unfullscreen
  // given |doc|.
  HeapVector<Member<Document>> exit_docs = CollectDocumentsToUnfullscreen(*doc);

  // 11. Let |descendantDocs| be an ordered set consisting of |doc|'s
  // descendant browsing contexts' documents whose fullscreen element is
  // non-null, if any, in tree order.
  HeapVector<Member<Document>> descendant_docs;
  for (Frame* descendant = doc->GetFrame()->Tree().FirstChild(); descendant;
       descendant = descendant->Tree().TraverseNext(doc->GetFrame())) {
    if (!descendant->IsLocalFrame())
      continue;
    DCHECK(ToLocalFrame(descendant)->GetDocument());
    if (FullscreenElementFrom(*ToLocalFrame(descendant)->GetDocument()))
      descendant_docs.push_back(ToLocalFrame(descendant)->GetDocument());
  }

  // 12. For each |exitDoc| in |exitDocs|:
  for (Document* exit_doc : exit_docs) {
    Fullscreen& fullscreen = From(*exit_doc);
    ElementStack& stack = fullscreen.fullscreen_element_stack_;
    DCHECK(!stack.IsEmpty());

    // 12.1. Append (fullscreenchange, |exitDoc|'s fullscreen element) to
    // |exitDoc|'s list of pending fullscreen events.
    EnqueueEvent(EventTypeNames::fullscreenchange, *stack.back().first,
                 *exit_doc, stack.back().second);

    // 12.2. If |resize| is true, unfullscreen |exitDoc|.
    // 12.3. Otherwise, unfullscreen |exitDoc|'s fullscreen element.
    if (resize) {
      while (!stack.IsEmpty())
        fullscreen.PopFullscreenElementStack();
    } else {
      fullscreen.PopFullscreenElementStack();
    }
  }

  // 13. For each |descendantDoc| in |descendantDocs|:
  for (Document* descendant_doc : descendant_docs) {
    Fullscreen& fullscreen = From(*descendant_doc);
    ElementStack& stack = fullscreen.fullscreen_element_stack_;
    DCHECK(!stack.IsEmpty());

    // 13.1. Append (fullscreenchange, |descendantDoc|'s fullscreen element) to
    // |descendantDoc|'s list of pending fullscreen events.
    EnqueueEvent(EventTypeNames::fullscreenchange, *stack.back().first,
                 *descendant_doc, stack.back().second);

    // 13.2. Unfullscreen |descendantDoc|.
    while (!stack.IsEmpty())
      fullscreen.PopFullscreenElementStack();
  }

  // 14. Resolve |promise| with undefined.
}

// https://fullscreen.spec.whatwg.org/#dom-document-fullscreenenabled
bool Fullscreen::FullscreenEnabled(Document& document) {
  // The fullscreenEnabled attribute's getter must return true if the context
  // object is allowed to use the feature indicated by attribute name
  // allowfullscreen and fullscreen is supported, and false otherwise.
  return AllowedToUseFullscreen(document.GetFrame()) &&
         FullscreenIsSupported(document);
}

void Fullscreen::SetFullScreenLayoutObject(LayoutFullScreen* layout_object) {
  if (layout_object == full_screen_layout_object_)
    return;

  if (layout_object && saved_placeholder_computed_style_) {
    layout_object->CreatePlaceholder(
        std::move(saved_placeholder_computed_style_),
        saved_placeholder_frame_rect_);
  } else if (layout_object && full_screen_layout_object_ &&
             full_screen_layout_object_->Placeholder()) {
    LayoutBlockFlow* placeholder = full_screen_layout_object_->Placeholder();
    layout_object->CreatePlaceholder(
        ComputedStyle::Clone(placeholder->StyleRef()),
        placeholder->FrameRect());
  }

  if (full_screen_layout_object_)
    full_screen_layout_object_->UnwrapLayoutObject();
  DCHECK(!full_screen_layout_object_);

  full_screen_layout_object_ = layout_object;
}

void Fullscreen::FullScreenLayoutObjectDestroyed() {
  full_screen_layout_object_ = nullptr;
}

void Fullscreen::ElementRemoved(Element& node) {
  DCHECK_EQ(GetDocument(), &node.GetDocument());

  // |Fullscreen::ElementRemoved()| is called for each removed element, so only
  // the body of the spec "removing steps" loop appears here:

  // 2.1. If |node| is its node document's fullscreen element, exit fullscreen
  // that document.
  if (FullscreenElement() == &node) {
    ExitFullscreen(node.GetDocument());
    return;
  }

  // 2.2. Otherwise, unfullscreen |node| within its node document.
  for (size_t i = 0; i < fullscreen_element_stack_.size(); ++i) {
    if (fullscreen_element_stack_[i].first.Get() == &node) {
      fullscreen_element_stack_.EraseAt(i);
      return;
    }
  }

  // Note: |node| was not in the fullscreen element stack.
}

void Fullscreen::PopFullscreenElementStack() {
  DCHECK(!fullscreen_element_stack_.IsEmpty());

  Element* old_element = FullscreenElement();
  fullscreen_element_stack_.pop_back();

  // Note: |requestType| is only used if |FullscreenElement()| is non-null.
  RequestType request_type = fullscreen_element_stack_.IsEmpty()
                                 ? RequestType::kUnprefixed
                                 : fullscreen_element_stack_.back().second;
  FullscreenElementChanged(old_element, FullscreenElement(), request_type);
}

void Fullscreen::PushFullscreenElementStack(Element& element,
                                            RequestType request_type) {
  Element* old_element = FullscreenElement();
  fullscreen_element_stack_.push_back(std::make_pair(&element, request_type));

  FullscreenElementChanged(old_element, &element, request_type);
}

void Fullscreen::FullscreenElementChanged(Element* old_element,
                                          Element* new_element,
                                          RequestType new_request_type) {
  DCHECK_NE(old_element, new_element);

  if (!GetDocument())
    return;

  GetDocument()->GetStyleEngine().EnsureUAStyleForFullscreen();

  if (full_screen_layout_object_)
    full_screen_layout_object_->UnwrapLayoutObject();
  DCHECK(!full_screen_layout_object_);

  if (old_element) {
    DCHECK_NE(old_element, FullscreenElement());

    old_element->PseudoStateChanged(CSSSelector::kPseudoFullScreen);
    old_element->PseudoStateChanged(CSSSelector::kPseudoFullscreen);

    old_element->SetContainsFullScreenElement(false);
    old_element->SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(
        false);
  }

  if (new_element) {
    DCHECK_EQ(new_element, FullscreenElement());

    new_element->PseudoStateChanged(CSSSelector::kPseudoFullScreen);
    new_element->PseudoStateChanged(CSSSelector::kPseudoFullscreen);

    // OOPIF: For RequestType::PrefixedForCrossProcessDescendant, |toElement| is
    // the iframe element for the out-of-process frame that contains the
    // fullscreen element. Hence, it must match :-webkit-full-screen-ancestor.
    if (new_request_type == RequestType::kPrefixedForCrossProcessDescendant) {
      DCHECK(IsHTMLIFrameElement(new_element));
      new_element->SetContainsFullScreenElement(true);
    }
    new_element->SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(
        true);

    // Create a placeholder block for the fullscreen element, to keep the page
    // from reflowing when the element is removed from the normal flow. Only do
    // this for a LayoutBox, as only a box will have a frameRect. The
    // placeholder will be created in setFullScreenLayoutObject() during layout.
    LayoutObject* layout_object = new_element->GetLayoutObject();
    bool should_create_placeholder = layout_object && layout_object->IsBox();
    if (should_create_placeholder) {
      saved_placeholder_frame_rect_ = ToLayoutBox(layout_object)->FrameRect();
      saved_placeholder_computed_style_ =
          ComputedStyle::Clone(layout_object->StyleRef());
    }

    if (new_element != GetDocument()->documentElement()) {
      LayoutFullScreen::WrapLayoutObject(
          layout_object, layout_object ? layout_object->Parent() : nullptr,
          GetDocument());
    }
  }

  if (LocalFrame* frame = GetDocument()->GetFrame()) {
    // TODO(foolip): Synchronize hover state changes with animation frames.
    // https://crbug.com/668758
    frame->GetEventHandler().ScheduleHoverStateUpdate();
    frame->GetChromeClient().FullscreenElementChanged(old_element, new_element);

    // Descendant frames may have been inert because their owner iframes were
    // outside of fullscreen element. SetIsInert recurses through subframes to
    // propagate the inert bit as needed.
    frame->SetIsInert(GetDocument()->LocalOwner() &&
                      GetDocument()->LocalOwner()->IsInert());
  }

  // TODO(foolip): This should not call |UpdateStyleAndLayoutTree()|.
  GetDocument()->UpdateStyleAndLayoutTree();

  // Any element not contained by the fullscreen element is inert (see
  // |Node::IsInert()|), so changing the fullscreen element will typically
  // change the inertness of most elements. Clear the entire cache.
  GetDocument()->ClearAXObjectCache();
}

void Fullscreen::Trace(blink::Visitor* visitor) {
  visitor->Trace(pending_requests_);
  visitor->Trace(fullscreen_element_stack_);
  Supplement<Document>::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
