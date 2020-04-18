/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"

#include <limits>

#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/exported/web_plugin_container_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/remote_frame_view.h"
#include "third_party/blink/renderer/core/geometry/dom_rect_read_only.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_entry.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_controller.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/length.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

namespace {

using PluginSet = PersistentHeapHashSet<Member<WebPluginContainerImpl>>;
PluginSet& PluginsPendingDispose() {
  DEFINE_STATIC_LOCAL(PluginSet, set, ());
  return set;
}

bool DoesParentAllowLazyLoadingChildren(Document& document) {
  LocalFrame* containing_frame = document.GetFrame();
  if (!containing_frame)
    return true;

  // If the embedding document has no owner, then by default allow lazy loading
  // children.
  FrameOwner* containing_frame_owner = containing_frame->Owner();
  if (!containing_frame_owner)
    return true;

  return containing_frame_owner->ShouldLazyLoadChildren();
}

// Determine if the |bounding_client_rect| for a frame indicates that the frame
// is probably hidden according to some experimental heuristics. Since hidden
// frames are often used for analytics or communication, and lazily loading them
// could break their functionality, so these heuristics are used to recognize
// likely hidden frames and immediately load them so that they can function
// properly.
bool IsFrameProbablyHidden(const DOMRectReadOnly& bounding_client_rect) {
  // Tiny frames that are 4x4 or smaller are likely not intended to be seen by
  // the user. Note that this condition includes frames marked as
  // "display:none", since those frames would have dimensions of 0x0.
  if (bounding_client_rect.width() < 4.1 || bounding_client_rect.height() < 4.1)
    return true;

  // Frames that are positioned completely off the page above or to the left are
  // likely never intended to be visible to the user.
  if (bounding_client_rect.right() < 0.0 || bounding_client_rect.bottom() < 0.0)
    return true;

  return false;
}

}  // namespace

SubframeLoadingDisabler::SubtreeRootSet&
SubframeLoadingDisabler::DisabledSubtreeRoots() {
  DEFINE_STATIC_LOCAL(SubtreeRootSet, nodes, ());
  return nodes;
}

// static
int HTMLFrameOwnerElement::PluginDisposeSuspendScope::suspend_count_ = 0;

void HTMLFrameOwnerElement::PluginDisposeSuspendScope::
    PerformDeferredPluginDispose() {
  DCHECK_EQ(suspend_count_, 1);
  suspend_count_ = 0;

  PluginSet dispose_set;
  PluginsPendingDispose().swap(dispose_set);
  for (const auto& plugin : dispose_set) {
    plugin->Dispose();
  }
}

HTMLFrameOwnerElement::HTMLFrameOwnerElement(const QualifiedName& tag_name,
                                             Document& document)
    : HTMLElement(tag_name, document),
      content_frame_(nullptr),
      embedded_content_view_(nullptr),
      sandbox_flags_(kSandboxNone),
      should_lazy_load_children_(DoesParentAllowLazyLoadingChildren(document)) {
}

LayoutEmbeddedContent* HTMLFrameOwnerElement::GetLayoutEmbeddedContent() const {
  // HTMLObjectElement and HTMLEmbedElement may return arbitrary layoutObjects
  // when using fallback content.
  if (!GetLayoutObject() || !GetLayoutObject()->IsLayoutEmbeddedContent())
    return nullptr;
  return ToLayoutEmbeddedContent(GetLayoutObject());
}

void HTMLFrameOwnerElement::SetContentFrame(Frame& frame) {
  // Make sure we will not end up with two frames referencing the same owner
  // element.
  DCHECK(!content_frame_ || content_frame_->Owner() != this);
  // Disconnected frames should not be allowed to load.
  DCHECK(isConnected());

  // There should be no lazy load in progress since before SetContentFrame,
  // |this| frame element should have been disconnected.
  DCHECK(!lazy_load_intersection_observer_);

  content_frame_ = &frame;

  SetNeedsStyleRecalc(kLocalStyleChange, StyleChangeReasonForTracing::Create(
                                             StyleChangeReason::kFrame));

  for (ContainerNode* node = this; node; node = node->ParentOrShadowHostNode())
    node->IncrementConnectedSubframeCount();
}

