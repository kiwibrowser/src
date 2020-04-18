/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
 */

#include "third_party/blink/renderer/core/svg/svg_filter_element.h"

#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_filter.h"
#include "third_party/blink/renderer/core/svg/svg_resource.h"
#include "third_party/blink/renderer/core/svg/svg_tree_scope_resources.h"

namespace blink {

inline SVGFilterElement::SVGFilterElement(Document& document)
    : SVGElement(SVGNames::filterTag, document),
      SVGURIReference(this),
      x_(SVGAnimatedLength::Create(this,
                                   SVGNames::xAttr,
                                   SVGLength::Create(SVGLengthMode::kWidth))),
      y_(SVGAnimatedLength::Create(this,
                                   SVGNames::yAttr,
                                   SVGLength::Create(SVGLengthMode::kHeight))),
      width_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::widthAttr,
                                    SVGLength::Create(SVGLengthMode::kWidth))),
      height_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::heightAttr,
                                    SVGLength::Create(SVGLengthMode::kHeight))),
      filter_units_(SVGAnimatedEnumeration<SVGUnitTypes::SVGUnitType>::Create(
          this,
          SVGNames::filterUnitsAttr,
          SVGUnitTypes::kSvgUnitTypeObjectboundingbox)),
      primitive_units_(
          SVGAnimatedEnumeration<SVGUnitTypes::SVGUnitType>::Create(
              this,
              SVGNames::primitiveUnitsAttr,
              SVGUnitTypes::kSvgUnitTypeUserspaceonuse)) {
  // Spec: If the x/y attribute is not specified, the effect is as if a value of
  // "-10%" were specified.
  x_->SetDefaultValueAsString("-10%");
  y_->SetDefaultValueAsString("-10%");
  // Spec: If the width/height attribute is not specified, the effect is as if a
  // value of "120%" were specified.
  width_->SetDefaultValueAsString("120%");
  height_->SetDefaultValueAsString("120%");

  AddToPropertyMap(x_);
  AddToPropertyMap(y_);
  AddToPropertyMap(width_);
  AddToPropertyMap(height_);
  AddToPropertyMap(filter_units_);
  AddToPropertyMap(primitive_units_);
}

SVGFilterElement::~SVGFilterElement() = default;

DEFINE_NODE_FACTORY(SVGFilterElement)

void SVGFilterElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(x_);
  visitor->Trace(y_);
  visitor->Trace(width_);
  visitor->Trace(height_);
  visitor->Trace(filter_units_);
  visitor->Trace(primitive_units_);
  SVGElement::Trace(visitor);
  SVGURIReference::Trace(visitor);
}

void SVGFilterElement::SvgAttributeChanged(const QualifiedName& attr_name) {
  bool is_xywh = attr_name == SVGNames::xAttr || attr_name == SVGNames::yAttr ||
                 attr_name == SVGNames::widthAttr ||
                 attr_name == SVGNames::heightAttr;
  if (is_xywh)
    UpdateRelativeLengthsInformation();

  if (is_xywh || attr_name == SVGNames::filterUnitsAttr ||
      attr_name == SVGNames::primitiveUnitsAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    InvalidateFilterChain();
    return;
  }

  SVGElement::SvgAttributeChanged(attr_name);
}

LocalSVGResource* SVGFilterElement::AssociatedResource() const {
  return GetTreeScope().EnsureSVGTreeScopedResources().ExistingResourceForId(
      GetIdAttribute());
}

void SVGFilterElement::PrimitiveAttributeChanged(
    SVGFilterPrimitiveStandardAttributes& primitive,
    const QualifiedName& attribute) {
  if (LayoutObject* layout_object = GetLayoutObject()) {
    ToLayoutSVGResourceFilter(layout_object)
        ->PrimitiveAttributeChanged(primitive, attribute);
  } else if (LocalSVGResource* resource = AssociatedResource()) {
    resource->NotifyContentChanged(SVGResourceClient::kPaintInvalidation);
  }
}

void SVGFilterElement::InvalidateFilterChain() {
  if (LayoutObject* layout_object = GetLayoutObject()) {
    ToLayoutSVGResourceFilter(layout_object)->RemoveAllClientsFromCache();
  } else if (LocalSVGResource* resource = AssociatedResource()) {
    resource->NotifyContentChanged(SVGResourceClient::kLayoutInvalidation |
                                   SVGResourceClient::kBoundariesInvalidation);
  }
}

void SVGFilterElement::ChildrenChanged(const ChildrenChange& change) {
  SVGElement::ChildrenChanged(change);

  if (change.by_parser)
    return;

  if (LayoutObject* object = GetLayoutObject())
    object->SetNeedsLayoutAndFullPaintInvalidation(
        LayoutInvalidationReason::kChildChanged);
  InvalidateFilterChain();
}

LayoutObject* SVGFilterElement::CreateLayoutObject(const ComputedStyle&) {
  return new LayoutSVGResourceFilter(this);
}

bool SVGFilterElement::SelfHasRelativeLengths() const {
  return x_->CurrentValue()->IsRelative() || y_->CurrentValue()->IsRelative() ||
         width_->CurrentValue()->IsRelative() ||
         height_->CurrentValue()->IsRelative();
}

}  // namespace blink
