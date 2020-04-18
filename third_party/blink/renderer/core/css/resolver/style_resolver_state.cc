/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.
 * All rights reserved.
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

#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"

#include "third_party/blink/renderer/core/animation/css/css_animations.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

StyleResolverState::StyleResolverState(
    Document& document,
    const ElementResolveContext& element_context,
    const ComputedStyle* parent_style,
    const ComputedStyle* layout_parent_style)
    : element_context_(element_context),
      document_(document),
      style_(nullptr),
      parent_style_(parent_style),
      layout_parent_style_(layout_parent_style),
      is_animation_interpolation_map_ready_(false),
      is_animating_custom_properties_(false),
      apply_property_to_regular_style_(true),
      apply_property_to_visited_link_style_(false),
      has_dir_auto_attribute_(false),
      font_builder_(&document),
      element_style_resources_(document, document.DevicePixelRatio()) {
  DCHECK(!!parent_style_ == !!layout_parent_style_);

  if (!parent_style_) {
    parent_style_ = element_context_.ParentStyle();
  }

  if (!layout_parent_style_)
    layout_parent_style_ = element_context_.LayoutParentStyle();

  if (!layout_parent_style_)
    layout_parent_style_ = parent_style_;

  DCHECK(document.IsActive());
}

StyleResolverState::StyleResolverState(Document& document,
                                       Element* element,
                                       const ComputedStyle* parent_style,
                                       const ComputedStyle* layout_parent_style)
    : StyleResolverState(document,
                         element ? ElementResolveContext(*element)
                                 : ElementResolveContext(document),
                         parent_style,
                         layout_parent_style) {}

StyleResolverState::~StyleResolverState() {
  // For performance reasons, explicitly clear HeapVectors and
  // HeapHashMaps to avoid giving a pressure on Oilpan's GC.
  animation_update_.Clear();
}

TreeScope& StyleResolverState::GetTreeScope() const {
  return GetElement() ? GetElement()->GetTreeScope() : GetDocument();
}

void StyleResolverState::SetStyle(scoped_refptr<ComputedStyle> style) {
  // FIXME: Improve RAII of StyleResolverState to remove this function.
  style_ = std::move(style);
  css_to_length_conversion_data_ = CSSToLengthConversionData(
      style_.get(), RootElementStyle(), GetDocument().GetLayoutView(),
      style_->EffectiveZoom());
}

scoped_refptr<ComputedStyle> StyleResolverState::TakeStyle() {
  return std::move(style_);
}

CSSToLengthConversionData StyleResolverState::FontSizeConversionData() const {
  float em = ParentStyle()->SpecifiedFontSize();
  float rem = RootElementStyle() ? RootElementStyle()->SpecifiedFontSize() : 1;
  CSSToLengthConversionData::FontSizes font_sizes(em, rem,
                                                  &ParentStyle()->GetFont());
  CSSToLengthConversionData::ViewportSize viewport_size(
      GetDocument().GetLayoutView());

  return CSSToLengthConversionData(Style(), font_sizes, viewport_size, 1);
}

void StyleResolverState::SetParentStyle(
    scoped_refptr<const ComputedStyle> parent_style) {
  parent_style_ = std::move(parent_style);
}

void StyleResolverState::SetLayoutParentStyle(
    scoped_refptr<const ComputedStyle> parent_style) {
  layout_parent_style_ = std::move(parent_style);
}

void StyleResolverState::CacheUserAgentBorderAndBackground() {
  // LayoutTheme only needs the cached style if it has an appearance,
  // and constructing it is expensive so we avoid it if possible.
  if (!Style()->HasAppearance())
    return;

  cached_ua_style_ = CachedUAStyle::Create(Style());
}

void StyleResolverState::LoadPendingResources() {
  element_style_resources_.LoadPendingResources(Style());
}

const FontDescription& StyleResolverState::ParentFontDescription() const {
  return parent_style_->GetFontDescription();
}

void StyleResolverState::SetZoom(float f) {
  float parent_effective_zoom = ParentStyle()
                                    ? ParentStyle()->EffectiveZoom()
                                    : ComputedStyleInitialValues::InitialZoom();

  style_->SetZoom(f);

  if (style_->SetEffectiveZoom(parent_effective_zoom * f))
    font_builder_.DidChangeEffectiveZoom();
}

void StyleResolverState::SetEffectiveZoom(float f) {
  if (style_->SetEffectiveZoom(f))
    font_builder_.DidChangeEffectiveZoom();
}

void StyleResolverState::SetWritingMode(WritingMode new_writing_mode) {
  if (style_->GetWritingMode() == new_writing_mode) {
    return;
  }
  style_->SetWritingMode(new_writing_mode);
  font_builder_.DidChangeWritingMode();
}

void StyleResolverState::SetTextOrientation(ETextOrientation text_orientation) {
  if (style_->GetTextOrientation() != text_orientation) {
    style_->SetTextOrientation(text_orientation);
    font_builder_.DidChangeTextOrientation();
  }
}

HeapHashMap<CSSPropertyID, Member<const CSSValue>>&
StyleResolverState::ParsedPropertiesForPendingSubstitutionCache(
    const CSSPendingSubstitutionValue& value) const {
  HeapHashMap<CSSPropertyID, Member<const CSSValue>>* map =
      parsed_properties_for_pending_substitution_cache_.at(&value);
  if (!map) {
    map = new HeapHashMap<CSSPropertyID, Member<const CSSValue>>;
    parsed_properties_for_pending_substitution_cache_.Set(&value, map);
  }
  return *map;
}

}  // namespace blink