void HTMLFrameOwnerElement::ClearContentFrame() {
  if (!content_frame_)
    return;

  // It's possible for there to be a lazy load in progress right now if
  // Frame::Detach() was called without
  // HTMLFrameOwnerElement::DisconnectContentFrame() being called first, so
  // cancel any pending lazy load here.
  // TODO(dcheng): Change this back to a DCHECK asserting that no lazy load is
  // in progress once https://crbug.com/773683 is fixed.
  CancelPendingLazyLoad();

  DCHECK_EQ(content_frame_->Owner(), this);
  content_frame_ = nullptr;

  for (ContainerNode* node = this; node; node = node->ParentOrShadowHostNode())
    node->DecrementConnectedSubframeCount();
}

void HTMLFrameOwnerElement::DisconnectContentFrame() {
  if (!ContentFrame())
    return;

  CancelPendingLazyLoad();

  // Removing a subframe that was still loading can impact the result of
  // AllDescendantsAreComplete that is consulted by Document::ShouldComplete.
  // Therefore we might need to re-check this after removing the subframe.  The
  // re-check is not needed for local frames (which will handle re-checking from
  // FrameLoader::DidFinishNavigation that responds to LocalFrame::Detach).
  // OTOH, re-checking is required for OOPIFs - see https://crbug.com/779433.
  Document& parent_doc = GetDocument();
  bool have_to_check_if_parent_is_completed = !parent_doc.IsLoadCompleted() &&
                                              ContentFrame()->IsRemoteFrame() &&
                                              ContentFrame()->IsLoading();

  // FIXME: Currently we don't do this in removedFrom because this causes an
  // unload event in the subframe which could execute script that could then
  // reach up into this document and then attempt to look back down. We should
  // see if this behavior is really needed as Gecko does not allow this.
  ContentFrame()->Detach(FrameDetachType::kRemove);

  // Check if removing the subframe caused |parent_doc| to finish loading.
  if (have_to_check_if_parent_is_completed)
    parent_doc.CheckCompleted();
}

HTMLFrameOwnerElement::~HTMLFrameOwnerElement() {
  // An owner must by now have been informed of detachment
  // when the frame was closed.
  DCHECK(!content_frame_);
}

Document* HTMLFrameOwnerElement::contentDocument() const {
  return (content_frame_ && content_frame_->IsLocalFrame())
             ? ToLocalFrame(content_frame_)->GetDocument()
             : nullptr;
}

DOMWindow* HTMLFrameOwnerElement::contentWindow() const {
  return content_frame_ ? content_frame_->DomWindow() : nullptr;
}

void HTMLFrameOwnerElement::SetSandboxFlags(SandboxFlags flags) {
  sandbox_flags_ = flags;
  // Recalculate the container policy in case the allow-same-origin flag has
  // changed.
  container_policy_ = ConstructContainerPolicy(nullptr);

  // Don't notify about updates if ContentFrame() is null, for example when
  // the subframe hasn't been created yet.
  if (ContentFrame()) {
    GetDocument().GetFrame()->Client()->DidChangeFramePolicy(
        ContentFrame(), sandbox_flags_, container_policy_);
  }
}

bool HTMLFrameOwnerElement::IsKeyboardFocusable() const {
  return content_frame_ && HTMLElement::IsKeyboardFocusable();
}

void HTMLFrameOwnerElement::DisposePluginSoon(WebPluginContainerImpl* plugin) {
  if (PluginDisposeSuspendScope::suspend_count_) {
    PluginsPendingDispose().insert(plugin);
    PluginDisposeSuspendScope::suspend_count_ |= 1;
  } else
    plugin->Dispose();
}

void HTMLFrameOwnerElement::UpdateContainerPolicy(Vector<String>* messages) {
  container_policy_ = ConstructContainerPolicy(messages);
  // Don't notify about updates if ContentFrame() is null, for example when
  // the subframe hasn't been created yet.
  if (ContentFrame()) {
    GetDocument().GetFrame()->Client()->DidChangeFramePolicy(
        ContentFrame(), sandbox_flags_, container_policy_);
  }
}

void HTMLFrameOwnerElement::FrameOwnerPropertiesChanged() {
  // Don't notify about updates if ContentFrame() is null, for example when
  // the subframe hasn't been created yet.
  if (ContentFrame()) {
    GetDocument().GetFrame()->Client()->DidChangeFrameOwnerProperties(this);
  }
}

