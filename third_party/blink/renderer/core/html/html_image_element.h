/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_IMAGE_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_IMAGE_ELEMENT_H_

#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/create_element_flags.h"
#include "third_party/blink/renderer/core/html/canvas/image_element_base.h"
#include "third_party/blink/renderer/core/html/forms/form_associated.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/html_image_loader.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"

namespace blink {

class HTMLFormElement;
class ImageCandidate;
class ShadowRoot;

class CORE_EXPORT HTMLImageElement final
    : public HTMLElement,
      public ImageElementBase,
      public ActiveScriptWrappable<HTMLImageElement>,
      public FormAssociated {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(HTMLImageElement);

 public:
  class ViewportChangeListener;

  static HTMLImageElement* Create(Document&);
  static HTMLImageElement* Create(Document&, const CreateElementFlags);
  static HTMLImageElement* CreateForJSConstructor(Document&);
  static HTMLImageElement* CreateForJSConstructor(Document&, unsigned width);
  static HTMLImageElement* CreateForJSConstructor(Document&,
                                                  unsigned width,
                                                  unsigned height);

  ~HTMLImageElement() override;
  void Trace(blink::Visitor*) override;

  unsigned width();
  unsigned height();

  unsigned naturalWidth() const;
  unsigned naturalHeight() const;

  unsigned LayoutBoxWidth() const;
  unsigned LayoutBoxHeight() const;

  const String& currentSrc() const;

  bool IsServerMap() const;

  String AltText() const final;

  ImageResourceContent* CachedImage() const {
    return GetImageLoader().GetContent();
  }
  ImageResource* CachedImageResourceForImageDocument() const {
    return GetImageLoader().ImageResourceForImageDocument();
  }
  void SetImageForTest(ImageResourceContent* content) {
    GetImageLoader().SetImageForTest(content);
  }

  void SetLoadingImageDocument() { GetImageLoader().SetLoadingImageDocument(); }

  void setHeight(unsigned);

  KURL Src() const;
  void SetSrc(const String&);

  void setWidth(unsigned);

  int x() const;
  int y() const;

  ScriptPromise decode(ScriptState*, ExceptionState&);

  bool complete() const;

  bool HasPendingActivity() const final {
    return GetImageLoader().HasPendingActivity();
  }

  bool CanContainRangeEndPoint() const override { return false; }

  const AtomicString ImageSourceURL() const override;

  HTMLFormElement* formOwner() const override;
  void FormRemovedFromTree(const Node& form_root);
  virtual void EnsureCollapsedOrFallbackContent();
  virtual void EnsureFallbackForGeneratedContent();
  virtual void EnsurePrimaryContent();
  bool IsCollapsed() const;

  // CanvasImageSource interface implementation.
  FloatSize DefaultDestinationSize(const FloatSize&) const override;

  // public so that HTMLPictureElement can call this as well.
  void SelectSourceURL(ImageLoader::UpdateFromElementBehavior);

  void SetIsFallbackImage() { is_fallback_image_ = true; }

  FetchParameters::ResourceWidth GetResourceWidth();
  float SourceSize(Element&);

  void ForceReload() const;

  FormAssociated* ToFormAssociatedOrNull() override { return this; };
  void AssociateWith(HTMLFormElement*) override;

 protected:
  // Controls how an image element appears in the layout. See:
  // https://html.spec.whatwg.org/multipage/embedded-content.html#image-request
  enum class LayoutDisposition : uint8_t {
    // Displayed as a partially or completely loaded image. Corresponds to the
    // `current request` state being: `unavailable`, `partially available`, or
    // `completely available`.
    kPrimaryContent,
    // Showing a broken image icon and 'alt' text, if any. Corresponds to the
    // `current request` being in the `broken` state.
    kFallbackContent,
    // No layout object. Corresponds to the `current request` being in the
    // `broken` state when the resource load failed with an error that has the
    // |shouldCollapseInitiator| flag set.
    kCollapsed
  };

  explicit HTMLImageElement(Document&, bool created_by_parser = false);

  void DidMoveToNewDocument(Document& old_document) override;

  void DidAddUserAgentShadowRoot(ShadowRoot&) override;
  scoped_refptr<ComputedStyle> CustomStyleForLayoutObject() override;

 private:
  bool AreAuthorShadowsAllowed() const override { return false; }

  void ParseAttribute(const AttributeModificationParams&) override;
  bool IsPresentationAttribute(const QualifiedName&) const override;
  void CollectStyleForPresentationAttribute(
      const QualifiedName&,
      const AtomicString&,
      MutableCSSPropertyValueSet*) override;
  void SetLayoutDisposition(LayoutDisposition, bool force_reattach = false);

  void AttachLayoutTree(AttachContext&) override;
  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;

  bool CanStartSelection() const override { return false; }

  bool IsURLAttribute(const Attribute&) const override;
  bool HasLegalLinkAttribute(const QualifiedName&) const override;
  const QualifiedName& SubResourceAttributeName() const override;

  bool draggable() const override;

  InsertionNotificationRequest InsertedInto(ContainerNode*) override;
  void RemovedFrom(ContainerNode*) override;
  NamedItemType GetNamedItemType() const override {
    return NamedItemType::kNameOrIdWithName;
  }
  bool IsInteractiveContent() const override;
  Image* ImageContents() override;

  void ResetFormOwner();
  ImageCandidate FindBestFitImageFromPictureParent();
  void SetBestFitURLAndDPRFromImageCandidate(const ImageCandidate&);
  LayoutSize DensityCorrectedIntrinsicDimensions() const;
  HTMLImageLoader& GetImageLoader() const override { return *image_loader_; }
  void NotifyViewportChanged();
  void CreateMediaQueryListIfDoesNotExist();

  Member<HTMLImageLoader> image_loader_;
  Member<ViewportChangeListener> listener_;
  Member<HTMLFormElement> form_;
  AtomicString best_fit_image_url_;
  float image_device_pixel_ratio_;
  Member<HTMLSourceElement> source_;
  LayoutDisposition layout_disposition_;
  unsigned form_was_set_by_parser_ : 1;
  unsigned element_created_by_parser_ : 1;
  unsigned is_fallback_image_ : 1;
  bool should_invert_color_;

  ReferrerPolicy referrer_policy_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_IMAGE_ELEMENT_H_