void HTMLFrameOwnerElement::AddResourceTiming(const ResourceTimingInfo& info) {
  // Resource timing info should only be reported if the subframe is attached.
  DCHECK(ContentFrame() && ContentFrame()->IsLocalFrame());
  DOMWindowPerformance::performance(*GetDocument().domWindow())
      ->GenerateAndAddResourceTiming(info, localName());
}

void HTMLFrameOwnerElement::DispatchLoad() {
  DispatchScopedEvent(Event::Create(EventTypeNames::load));
}

const ParsedFeaturePolicy& HTMLFrameOwnerElement::ContainerPolicy() const {
  return container_policy_;
}

Document* HTMLFrameOwnerElement::getSVGDocument(
    ExceptionState& exception_state) const {
  Document* doc = contentDocument();
  if (doc && doc->IsSVGDocument())
    return doc;
  return nullptr;
}

void HTMLFrameOwnerElement::SetEmbeddedContentView(
    EmbeddedContentView* embedded_content_view) {
  if (embedded_content_view == embedded_content_view_)
    return;

  Document* doc = contentDocument();
  if (doc && doc->GetFrame()) {
    bool will_be_display_none = !embedded_content_view;
    if (IsDisplayNone() != will_be_display_none) {
      doc->WillChangeFrameOwnerProperties(
          MarginWidth(), MarginHeight(), ScrollingMode(), will_be_display_none);
    }
  }

  if (embedded_content_view_) {
    if (embedded_content_view_->IsAttached()) {
      embedded_content_view_->DetachFromLayout();
      if (embedded_content_view_->IsPluginView())
        DisposePluginSoon(ToWebPluginContainerImpl(embedded_content_view_));
      else
        embedded_content_view_->Dispose();
    }
  }

  embedded_content_view_ = embedded_content_view;
  FrameOwnerPropertiesChanged();

  GetDocument().GetRootScrollerController().DidUpdateIFrameFrameView(*this);

  LayoutEmbeddedContent* layout_embedded_content =
      ToLayoutEmbeddedContent(GetLayoutObject());
  if (!layout_embedded_content)
    return;

  if (embedded_content_view_) {
    // TODO(crbug.com/729196): Trace why LocalFrameView::DetachFromLayout
    // crashes.  Perhaps view is getting reattached while document is shutting
    // down.
    if (doc) {
      CHECK_NE(doc->Lifecycle().GetState(), DocumentLifecycle::kStopping);
    }
    layout_embedded_content->UpdateOnEmbeddedContentViewChange();

    DCHECK_EQ(GetDocument().View(), layout_embedded_content->GetFrameView());
    DCHECK(layout_embedded_content->GetFrameView());
    embedded_content_view_->AttachToLayout();
  }

  if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache())
    cache->ChildrenChanged(layout_embedded_content);
}

EmbeddedContentView* HTMLFrameOwnerElement::ReleaseEmbeddedContentView() {
  if (!embedded_content_view_)
    return nullptr;
  if (embedded_content_view_->IsAttached())
    embedded_content_view_->DetachFromLayout();
  LayoutEmbeddedContent* layout_embedded_content =
      ToLayoutEmbeddedContent(GetLayoutObject());
  if (layout_embedded_content) {
    if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache())
      cache->ChildrenChanged(layout_embedded_content);
  }
  return embedded_content_view_.Release();
}

bool HTMLFrameOwnerElement::LoadOrRedirectSubframe(
    const KURL& url,
    const AtomicString& frame_name,
    bool replace_current_item) {
  UpdateContainerPolicy();

  if (ContentFrame()) {
    // TODO(sclittle): Support lazily loading frame navigations.
    ContentFrame()->Navigate(GetDocument(), url, replace_current_item,
                             UserGestureStatus::kNone);
    return true;
  }

  if (!SubframeLoadingDisabler::CanLoadFrame(*this))
    return false;

  if (GetDocument().GetFrame()->GetPage()->SubframeCount() >=
      Page::kMaxNumberOfFrames)
    return false;

  LocalFrame* child_frame =
      GetDocument().GetFrame()->Client()->CreateFrame(frame_name, this);
  DCHECK_EQ(ContentFrame(), child_frame);
  if (!child_frame)
    return false;

  ResourceRequest request(url);
  ReferrerPolicy policy = ReferrerPolicyAttribute();
  if (policy != kReferrerPolicyDefault) {
    request.SetHTTPReferrer(SecurityPolicy::GenerateReferrer(
        policy, url, GetDocument().OutgoingReferrer()));
  }

  FrameLoadType child_load_type = kFrameLoadTypeInitialInChildFrame;
  if (!GetDocument().LoadEventFinished() &&
      GetDocument().Loader()->LoadType() ==
          kFrameLoadTypeReloadBypassingCache) {
    child_load_type = kFrameLoadTypeReloadBypassingCache;
    request.SetCacheMode(mojom::FetchCacheMode::kBypassCache);
  }

  // Plug-ins should not load via service workers as plug-ins may have their
  // own origin checking logic that may get confused if service workers respond
  // with resources from another origin.
  // https://w3c.github.io/ServiceWorker/#implementer-concerns
  if (IsPlugin())
    request.SetSkipServiceWorker(true);

  if (RuntimeEnabledFeatures::LazyFrameLoadingEnabled() &&
      should_lazy_load_children_ &&
      // Only http:// or https:// URLs are eligible for lazy loading, excluding
      // URLs like invalid or empty URLs, "about:blank", local file URLs, etc.
      // that it doesn't make sense to lazily load.
      url.ProtocolIsInHTTPFamily() &&
      // Disallow lazy loading if javascript in the embedding document would be
      // able to access the contents of the frame, since in those cases
      // deferring the frame could break the page. Note that this check does not
      // take any possible redirects of |url| into account.
      !GetDocument().GetSecurityOrigin()->CanAccess(
          SecurityOrigin::Create(url).get())) {
    // Don't lazy load subresources inside a lazily loaded frame. This will make
    // it possible for subresources in hidden frames to load that will
    // never be visible, as well as make it so that deferred frames that have
    // multiple layers of iframes inside them can load faster once they're near
    // the viewport or visible.
    should_lazy_load_children_ = false;

    lazy_load_intersection_observer_ = IntersectionObserver::Create(
        {Length(kLazyLoadRootMarginPx, kFixed)},
        {std::numeric_limits<float>::min()}, &GetDocument(),
        WTF::BindRepeating(&HTMLFrameOwnerElement::LoadIfHiddenOrNearViewport,
                           WrapWeakPersistent(this), request, child_load_type));

    lazy_load_intersection_observer_->observe(this);
  } else {
    child_frame->Loader().StartNavigation(
        FrameLoadRequest(&GetDocument(), request), child_load_type);
  }
  return true;
}

void HTMLFrameOwnerElement::LoadIfHiddenOrNearViewport(
    const ResourceRequest& resource_request,
    FrameLoadType frame_load_type,
    const HeapVector<Member<IntersectionObserverEntry>>& entries) {
  DCHECK(!entries.IsEmpty());
  DCHECK_EQ(this, entries.back()->target());

  if (!entries.back()->isIntersecting() &&
      !IsFrameProbablyHidden(*entries.back()->boundingClientRect())) {
    return;
  }

  // The content frame of this element should not have changed, since any
  // pending lazy load should have been already been cancelled in
  // DisconnectContentFrame() if the content frame changes.
  DCHECK(ContentFrame());

  // Note that calling FrameLoader::Load() causes this intersection observer to
  // be disconnected.
  ToLocalFrame(ContentFrame())
      ->Loader()
      .StartNavigation(FrameLoadRequest(&GetDocument(), resource_request),
                       frame_load_type);
}

void HTMLFrameOwnerElement::CancelPendingLazyLoad() {
  if (!lazy_load_intersection_observer_)
    return;
  lazy_load_intersection_observer_->disconnect();
  lazy_load_intersection_observer_.Clear();
}

bool HTMLFrameOwnerElement::ShouldLazyLoadChildren() const {
  return should_lazy_load_children_;
}

void HTMLFrameOwnerElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(content_frame_);
  visitor->Trace(embedded_content_view_);
  visitor->Trace(lazy_load_intersection_observer_);
  HTMLElement::Trace(visitor);
  FrameOwner::Trace(visitor);
}

}  // namespace blink
