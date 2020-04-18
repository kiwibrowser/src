/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013 Apple Inc.
 * All rights reserved.
 *           (C) 2007 Eric Seidel (eric@webkit.org)
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

#include "third_party/blink/renderer/core/dom/element.h"

#include <memory>

#include "third_party/blink/public/platform/web_scroll_into_view_params.h"
#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/scroll_into_view_options_or_boolean.h"
#include "third_party/blink/renderer/bindings/core/v8/string_or_trusted_html.h"
#include "third_party/blink/renderer/bindings/core/v8/string_or_trusted_script_url.h"
#include "third_party/blink/renderer/core/animation/css/css_animations.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/css_selector_watch.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/core/css/property_set_css_style_declaration.h"
#include "third_party/blink/renderer/core/css/resolver/selector_filter_parent_scope.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_stats.h"
#include "third_party/blink/renderer/core/css/selector_query.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/dom/attr.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/dom/computed_accessible_node.h"
#include "third_party/blink/renderer/core/dom/dataset_dom_string_map.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_token_list.h"
#include "third_party/blink/renderer/core/dom/element_data_cache.h"
#include "third_party/blink/renderer/core/dom/element_rare_data.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/events/event_dispatcher.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"
#include "third_party/blink/renderer/core/dom/layout_tree_builder.h"
#include "third_party/blink/renderer/core/dom/mutation_observer_interest_group.h"
#include "third_party/blink/renderer/core/dom/mutation_record.h"
#include "third_party/blink/renderer/core/dom/named_node_map.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/dom/presentation_attribute_style.h"
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/shadow_root_init.h"
#include "third_party/blink/renderer/core/dom/shadow_root_v0.h"
#include "third_party/blink/renderer/core/dom/slot_assignment.h"
#include "third_party/blink/renderer/core/dom/space_split_string.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/dom/trustedtypes/trusted_html.h"
#include "third_party/blink/renderer/core/dom/trustedtypes/trusted_script_url.h"
#include "third_party/blink/renderer/core/dom/v0_insertion_point.h"
#include "third_party/blink/renderer/core/dom/whitespace_attacher.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/iterators/text_iterator.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/serializers/serialization.h"
#include "third_party/blink/renderer/core/editing/set_selection_options.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/events/focus_event.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/hosts_using_features.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/scroll_into_view_options.h"
#include "third_party/blink/renderer/core/frame/scroll_to_options.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/geometry/dom_rect_list.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/html/custom/custom_element.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_registry.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element_registration_context.h"
#include "third_party/blink/renderer/core/html/forms/html_form_controls_collection.h"
#include "third_party/blink/renderer/core/html/forms/html_options_collection.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_document.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/html_frame_element_base.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/html_plugin_element.h"
#include "third_party/blink/renderer/core/html/html_slot_element.h"
#include "third_party/blink/renderer/core/html/html_table_rows_collection.h"
#include "third_party/blink/renderer/core/html/html_template_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/intersection_observer/element_intersection_observer_data.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/pointer_lock_controller.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_controller.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_util.h"
#include "third_party/blink/renderer/core/page/scrolling/scroll_customization_callbacks.h"
#include "third_party/blink/renderer/core/page/scrolling/scroll_state.h"
#include "third_party/blink/renderer/core/page/scrolling/scroll_state_callback.h"
#include "third_party/blink/renderer/core/page/scrolling/snap_coordinator.h"
#include "third_party/blink/renderer/core/page/scrolling/top_document_root_scroller_controller.h"
#include "third_party/blink/renderer/core/page/spatial_navigation.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observation.h"
#include "third_party/blink/renderer/core/svg/svg_a_element.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/core/xml_names.h"
#include "third_party/blink/renderer/platform/bindings/dom_data_store.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_activity_logger.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_wrapper.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_context_data.h"
#include "third_party/blink/renderer/platform/event_dispatch_forbidden_scope.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/scroll/smooth_scroll_sequencer.h"
#include "third_party/blink/renderer/platform/wtf/bit_vector.h"
#include "third_party/blink/renderer/platform/wtf/hash_functions.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"

namespace blink {

namespace {

// We need to retain the scroll customization callbacks until the element
// they're associated with is destroyed. It would be simplest if the callbacks
// could be stored in ElementRareData, but we can't afford the space increase.
// Instead, keep the scroll customization callbacks here. The other option would
// be to store these callbacks on the Page or document, but that necessitates a
// bunch more logic for transferring the callbacks between Pages when elements
// are moved around.
ScrollCustomizationCallbacks& GetScrollCustomizationCallbacks() {
  DEFINE_STATIC_LOCAL(ScrollCustomizationCallbacks,
                      scroll_customization_callbacks,
                      (new ScrollCustomizationCallbacks));
  return scroll_customization_callbacks;
}

}  // namespace

using namespace HTMLNames;
using namespace XMLNames;

enum class ClassStringContent { kEmpty, kWhiteSpaceOnly, kHasClasses };

Element* Element::Create(const QualifiedName& tag_name, Document* document) {
  return new Element(tag_name, document, kCreateElement);
}

Element::Element(const QualifiedName& tag_name,
                 Document* document,
                 ConstructionType type)
    : ContainerNode(document, type), tag_name_(tag_name) {}

Element::~Element() {
  DCHECK(NeedsAttach());
}

inline ElementRareData* Element::GetElementRareData() const {
  DCHECK(HasRareData());
  return static_cast<ElementRareData*>(RareData());
}

inline ElementRareData& Element::EnsureElementRareData() {
  return static_cast<ElementRareData&>(EnsureRareData());
}

bool Element::HasElementFlagInternal(ElementFlags mask) const {
  return GetElementRareData()->HasElementFlag(mask);
}

void Element::SetElementFlag(ElementFlags mask, bool value) {
  if (!HasRareData() && !value)
    return;
  EnsureElementRareData().SetElementFlag(mask, value);
}

void Element::ClearElementFlag(ElementFlags mask) {
  if (!HasRareData())
    return;
  GetElementRareData()->ClearElementFlag(mask);
}

void Element::ClearTabIndexExplicitlyIfNeeded() {
  if (HasRareData())
    GetElementRareData()->ClearTabIndexExplicitly();
}

void Element::SetTabIndexExplicitly() {
  EnsureElementRareData().SetTabIndexExplicitly();
}

void Element::setTabIndex(int value) {
  SetIntegralAttribute(tabindexAttr, value);
}

int Element::tabIndex() const {
  return HasElementFlag(ElementFlags::kTabIndexWasSetExplicitly)
             ? GetIntegralAttribute(tabindexAttr)
             : 0;
}

bool Element::IsFocusableStyle() const {
  // Elements in canvas fallback content are not rendered, but they are allowed
  // to be focusable as long as their canvas is displayed and visible.
  if (IsInCanvasSubtree()) {
    const HTMLCanvasElement* canvas =
        Traversal<HTMLCanvasElement>::FirstAncestorOrSelf(*this);
    DCHECK(canvas);
    return canvas->GetLayoutObject() &&
           canvas->GetLayoutObject()->Style()->Visibility() ==
               EVisibility::kVisible;
  }

  // FIXME: Even if we are not visible, we might have a child that is visible.
  // Hyatt wants to fix that some day with a "has visible content" flag or the
  // like.
  return GetLayoutObject() &&
         GetLayoutObject()->Style()->Visibility() == EVisibility::kVisible;
}

Node* Element::Clone(Document& factory, CloneChildrenFlag flag) const {
  return flag == CloneChildrenFlag::kClone ? CloneWithChildren(&factory)
                                           : CloneWithoutChildren(&factory);
}

Element* Element::CloneWithChildren(Document* nullable_factory) const {
  Element* clone = CloneWithoutAttributesAndChildren(
      nullable_factory ? *nullable_factory : GetDocument());
  // This will catch HTML elements in the wrong namespace that are not correctly
  // copied.  This is a sanity check as HTML overloads some of the DOM methods.
  DCHECK_EQ(IsHTMLElement(), clone->IsHTMLElement());

  clone->CloneAttributesFrom(*this);
  clone->CloneNonAttributePropertiesFrom(*this, CloneChildrenFlag::kClone);
  clone->CloneChildNodesFrom(*this);
  return clone;
}

Element* Element::CloneWithoutChildren(Document* nullable_factory) const {
  Element* clone = CloneWithoutAttributesAndChildren(
      nullable_factory ? *nullable_factory : GetDocument());
  // This will catch HTML elements in the wrong namespace that are not correctly
  // copied.  This is a sanity check as HTML overloads some of the DOM methods.
  DCHECK_EQ(IsHTMLElement(), clone->IsHTMLElement());

  clone->CloneAttributesFrom(*this);
  clone->CloneNonAttributePropertiesFrom(*this, CloneChildrenFlag::kSkip);
  return clone;
}

Element* Element::CloneWithoutAttributesAndChildren(Document& factory) const {
  return factory.CreateElement(TagQName(), CreateElementFlags::ByCloneNode(),
                               IsValue());
}

Attr* Element::DetachAttribute(size_t index) {
  DCHECK(GetElementData());
  const Attribute& attribute = GetElementData()->Attributes().at(index);
  Attr* attr_node = AttrIfExists(attribute.GetName());
  if (attr_node) {
    DetachAttrNodeAtIndex(attr_node, index);
  } else {
    attr_node =
        Attr::Create(GetDocument(), attribute.GetName(), attribute.Value());
    RemoveAttributeInternal(index, kNotInSynchronizationOfLazyAttribute);
  }
  return attr_node;
}

void Element::DetachAttrNodeAtIndex(Attr* attr, size_t index) {
  DCHECK(attr);
  DCHECK(GetElementData());

  const Attribute& attribute = GetElementData()->Attributes().at(index);
  DCHECK(attribute.GetName() == attr->GetQualifiedName());
  DetachAttrNodeFromElementWithValue(attr, attribute.Value());
  RemoveAttributeInternal(index, kNotInSynchronizationOfLazyAttribute);
}

void Element::removeAttribute(const QualifiedName& name) {
  if (!GetElementData())
    return;

  size_t index = GetElementData()->Attributes().FindIndex(name);
  if (index == kNotFound)
    return;

  RemoveAttributeInternal(index, kNotInSynchronizationOfLazyAttribute);
}

void Element::SetBooleanAttribute(const QualifiedName& name, bool value) {
  if (value)
    setAttribute(name, g_empty_atom);
  else
    removeAttribute(name);
}

NamedNodeMap* Element::attributesForBindings() const {
  ElementRareData& rare_data =
      const_cast<Element*>(this)->EnsureElementRareData();
  if (NamedNodeMap* attribute_map = rare_data.AttributeMap())
    return attribute_map;

  rare_data.SetAttributeMap(NamedNodeMap::Create(const_cast<Element*>(this)));
  return rare_data.AttributeMap();
}

Vector<AtomicString> Element::getAttributeNames() const {
  Vector<AtomicString> attributesVector;
  if (!hasAttributes())
    return attributesVector;

  AttributeCollection attributes = element_data_->Attributes();
  attributesVector.ReserveInitialCapacity(attributes.size());
  for (const Attribute& attr : attributes)
    attributesVector.UncheckedAppend(attr.GetName().ToString());
  return attributesVector;
}

ElementAnimations* Element::GetElementAnimations() const {
  if (HasRareData())
    return GetElementRareData()->GetElementAnimations();
  return nullptr;
}

ElementAnimations& Element::EnsureElementAnimations() {
  ElementRareData& rare_data = EnsureElementRareData();
  if (!rare_data.GetElementAnimations())
    rare_data.SetElementAnimations(new ElementAnimations());
  return *rare_data.GetElementAnimations();
}

bool Element::HasAnimations() const {
  if (!HasRareData())
    return false;

  ElementAnimations* element_animations =
      GetElementRareData()->GetElementAnimations();
  return element_animations && !element_animations->IsEmpty();
}

Node::NodeType Element::getNodeType() const {
  return kElementNode;
}

bool Element::hasAttribute(const QualifiedName& name) const {
  return hasAttributeNS(name.NamespaceURI(), name.LocalName());
}

void Element::SynchronizeAllAttributes() const {
  if (!GetElementData())
    return;
  // NOTE: anyAttributeMatches in SelectorChecker.cpp
  // currently assumes that all lazy attributes have a null namespace.
  // If that ever changes we'll need to fix that code.
  if (GetElementData()->style_attribute_is_dirty_) {
    DCHECK(IsStyledElement());
    SynchronizeStyleAttributeInternal();
  }
  if (GetElementData()->animated_svg_attributes_are_dirty_)
    ToSVGElement(this)->SynchronizeAnimatedSVGAttribute(AnyQName());
}

inline void Element::SynchronizeAttribute(const QualifiedName& name) const {
  if (!GetElementData())
    return;
  if (UNLIKELY(name == styleAttr &&
               GetElementData()->style_attribute_is_dirty_)) {
    DCHECK(IsStyledElement());
    SynchronizeStyleAttributeInternal();
    return;
  }
  if (UNLIKELY(GetElementData()->animated_svg_attributes_are_dirty_)) {
    // See comment in the AtomicString version of SynchronizeAttribute()
    // also.
    ToSVGElement(this)->SynchronizeAnimatedSVGAttribute(name);
  }
}

void Element::SynchronizeAttribute(const AtomicString& local_name) const {
  // This version of synchronizeAttribute() is streamlined for the case where
  // you don't have a full QualifiedName, e.g when called from DOM API.
  if (!GetElementData())
    return;
  if (GetElementData()->style_attribute_is_dirty_ &&
      LowercaseIfNecessary(local_name) == styleAttr.LocalName()) {
    DCHECK(IsStyledElement());
    SynchronizeStyleAttributeInternal();
    return;
  }
  if (GetElementData()->animated_svg_attributes_are_dirty_) {
    // We're not passing a namespace argument on purpose. SVGNames::*Attr are
    // defined w/o namespaces as well.

    // FIXME: this code is called regardless of whether name is an
    // animated SVG Attribute. It would seem we should only call this method
    // if SVGElement::isAnimatableAttribute is true, but the list of
    // animatable attributes in isAnimatableAttribute does not suffice to
    // pass all layout tests. Also, m_animatedSVGAttributesAreDirty stays
    // dirty unless synchronizeAnimatedSVGAttribute is called with
    // anyQName(). This means that even if Element::synchronizeAttribute()
    // is called on all attributes, m_animatedSVGAttributesAreDirty remains
    // true.
    ToSVGElement(this)->SynchronizeAnimatedSVGAttribute(
        QualifiedName(g_null_atom, local_name, g_null_atom));
  }
}

const AtomicString& Element::getAttribute(const QualifiedName& name) const {
  if (!GetElementData())
    return g_null_atom;
  SynchronizeAttribute(name);
  if (const Attribute* attribute = GetElementData()->Attributes().Find(name))
    return attribute->Value();
  return g_null_atom;
}

AtomicString Element::LowercaseIfNecessary(const AtomicString& name) const {
  return IsHTMLElement() && GetDocument().IsHTMLDocument() ? name.LowerASCII()
                                                           : name;
}

const AtomicString& Element::nonce() const {
  return HasRareData() ? GetElementRareData()->GetNonce() : g_empty_atom;
}

void Element::setNonce(const AtomicString& nonce) {
  EnsureElementRareData().SetNonce(nonce);
}

void Element::scrollIntoView(ScrollIntoViewOptionsOrBoolean arg) {
  ScrollIntoViewOptions options;
  if (arg.IsBoolean()) {
    if (arg.GetAsBoolean())
      options.setBlock("start");
    else
      options.setBlock("end");
    options.setInlinePosition("nearest");
  } else if (arg.IsScrollIntoViewOptions()) {
    options = arg.GetAsScrollIntoViewOptions();
    if (!RuntimeEnabledFeatures::CSSOMSmoothScrollEnabled() &&
        options.behavior() == "smooth") {
      options.setBehavior("instant");
    }
  }
  scrollIntoViewWithOptions(options);
}

void Element::scrollIntoView(bool align_to_top) {
  ScrollIntoViewOptionsOrBoolean arg;
  arg.SetBoolean(align_to_top);
  scrollIntoView(arg);
}

static ScrollAlignment ToPhysicalAlignment(const ScrollIntoViewOptions& options,
                                           ScrollOrientation axis,
                                           bool is_horizontal_writing_mode) {
  String alignment =
      ((axis == kHorizontalScroll && is_horizontal_writing_mode) ||
       (axis == kVerticalScroll && !is_horizontal_writing_mode))
          ? options.inlinePosition()
          : options.block();

  if (alignment == "center")
    return ScrollAlignment::kAlignCenterAlways;
  if (alignment == "nearest")
    return ScrollAlignment::kAlignToEdgeIfNeeded;
  if (alignment == "start") {
    return (axis == kHorizontalScroll) ? ScrollAlignment::kAlignLeftAlways
                                       : ScrollAlignment::kAlignTopAlways;
  }
  if (alignment == "end") {
    return (axis == kHorizontalScroll) ? ScrollAlignment::kAlignRightAlways
                                       : ScrollAlignment::kAlignBottomAlways;
  }

  // Default values
  if (is_horizontal_writing_mode) {
    return (axis == kHorizontalScroll) ? ScrollAlignment::kAlignToEdgeIfNeeded
                                       : ScrollAlignment::kAlignTopAlways;
  }
  return (axis == kHorizontalScroll) ? ScrollAlignment::kAlignLeftAlways
                                     : ScrollAlignment::kAlignToEdgeIfNeeded;
}

void Element::scrollIntoViewWithOptions(const ScrollIntoViewOptions& options) {
  GetDocument().EnsurePaintLocationDataValidForNode(this);

  if (!GetLayoutObject() || !GetDocument().GetPage())
    return;

  // TODO(810510): Move this logic inside "ScrollableArea::SetScrollOffset" and
  // rely on ScrollType to detect js scrolls and set the flag. This requires
  // adding new scroll type to enable this.
  if (GetDocument().Loader())
    GetDocument().Loader()->GetInitialScrollState().was_scrolled_by_js = true;

  ScrollBehavior behavior = (options.behavior() == "smooth")
                                ? kScrollBehaviorSmooth
                                : kScrollBehaviorAuto;

  bool is_horizontal_writing_mode =
      GetComputedStyle()->IsHorizontalWritingMode();
  ScrollAlignment align_x = ToPhysicalAlignment(options, kHorizontalScroll,
                                                is_horizontal_writing_mode);
  ScrollAlignment align_y =
      ToPhysicalAlignment(options, kVerticalScroll, is_horizontal_writing_mode);

  LayoutRect bounds = BoundingBoxForScrollIntoView();
  GetLayoutObject()->ScrollRectToVisible(
      bounds, {align_x, align_y, kProgrammaticScroll, false, behavior});

  GetDocument().SetSequentialFocusNavigationStartingPoint(this);
}

void Element::scrollIntoViewIfNeeded(bool center_if_needed) {
  GetDocument().EnsurePaintLocationDataValidForNode(this);

  if (!GetLayoutObject())
    return;

  // TODO(810510): Move this logic inside "ScrollableArea::SetScrollOffset" and
  // rely on ScrollType to detect js scrolls and set the flag. This requires
  // adding new scroll type to enable this.
  if (GetDocument().Loader())
    GetDocument().Loader()->GetInitialScrollState().was_scrolled_by_js = true;

  LayoutRect bounds = BoundingBoxForScrollIntoView();
  if (center_if_needed) {
    GetLayoutObject()->ScrollRectToVisible(
        bounds,
        {ScrollAlignment::kAlignCenterIfNeeded,
         ScrollAlignment::kAlignCenterIfNeeded, kProgrammaticScroll, false});
  } else {
    GetLayoutObject()->ScrollRectToVisible(
        bounds,
        {ScrollAlignment::kAlignToEdgeIfNeeded,
         ScrollAlignment::kAlignToEdgeIfNeeded, kProgrammaticScroll, false});
  }
}

void Element::setDistributeScroll(V8ScrollStateCallback* scroll_state_callback,
                                  const String& native_scroll_behavior) {
  GetScrollCustomizationCallbacks().SetDistributeScroll(
      this, ScrollStateCallbackV8Impl::Create(scroll_state_callback,
                                              native_scroll_behavior));
}

void Element::setApplyScroll(V8ScrollStateCallback* scroll_state_callback,
                             const String& native_scroll_behavior) {
  SetApplyScroll(ScrollStateCallbackV8Impl::Create(scroll_state_callback,
                                                   native_scroll_behavior));
}

void Element::SetApplyScroll(ScrollStateCallback* scroll_state_callback) {
  GetScrollCustomizationCallbacks().SetApplyScroll(this, scroll_state_callback);
}

void Element::RemoveApplyScroll() {
  GetScrollCustomizationCallbacks().RemoveApplyScroll(this);
}

ScrollStateCallback* Element::GetApplyScroll() {
  return GetScrollCustomizationCallbacks().GetApplyScroll(this);
}

void Element::NativeDistributeScroll(ScrollState& scroll_state) {
  if (scroll_state.FullyConsumed())
    return;

  scroll_state.distributeToScrollChainDescendant();

  // The scroll doesn't propagate, and we're currently scrolling an element
  // other than this one, prevent the scroll from propagating to this element.
  if (scroll_state.DeltaConsumedForScrollSequence() &&
      scroll_state.CurrentNativeScrollingElement() != this) {
    return;
  }

  const double delta_x = scroll_state.deltaX();
  const double delta_y = scroll_state.deltaY();

  CallApplyScroll(scroll_state);

  if (delta_x != scroll_state.deltaX() || delta_y != scroll_state.deltaY())
    scroll_state.SetCurrentNativeScrollingElement(this);
}

void Element::CallDistributeScroll(ScrollState& scroll_state) {
  TRACE_EVENT0("input", "Element::CallDistributeScroll");
  ScrollStateCallback* callback =
      GetScrollCustomizationCallbacks().GetDistributeScroll(this);

  // TODO(bokan): Need to add tests before we allow calling custom callbacks
  // for non-touch modalities. For now, just call into the native callback but
  // allow the viewport scroll callback so we don't disable overscroll.
  // crbug.com/623079.
  bool disable_custom_callbacks = !scroll_state.isDirectManipulation() &&
                                  !GetDocument()
                                       .GetPage()
                                       ->GlobalRootScrollerController()
                                       .IsViewportScrollCallback(callback);

  disable_custom_callbacks |=
      !RootScrollerUtil::IsGlobal(this) &&
      RuntimeEnabledFeatures::ScrollCustomizationEnabled() &&
      !GetScrollCustomizationCallbacks().InScrollPhase(this);

  if (!callback || disable_custom_callbacks) {
    NativeDistributeScroll(scroll_state);
    return;
  }
  if (callback->NativeScrollBehavior() !=
      WebNativeScrollBehavior::kPerformAfterNativeScroll)
    callback->Invoke(&scroll_state);
  if (callback->NativeScrollBehavior() !=
      WebNativeScrollBehavior::kDisableNativeScroll)
    NativeDistributeScroll(scroll_state);
  if (callback->NativeScrollBehavior() ==
      WebNativeScrollBehavior::kPerformAfterNativeScroll)
    callback->Invoke(&scroll_state);
}

void Element::NativeApplyScroll(ScrollState& scroll_state) {
  // All elements in the scroll chain should be boxes.
  DCHECK(!GetLayoutObject() || GetLayoutObject()->IsBox());

  if (scroll_state.FullyConsumed())
    return;

  FloatSize delta(scroll_state.deltaX(), scroll_state.deltaY());

  if (delta.IsZero())
    return;

  // TODO(esprehn): This should use
  // updateStyleAndLayoutIgnorePendingStylesheetsForNode.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  LayoutBox* box_to_scroll = nullptr;

  if (this == GetDocument().documentElement())
    box_to_scroll = GetDocument().GetLayoutView();
  else if (GetLayoutObject())
    box_to_scroll = ToLayoutBox(GetLayoutObject());

  if (!box_to_scroll)
    return;

  ScrollableArea* scrollable_area =
      box_to_scroll->EnclosingBox()->GetScrollableArea();

  if (!scrollable_area)
    return;

  ScrollResult result = scrollable_area->UserScroll(
      ScrollGranularity(static_cast<int>(scroll_state.deltaGranularity())),
      delta);

  if (!result.DidScroll())
    return;

  // FIXME: Native scrollers should only consume the scroll they
  // apply. See crbug.com/457765.
  scroll_state.ConsumeDeltaNative(delta.Width(), delta.Height());

  // We need to setCurrentNativeScrollingElement in both the
  // distributeScroll and applyScroll default implementations so
  // that if JS overrides one of these methods, but not the
  // other, this bookkeeping remains accurate.
  scroll_state.SetCurrentNativeScrollingElement(this);
};

void Element::CallApplyScroll(ScrollState& scroll_state) {
  TRACE_EVENT0("input", "Element::CallApplyScroll");
  // Hits ASSERTs when trying to determine whether we need to scroll on main
  // or CC. http://crbug.com/625676.
  DisableCompositingQueryAsserts disabler;

  if (!GetDocument().GetPage()) {
    // We should always have a Page if we're scrolling. See
    // crbug.com/689074 for details.
    return;
  }

  ScrollStateCallback* callback =
      GetScrollCustomizationCallbacks().GetApplyScroll(this);

  // TODO(bokan): Need to add tests before we allow calling custom callbacks
  // for non-touch modalities. For now, just call into the native callback but
  // allow the viewport scroll callback so we don't disable overscroll.
  // crbug.com/623079.
  bool disable_custom_callbacks = !scroll_state.isDirectManipulation() &&
                                  !GetDocument()
                                       .GetPage()
                                       ->GlobalRootScrollerController()
                                       .IsViewportScrollCallback(callback);
  disable_custom_callbacks |=
      !RootScrollerUtil::IsGlobal(this) &&
      RuntimeEnabledFeatures::ScrollCustomizationEnabled() &&
      !GetScrollCustomizationCallbacks().InScrollPhase(this);

  if (!callback || disable_custom_callbacks) {
    NativeApplyScroll(scroll_state);
    return;
  }
  if (callback->NativeScrollBehavior() !=
      WebNativeScrollBehavior::kPerformAfterNativeScroll)
    callback->Invoke(&scroll_state);
  if (callback->NativeScrollBehavior() !=
      WebNativeScrollBehavior::kDisableNativeScroll)
    NativeApplyScroll(scroll_state);
  if (callback->NativeScrollBehavior() ==
      WebNativeScrollBehavior::kPerformAfterNativeScroll)
    callback->Invoke(&scroll_state);
}

int Element::OffsetLeft() {
  GetDocument().EnsurePaintLocationDataValidForNode(this);
  if (LayoutBoxModelObject* layout_object = GetLayoutBoxModelObject())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(
               LayoutUnit(
                   layout_object->PixelSnappedOffsetLeft(OffsetParent())),
               layout_object->StyleRef())
        .Round();
  return 0;
}

int Element::OffsetTop() {
  GetDocument().EnsurePaintLocationDataValidForNode(this);
  if (LayoutBoxModelObject* layout_object = GetLayoutBoxModelObject())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(
               LayoutUnit(layout_object->PixelSnappedOffsetTop(OffsetParent())),
               layout_object->StyleRef())
        .Round();
  return 0;
}

int Element::OffsetWidth() {
  GetDocument().EnsurePaintLocationDataValidForNode(this);
  if (LayoutBoxModelObject* layout_object = GetLayoutBoxModelObject())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(
               LayoutUnit(
                   layout_object->PixelSnappedOffsetWidth(OffsetParent())),
               layout_object->StyleRef())
        .Round();
  return 0;
}

int Element::OffsetHeight() {
  GetDocument().EnsurePaintLocationDataValidForNode(this);
  if (LayoutBoxModelObject* layout_object = GetLayoutBoxModelObject())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(
               LayoutUnit(
                   layout_object->PixelSnappedOffsetHeight(OffsetParent())),
               layout_object->StyleRef())
        .Round();
  return 0;
}

Element* Element::OffsetParent() {
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  LayoutObject* layout_object = GetLayoutObject();
  return layout_object ? layout_object->OffsetParent() : nullptr;
}

int Element::clientLeft() {
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (LayoutBox* layout_object = GetLayoutBox())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(layout_object->ClientLeft(),
                                                   layout_object->StyleRef())
        .Round();
  return 0;
}

int Element::clientTop() {
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (LayoutBox* layout_object = GetLayoutBox())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(layout_object->ClientTop(),
                                                   layout_object->StyleRef())
        .Round();
  return 0;
}

int Element::clientWidth() {
  // When in strict mode, clientWidth for the document element should return the
  // width of the containing frame.
  // When in quirks mode, clientWidth for the body element should return the
  // width of the containing frame.
  bool in_quirks_mode = GetDocument().InQuirksMode();
  if ((!in_quirks_mode && GetDocument().documentElement() == this) ||
      (in_quirks_mode && IsHTMLElement() && GetDocument().body() == this)) {
    auto* layout_view = GetDocument().GetLayoutView();
    if (layout_view) {
      if (!RuntimeEnabledFeatures::OverlayScrollbarsEnabled() ||
          !GetDocument().GetFrame()->IsLocalRoot())
        GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);
      if (GetDocument().GetPage()->GetSettings().GetForceZeroLayoutHeight())
        return AdjustForAbsoluteZoom::AdjustLayoutUnit(
                   layout_view->OverflowClipRect(LayoutPoint()).Width(),
                   layout_view->StyleRef())
            .Round();
      return AdjustForAbsoluteZoom::AdjustLayoutUnit(
                 LayoutUnit(layout_view->GetLayoutSize().Width()),
                 layout_view->StyleRef())
          .Round();
    }
  }

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (LayoutBox* layout_object = GetLayoutBox())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(
               LayoutUnit(layout_object->PixelSnappedClientWidth()),
               layout_object->StyleRef())
        .Round();
  return 0;
}

int Element::clientHeight() {
  // When in strict mode, clientHeight for the document element should return
  // the height of the containing frame.
  // When in quirks mode, clientHeight for the body element should return the
  // height of the containing frame.
  bool in_quirks_mode = GetDocument().InQuirksMode();

  if ((!in_quirks_mode && GetDocument().documentElement() == this) ||
      (in_quirks_mode && IsHTMLElement() && GetDocument().body() == this)) {
    auto* layout_view = GetDocument().GetLayoutView();
    if (layout_view) {
      if (!RuntimeEnabledFeatures::OverlayScrollbarsEnabled() ||
          !GetDocument().GetFrame()->IsLocalRoot())
        GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);
      if (GetDocument().GetPage()->GetSettings().GetForceZeroLayoutHeight())
        return AdjustForAbsoluteZoom::AdjustLayoutUnit(
                   layout_view->OverflowClipRect(LayoutPoint()).Height(),
                   layout_view->StyleRef())
            .Round();
      return AdjustForAbsoluteZoom::AdjustLayoutUnit(
                 LayoutUnit(layout_view->GetLayoutSize().Height()),
                 layout_view->StyleRef())
          .Round();
    }
  }

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (LayoutBox* layout_object = GetLayoutBox())
    return AdjustForAbsoluteZoom::AdjustLayoutUnit(
               LayoutUnit(layout_object->PixelSnappedClientHeight()),
               layout_object->StyleRef())
        .Round();
  return 0;
}

double Element::scrollLeft() {
  if (!InActiveDocument())
    return 0;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    if (GetDocument().domWindow())
      return GetDocument().domWindow()->scrollX();
    return 0;
  }

  if (LayoutBox* box = GetLayoutBox()) {
    return AdjustForAbsoluteZoom::AdjustScroll(box->ScrollLeft(), *box);
  }

  return 0;
}

double Element::scrollTop() {
  if (!InActiveDocument())
    return 0;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    if (GetDocument().domWindow())
      return GetDocument().domWindow()->scrollY();
    return 0;
  }

  if (LayoutBox* box = GetLayoutBox()) {
    return AdjustForAbsoluteZoom::AdjustScroll(box->ScrollTop(), *box);
  }

  return 0;
}

void Element::setScrollLeft(double new_left) {
  if (!InActiveDocument())
    return;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  new_left = ScrollableArea::NormalizeNonFiniteScroll(new_left);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    if (LocalDOMWindow* window = GetDocument().domWindow()) {
      ScrollToOptions options;
      options.setLeft(new_left);
      window->scrollTo(options);
    }
  } else {
    LayoutBox* box = GetLayoutBox();
    if (!box)
      return;

    FloatPoint end_point(new_left * box->Style()->EffectiveZoom(),
                         box->ScrollTop().ToFloat());
    if (SnapCoordinator* coordinator = GetDocument().GetSnapCoordinator()) {
      end_point =
          coordinator->GetSnapPositionForPoint(*box, end_point, true, false);
    }
    box->SetScrollLeft(LayoutUnit::FromFloatRound(end_point.X()));
  }
}

void Element::setScrollTop(double new_top) {
  if (!InActiveDocument())
    return;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  new_top = ScrollableArea::NormalizeNonFiniteScroll(new_top);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    if (LocalDOMWindow* window = GetDocument().domWindow()) {
      ScrollToOptions options;
      options.setTop(new_top);
      window->scrollTo(options);
    }
  } else {
    LayoutBox* box = GetLayoutBox();
    if (!box)
      return;

    FloatPoint end_point(box->ScrollLeft().ToFloat(),
                         new_top * box->Style()->EffectiveZoom());
    if (SnapCoordinator* coordinator = GetDocument().GetSnapCoordinator()) {
      end_point =
          coordinator->GetSnapPositionForPoint(*box, end_point, false, true);
    }
    box->SetScrollTop(LayoutUnit::FromFloatRound(end_point.Y()));
  }
}

int Element::scrollWidth() {
  if (!InActiveDocument())
    return 0;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    if (GetDocument().View()) {
      return AdjustForAbsoluteZoom::AdjustInt(
          GetDocument()
              .View()
              ->LayoutViewportScrollableArea()
              ->ContentsSize()
              .Width(),
          GetDocument().GetFrame()->PageZoomFactor());
    }
    return 0;
  }

  if (LayoutBox* box = GetLayoutBox()) {
    return AdjustForAbsoluteZoom::AdjustInt(box->PixelSnappedScrollWidth(),
                                            box);
  }
  return 0;
}

int Element::scrollHeight() {
  if (!InActiveDocument())
    return 0;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    if (GetDocument().View()) {
      return AdjustForAbsoluteZoom::AdjustInt(
          GetDocument()
              .View()
              ->LayoutViewportScrollableArea()
              ->ContentsSize()
              .Height(),
          GetDocument().GetFrame()->PageZoomFactor());
    }
    return 0;
  }

  if (LayoutBox* box = GetLayoutBox()) {
    return AdjustForAbsoluteZoom::AdjustInt(box->PixelSnappedScrollHeight(),
                                            box);
  }
  return 0;
}

void Element::scrollBy(double x, double y) {
  ScrollToOptions scroll_to_options;
  scroll_to_options.setLeft(x);
  scroll_to_options.setTop(y);
  scrollBy(scroll_to_options);
}

void Element::scrollBy(const ScrollToOptions& scroll_to_options) {
  if (!InActiveDocument())
    return;

  // FIXME: This should be removed once scroll updates are processed only after
  // the compositing update. See http://crbug.com/420741.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    ScrollFrameBy(scroll_to_options);
  } else {
    ScrollLayoutBoxBy(scroll_to_options);
  }
}

void Element::scrollTo(double x, double y) {
  ScrollToOptions scroll_to_options;
  scroll_to_options.setLeft(x);
  scroll_to_options.setTop(y);
  scrollTo(scroll_to_options);
}

void Element::scrollTo(const ScrollToOptions& scroll_to_options) {
  if (!InActiveDocument())
    return;

  // FIXME: This should be removed once scroll updates are processed only after
  // the compositing update. See http://crbug.com/420741.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (GetDocument().ScrollingElementNoLayout() == this) {
    ScrollFrameTo(scroll_to_options);
  } else {
    ScrollLayoutBoxTo(scroll_to_options);
  }
}

void Element::ScrollLayoutBoxBy(const ScrollToOptions& scroll_to_options) {
  double left =
      scroll_to_options.hasLeft()
          ? ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.left())
          : 0.0;
  double top =
      scroll_to_options.hasTop()
          ? ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.top())
          : 0.0;

  ScrollBehavior scroll_behavior = kScrollBehaviorAuto;
  ScrollableArea::ScrollBehaviorFromString(scroll_to_options.behavior(),
                                           scroll_behavior);
  LayoutBox* box = GetLayoutBox();
  if (box) {
    float current_scaled_left = box->ScrollLeft().ToFloat();
    float current_scaled_top = box->ScrollTop().ToFloat();
    float new_scaled_left =
        left * box->Style()->EffectiveZoom() + current_scaled_left;
    float new_scaled_top =
        top * box->Style()->EffectiveZoom() + current_scaled_top;

    FloatPoint new_scaled_position(new_scaled_left, new_scaled_top);
    if (SnapCoordinator* coordinator = GetDocument().GetSnapCoordinator()) {
      new_scaled_position = coordinator->GetSnapPositionForPoint(
          *box, new_scaled_position, scroll_to_options.hasLeft(),
          scroll_to_options.hasTop());
    }
    box->ScrollToPosition(new_scaled_position, scroll_behavior);
  }
}

void Element::ScrollLayoutBoxTo(const ScrollToOptions& scroll_to_options) {
  ScrollBehavior scroll_behavior = kScrollBehaviorAuto;
  ScrollableArea::ScrollBehaviorFromString(scroll_to_options.behavior(),
                                           scroll_behavior);

  LayoutBox* box = GetLayoutBox();
  if (box) {
    float scaled_left = box->ScrollLeft().ToFloat();
    float scaled_top = box->ScrollTop().ToFloat();
    if (scroll_to_options.hasLeft())
      scaled_left =
          ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.left()) *
          box->Style()->EffectiveZoom();
    if (scroll_to_options.hasTop())
      scaled_top =
          ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.top()) *
          box->Style()->EffectiveZoom();

    FloatPoint new_scaled_position(scaled_left, scaled_top);
    if (SnapCoordinator* coordinator = GetDocument().GetSnapCoordinator()) {
      new_scaled_position = coordinator->GetSnapPositionForPoint(
          *box, new_scaled_position, scroll_to_options.hasLeft(),
          scroll_to_options.hasTop());
    }
    box->ScrollToPosition(new_scaled_position, scroll_behavior);
  }
}

void Element::ScrollFrameBy(const ScrollToOptions& scroll_to_options) {
  double left =
      scroll_to_options.hasLeft()
          ? ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.left())
          : 0.0;
  double top =
      scroll_to_options.hasTop()
          ? ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.top())
          : 0.0;

  ScrollBehavior scroll_behavior = kScrollBehaviorAuto;
  ScrollableArea::ScrollBehaviorFromString(scroll_to_options.behavior(),
                                           scroll_behavior);
  LocalFrame* frame = GetDocument().GetFrame();
  if (!frame || !frame->View() || !GetDocument().GetPage())
    return;

  ScrollableArea* viewport = frame->View()->LayoutViewportScrollableArea();
  if (!viewport)
    return;

  // TODO(810510): Move this logic inside "ScrollableArea::SetScrollOffset" and
  // rely on ScrollType to detect js scrolls and set the flag. This requires
  // adding new scroll type to enable this.  if (GetDocument().Loader())
  GetDocument().Loader()->GetInitialScrollState().was_scrolled_by_js = true;

  float new_scaled_left =
      left * frame->PageZoomFactor() + viewport->GetScrollOffset().Width();
  float new_scaled_top =
      top * frame->PageZoomFactor() + viewport->GetScrollOffset().Height();

  FloatPoint new_scaled_position = ScrollOffsetToPosition(
      ScrollOffset(new_scaled_left, new_scaled_top), viewport->ScrollOrigin());
  if (SnapCoordinator* coordinator = GetDocument().GetSnapCoordinator()) {
    new_scaled_position = coordinator->GetSnapPositionForPoint(
        *GetDocument().GetLayoutView(), new_scaled_position,
        scroll_to_options.hasLeft(), scroll_to_options.hasTop());
  }
  viewport->SetScrollOffset(
      ScrollPositionToOffset(new_scaled_position, viewport->ScrollOrigin()),
      kProgrammaticScroll, scroll_behavior);
}

void Element::ScrollFrameTo(const ScrollToOptions& scroll_to_options) {
  ScrollBehavior scroll_behavior = kScrollBehaviorAuto;
  ScrollableArea::ScrollBehaviorFromString(scroll_to_options.behavior(),
                                           scroll_behavior);
  LocalFrame* frame = GetDocument().GetFrame();
  if (!frame || !frame->View() || !GetDocument().GetPage())
    return;

  ScrollableArea* viewport = frame->View()->LayoutViewportScrollableArea();
  if (!viewport)
    return;

  // TODO(810510): Move this logic inside "ScrollableArea::SetScrollOffset" and
  // rely on ScrollType to detect js scrolls and set the flag. This requires
  // adding new scroll type to enable this.
  if (GetDocument().Loader())
    GetDocument().Loader()->GetInitialScrollState().was_scrolled_by_js = true;

  float scaled_left = viewport->GetScrollOffset().Width();
  float scaled_top = viewport->GetScrollOffset().Height();
  if (scroll_to_options.hasLeft())
    scaled_left =
        ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.left()) *
        frame->PageZoomFactor();
  if (scroll_to_options.hasTop())
    scaled_top =
        ScrollableArea::NormalizeNonFiniteScroll(scroll_to_options.top()) *
        frame->PageZoomFactor();

  FloatPoint new_scaled_position = ScrollOffsetToPosition(
      ScrollOffset(scaled_left, scaled_top), viewport->ScrollOrigin());
  if (SnapCoordinator* coordinator = GetDocument().GetSnapCoordinator()) {
    new_scaled_position = coordinator->GetSnapPositionForPoint(
        *GetDocument().GetLayoutView(), new_scaled_position,
        scroll_to_options.hasLeft(), scroll_to_options.hasTop());
  }
  viewport->SetScrollOffset(
      ScrollPositionToOffset(new_scaled_position, viewport->ScrollOrigin()),
      kProgrammaticScroll, scroll_behavior);
}

bool Element::HasNonEmptyLayoutSize() const {
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  if (LayoutBoxModelObject* box = GetLayoutBoxModelObject())
    return box->HasNonEmptyLayoutSize();
  return false;
}

IntRect Element::BoundsInViewport() const {
  GetDocument().EnsurePaintLocationDataValidForNode(this);

  LocalFrameView* view = GetDocument().View();
  if (!view)
    return IntRect();

  Vector<FloatQuad> quads;

  // TODO(pdr): Unify the quad/bounds code with Element::ClientQuads.

  // Foreign objects need to convert between SVG and HTML coordinate spaces and
  // cannot use LocalToAbsoluteQuad directly with ObjectBoundingBox which is
  // SVG coordinates and not HTML coordinates. Instead, use the AbsoluteQuads
  // codepath below.
  if (IsSVGElement() && GetLayoutObject() &&
      !GetLayoutObject()->IsSVGForeignObject()) {
    // Get the bounding rectangle from the SVG model.
    // TODO(pdr): This should include stroke.
    if (ToSVGElement(this)->IsSVGGraphicsElement())
      quads.push_back(GetLayoutObject()->LocalToAbsoluteQuad(
          GetLayoutObject()->ObjectBoundingBox()));
  } else {
    // Get the bounding rectangle from the box model.
    if (GetLayoutBoxModelObject())
      GetLayoutBoxModelObject()->AbsoluteQuads(quads);
  }

  if (quads.IsEmpty())
    return IntRect();

  IntRect result = quads[0].EnclosingBoundingBox();
  for (size_t i = 1; i < quads.size(); ++i)
    result.Unite(quads[i].EnclosingBoundingBox());

  return view->ContentsToViewport(result);
}

IntRect Element::VisibleBoundsInVisualViewport() const {
  if (!GetLayoutObject() || !GetDocument().GetPage() ||
      !GetDocument().GetFrame())
    return IntRect();

  // We don't use absoluteBoundingBoxRect() because it can return an IntRect
  // larger the actual size by 1px. crbug.com/470503
  LayoutRect rect(
      RoundedIntRect(GetLayoutObject()->AbsoluteBoundingBoxFloatRect()));
  LayoutRect frame_clip_rect =
      GetDocument().View()->GetLayoutBox()->ClippingRect(LayoutPoint());
  rect.Intersect(frame_clip_rect);

  // MapToVisualRectInAncestorSpace, called with a null ancestor argument,
  // returns the viewport-visible rect in the local frame root's coordinates,
  // accounting for clips and transformed in embedding containers. This
  // includes clips that might be applied by out-of-process frame ancestors.
  GetDocument().View()->GetLayoutView()->MapToVisualRectInAncestorSpace(
      nullptr, rect, kUseTransforms | kTraverseDocumentBoundaries,
      kDefaultVisualRectFlags);

  IntRect visible_rect = PixelSnappedIntRect(rect);
  // If the rect is in the coordinates of the main frame, then it should
  // also be clipped to the viewport to account for page scale. For OOPIFs,
  // local frame root -> viewport coordinate conversion is done in the
  // browser process.
  if (GetDocument().GetFrame()->LocalFrameRoot().IsMainFrame()) {
    IntSize viewport_size = GetDocument().GetPage()->GetVisualViewport().Size();
    visible_rect =
        GetDocument().GetPage()->GetVisualViewport().RootFrameToViewport(
            visible_rect);
    visible_rect.Intersect(IntRect(IntPoint(), viewport_size));
  }
  return visible_rect;
}

void Element::ClientQuads(Vector<FloatQuad>& quads) {
  GetDocument().EnsurePaintLocationDataValidForNode(this);

  LayoutObject* element_layout_object = GetLayoutObject();
  if (!element_layout_object)
    return;

  // Foreign objects need to convert between SVG and HTML coordinate spaces and
  // cannot use LocalToAbsoluteQuad directly with ObjectBoundingBox which is
  // SVG coordinates and not HTML coordinates. Instead, use the AbsoluteQuads
  // codepath below.
  if (IsSVGElement() && !element_layout_object->IsSVGRoot() &&
      !element_layout_object->IsSVGForeignObject()) {
    // Get the bounding rectangle from the SVG model.
    // TODO(pdr): ObjectBoundingBox does not include stroke and the spec is not
    // clear (see: https://github.com/w3c/svgwg/issues/339, crbug.com/529734).
    // If stroke is desired, we can update this to use AbsoluteQuads, below.
    if (ToSVGElement(this)->IsSVGGraphicsElement())
      quads.push_back(element_layout_object->LocalToAbsoluteQuad(
          element_layout_object->ObjectBoundingBox()));
    return;
  }

  // FIXME: Handle table/inline-table with a caption.
  if (element_layout_object->IsBoxModelObject() ||
      element_layout_object->IsBR())
    element_layout_object->AbsoluteQuads(quads, kUseTransforms);
}

DOMRectList* Element::getClientRects() {
  Vector<FloatQuad> quads;
  ClientQuads(quads);
  if (quads.IsEmpty())
    return DOMRectList::Create();

  LayoutObject* element_layout_object = GetLayoutObject();
  DCHECK(element_layout_object);
  GetDocument().AdjustFloatQuadsForScrollAndAbsoluteZoom(
      quads, *element_layout_object);
  return DOMRectList::Create(quads);
}

DOMRect* Element::getBoundingClientRect() {
  Vector<FloatQuad> quads;
  ClientQuads(quads);
  if (quads.IsEmpty())
    return DOMRect::Create();

  FloatRect result = quads[0].BoundingBox();
  for (size_t i = 1; i < quads.size(); ++i)
    result.Unite(quads[i].BoundingBox());

  LayoutObject* element_layout_object = GetLayoutObject();
  DCHECK(element_layout_object);
  GetDocument().AdjustFloatRectForScrollAndAbsoluteZoom(result,
                                                        *element_layout_object);
  return DOMRect::FromFloatRect(result);
}

const AtomicString& Element::computedRole() {
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);
  std::unique_ptr<ScopedAXObjectCache> cache =
      ScopedAXObjectCache::Create(GetDocument());
  return cache->Get()->ComputedRoleForNode(this);
}

String Element::computedName() {
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);
  std::unique_ptr<ScopedAXObjectCache> cache =
      ScopedAXObjectCache::Create(GetDocument());
  return cache->Get()->ComputedNameForNode(this);
}

AccessibleNode* Element::ExistingAccessibleNode() const {
  if (!RuntimeEnabledFeatures::AccessibilityObjectModelEnabled())
    return nullptr;

  if (!HasRareData())
    return nullptr;

  return GetElementRareData()->GetAccessibleNode();
}

AccessibleNode* Element::accessibleNode() {
  if (!RuntimeEnabledFeatures::AccessibilityObjectModelEnabled())
    return nullptr;

  ElementRareData& rare_data = EnsureElementRareData();
  return rare_data.EnsureAccessibleNode(this);
}

const AtomicString& Element::getAttribute(
    const AtomicString& local_name) const {
  if (!GetElementData())
    return g_null_atom;
  SynchronizeAttribute(local_name);
  if (const Attribute* attribute =
          GetElementData()->Attributes().Find(LowercaseIfNecessary(local_name)))
    return attribute->Value();
  return g_null_atom;
}

const AtomicString& Element::getAttributeNS(
    const AtomicString& namespace_uri,
    const AtomicString& local_name) const {
  return getAttribute(QualifiedName(g_null_atom, local_name, namespace_uri));
}

void Element::setAttribute(const AtomicString& local_name,
                           const AtomicString& value,
                           ExceptionState& exception_state) {
  if (!Document::IsValidName(local_name)) {
    exception_state.ThrowDOMException(
        kInvalidCharacterError,
        "'" + local_name + "' is not a valid attribute name.");
    return;
  }

  SynchronizeAttribute(local_name);
  AtomicString case_adjusted_local_name = LowercaseIfNecessary(local_name);

  if (!GetElementData()) {
    SetAttributeInternal(
        kNotFound,
        QualifiedName(g_null_atom, case_adjusted_local_name, g_null_atom),
        value, kNotInSynchronizationOfLazyAttribute);
    return;
  }

  AttributeCollection attributes = GetElementData()->Attributes();
  size_t index = attributes.FindIndex(case_adjusted_local_name);
  const QualifiedName& q_name =
      index != kNotFound
          ? attributes[index].GetName()
          : QualifiedName(g_null_atom, case_adjusted_local_name, g_null_atom);
  SetAttributeInternal(index, q_name, value,
                       kNotInSynchronizationOfLazyAttribute);
}

void Element::setAttribute(const AtomicString& name,
                           const AtomicString& value) {
  setAttribute(name, value, ASSERT_NO_EXCEPTION);
}

void Element::setAttribute(const QualifiedName& name,
                           const AtomicString& value) {
  SynchronizeAttribute(name);
  size_t index = GetElementData()
                     ? GetElementData()->Attributes().FindIndex(name)
                     : kNotFound;
  SetAttributeInternal(index, name, value,
                       kNotInSynchronizationOfLazyAttribute);
}

void Element::SetSynchronizedLazyAttribute(const QualifiedName& name,
                                           const AtomicString& value) {
  size_t index = GetElementData()
                     ? GetElementData()->Attributes().FindIndex(name)
                     : kNotFound;
  SetAttributeInternal(index, name, value, kInSynchronizationOfLazyAttribute);
}

void Element::setAttribute(const QualifiedName& name,
                           const StringOrTrustedScriptURL& stringOrURL,
                           ExceptionState& exception_state) {
  DCHECK(stringOrURL.IsString() ||
         RuntimeEnabledFeatures::TrustedDOMTypesEnabled());
  if (stringOrURL.IsString() && GetDocument().RequireTrustedTypes()) {
    exception_state.ThrowTypeError(
        "This document requires `TrustedScriptURL` assignment.");
    return;
  }

  String valueString = stringOrURL.IsString()
                           ? stringOrURL.GetAsString()
                           : stringOrURL.GetAsTrustedScriptURL()->toString();

  setAttribute(name, AtomicString(valueString));
}

ALWAYS_INLINE void Element::SetAttributeInternal(
    size_t index,
    const QualifiedName& name,
    const AtomicString& new_value,
    SynchronizationOfLazyAttribute in_synchronization_of_lazy_attribute) {
  if (new_value.IsNull()) {
    if (index != kNotFound)
      RemoveAttributeInternal(index, in_synchronization_of_lazy_attribute);
    return;
  }

  if (index == kNotFound) {
    AppendAttributeInternal(name, new_value,
                            in_synchronization_of_lazy_attribute);
    return;
  }

  const Attribute& existing_attribute =
      GetElementData()->Attributes().at(index);
  AtomicString existing_attribute_value = existing_attribute.Value();
  QualifiedName existing_attribute_name = existing_attribute.GetName();

  if (!in_synchronization_of_lazy_attribute)
    WillModifyAttribute(existing_attribute_name, existing_attribute_value,
                        new_value);
  if (new_value != existing_attribute_value)
    EnsureUniqueElementData().Attributes().at(index).SetValue(new_value);
  if (!in_synchronization_of_lazy_attribute)
    DidModifyAttribute(existing_attribute_name, existing_attribute_value,
                       new_value);
}

static inline AtomicString MakeIdForStyleResolution(const AtomicString& value,
                                                    bool in_quirks_mode) {
  if (in_quirks_mode)
    return value.LowerASCII();
  return value;
}

DISABLE_CFI_PERF
void Element::AttributeChanged(const AttributeModificationParams& params) {
  const QualifiedName& name = params.name;
  if (ShadowRoot* parent_shadow_root =
          ShadowRootWhereNodeCanBeDistributedForV0(*this)) {
    if (ShouldInvalidateDistributionWhenAttributeChanged(
            *parent_shadow_root, name, params.new_value))
      parent_shadow_root->SetNeedsDistributionRecalc();
  }
  if (name == HTMLNames::slotAttr && params.old_value != params.new_value) {
    if (ShadowRoot* root = V1ShadowRootOfParent())
      root->DidChangeHostChildSlotName(params.old_value, params.new_value);
  }

  ParseAttribute(params);

  GetDocument().IncDOMTreeVersion();

  if (name == HTMLNames::idAttr) {
    AtomicString old_id = GetElementData()->IdForStyleResolution();
    AtomicString new_id = MakeIdForStyleResolution(
        params.new_value, GetDocument().InQuirksMode());
    if (new_id != old_id) {
      GetElementData()->SetIdForStyleResolution(new_id);
      GetDocument().GetStyleEngine().IdChangedForElement(old_id, new_id, *this);
    }
  } else if (name == classAttr) {
    ClassAttributeChanged(params.new_value);
    if (HasRareData() && GetElementRareData()->GetClassList()) {
      GetElementRareData()->GetClassList()->DidUpdateAttributeValue(
          params.old_value, params.new_value);
    }
  } else if (name == HTMLNames::nameAttr) {
    SetHasName(!params.new_value.IsNull());
  } else if (name == HTMLNames::partAttr) {
    if (RuntimeEnabledFeatures::CSSPartPseudoElementEnabled())
      EnsureElementRareData().SetPart(params.new_value);
  } else if (name == HTMLNames::partmapAttr) {
    if (RuntimeEnabledFeatures::CSSPartPseudoElementEnabled())
      EnsureElementRareData().SetPartNamesMap(params.new_value);
  } else if (IsStyledElement()) {
    if (name == styleAttr) {
      StyleAttributeChanged(params.new_value, params.reason);
    } else if (IsPresentationAttribute(name)) {
      GetElementData()->presentation_attribute_style_is_dirty_ = true;
      SetNeedsStyleRecalc(kLocalStyleChange,
                          StyleChangeReasonForTracing::FromAttribute(name));
    }
  }

  InvalidateNodeListCachesInAncestors(&name, this, nullptr);

  if (isConnected()) {
    if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache()) {
      if (params.old_value != params.new_value)
        cache->HandleAttributeChanged(name, this);
    }
  }

  if (params.reason == AttributeModificationReason::kDirectly &&
      name == tabindexAttr && AdjustedFocusedElementInTreeScope() == this) {
    // The attribute change may cause supportsFocus() to return false
    // for the element which had focus.
    //
    // TODO(tkent): We should avoid updating style.  We'd like to check only
    // DOM-level focusability here.
    GetDocument().UpdateStyleAndLayoutTreeForNode(this);
    if (!SupportsFocus())
      blur();
  }
}

bool Element::HasLegalLinkAttribute(const QualifiedName&) const {
  return false;
}

const QualifiedName& Element::SubResourceAttributeName() const {
  return QualifiedName::Null();
}

template <typename CharacterType>
static inline ClassStringContent ClassStringHasClassName(
    const CharacterType* characters,
    unsigned length) {
  DCHECK_GT(length, 0u);

  unsigned i = 0;
  do {
    if (IsNotHTMLSpace<CharacterType>(characters[i]))
      break;
    ++i;
  } while (i < length);

  if (i == length && length >= 1)
    return ClassStringContent::kWhiteSpaceOnly;

  return ClassStringContent::kHasClasses;
}

static inline ClassStringContent ClassStringHasClassName(
    const AtomicString& new_class_string) {
  unsigned length = new_class_string.length();

  if (!length)
    return ClassStringContent::kEmpty;

  if (new_class_string.Is8Bit())
    return ClassStringHasClassName(new_class_string.Characters8(), length);
  return ClassStringHasClassName(new_class_string.Characters16(), length);
}

void Element::ClassAttributeChanged(const AtomicString& new_class_string) {
  DCHECK(GetElementData());
  ClassStringContent class_string_content_type =
      ClassStringHasClassName(new_class_string);
  const bool should_fold_case = GetDocument().InQuirksMode();
  if (class_string_content_type == ClassStringContent::kHasClasses) {
    const SpaceSplitString old_classes = GetElementData()->ClassNames();
    GetElementData()->SetClass(new_class_string, should_fold_case);
    const SpaceSplitString& new_classes = GetElementData()->ClassNames();
    GetDocument().GetStyleEngine().ClassChangedForElement(old_classes,
                                                          new_classes, *this);
  } else {
    const SpaceSplitString& old_classes = GetElementData()->ClassNames();
    GetDocument().GetStyleEngine().ClassChangedForElement(old_classes, *this);
    if (class_string_content_type == ClassStringContent::kWhiteSpaceOnly)
      GetElementData()->SetClass(new_class_string, should_fold_case);
    else
      GetElementData()->ClearClass();
  }
}

bool Element::ShouldInvalidateDistributionWhenAttributeChanged(
    ShadowRoot& shadow_root,
    const QualifiedName& name,
    const AtomicString& new_value) {
  if (shadow_root.IsV1())
    return false;
  const SelectRuleFeatureSet& feature_set =
      shadow_root.V0().EnsureSelectFeatureSet();

  if (name == HTMLNames::idAttr) {
    AtomicString old_id = GetElementData()->IdForStyleResolution();
    AtomicString new_id =
        MakeIdForStyleResolution(new_value, GetDocument().InQuirksMode());
    if (new_id != old_id) {
      if (!old_id.IsEmpty() && feature_set.HasSelectorForId(old_id))
        return true;
      if (!new_id.IsEmpty() && feature_set.HasSelectorForId(new_id))
        return true;
    }
  }

  if (name == HTMLNames::classAttr) {
    const AtomicString& new_class_string = new_value;
    if (ClassStringHasClassName(new_class_string) ==
        ClassStringContent::kHasClasses) {
      const SpaceSplitString& old_classes = GetElementData()->ClassNames();
      const SpaceSplitString new_classes(GetDocument().InQuirksMode()
                                             ? new_class_string.LowerASCII()
                                             : new_class_string);
      if (feature_set.CheckSelectorsForClassChange(old_classes, new_classes))
        return true;
    } else {
      const SpaceSplitString& old_classes = GetElementData()->ClassNames();
      if (feature_set.CheckSelectorsForClassChange(old_classes))
        return true;
    }
  }

  return feature_set.HasSelectorForAttribute(name.LocalName());
}

// Returns true if the given attribute is an event handler.
// We consider an event handler any attribute that begins with "on".
// It is a simple solution that has the advantage of not requiring any
// code or configuration change if a new event handler is defined.

static inline bool IsEventHandlerAttribute(const Attribute& attribute) {
  return attribute.GetName().NamespaceURI().IsNull() &&
         attribute.GetName().LocalName().StartsWith("on");
}

bool Element::AttributeValueIsJavaScriptURL(const Attribute& attribute) {
  return ProtocolIsJavaScript(
      StripLeadingAndTrailingHTMLSpaces(attribute.Value()));
}

bool Element::IsJavaScriptURLAttribute(const Attribute& attribute) const {
  return IsURLAttribute(attribute) && AttributeValueIsJavaScriptURL(attribute);
}

bool Element::IsScriptingAttribute(const Attribute& attribute) const {
  return IsEventHandlerAttribute(attribute) ||
         IsJavaScriptURLAttribute(attribute) ||
         IsHTMLContentAttribute(attribute) ||
         IsSVGAnimationAttributeSettingJavaScriptURL(attribute);
}

void Element::StripScriptingAttributes(
    Vector<Attribute>& attribute_vector) const {
  size_t destination = 0;
  for (size_t source = 0; source < attribute_vector.size(); ++source) {
    if (IsScriptingAttribute(attribute_vector[source]))
      continue;

    if (source != destination)
      attribute_vector[destination] = attribute_vector[source];

    ++destination;
  }
  attribute_vector.Shrink(destination);
}

void Element::ParserSetAttributes(const Vector<Attribute>& attribute_vector) {
  DCHECK(!isConnected());
  DCHECK(!parentNode());
  DCHECK(!element_data_);

  if (!attribute_vector.IsEmpty()) {
    if (GetDocument().GetElementDataCache())
      element_data_ =
          GetDocument()
              .GetElementDataCache()
              ->CachedShareableElementDataWithAttributes(attribute_vector);
    else
      element_data_ =
          ShareableElementData::CreateWithAttributes(attribute_vector);
  }

  ParserDidSetAttributes();

  // Use attributeVector instead of m_elementData because attributeChanged might
  // modify m_elementData.
  for (const auto& attribute : attribute_vector) {
    AttributeChanged(AttributeModificationParams(
        attribute.GetName(), g_null_atom, attribute.Value(),
        AttributeModificationReason::kByParser));
  }
}

bool Element::HasEquivalentAttributes(const Element* other) const {
  SynchronizeAllAttributes();
  other->SynchronizeAllAttributes();
  if (GetElementData() == other->GetElementData())
    return true;
  if (GetElementData())
    return GetElementData()->IsEquivalent(other->GetElementData());
  if (other->GetElementData())
    return other->GetElementData()->IsEquivalent(GetElementData());
  return true;
}

String Element::nodeName() const {
  return tag_name_.ToString();
}

AtomicString Element::LocalNameForSelectorMatching() const {
  if (IsHTMLElement() || !GetDocument().IsHTMLDocument())
    return localName();
  return localName().DeprecatedLower();
}

const AtomicString& Element::LocateNamespacePrefix(
    const AtomicString& namespace_to_locate) const {
  if (!prefix().IsNull() && namespaceURI() == namespace_to_locate)
    return prefix();

  AttributeCollection attributes = Attributes();
  for (const Attribute& attr : attributes) {
    if (attr.Prefix() == g_xmlns_atom && attr.Value() == namespace_to_locate)
      return attr.LocalName();
  }

  if (Element* parent = parentElement())
    return parent->LocateNamespacePrefix(namespace_to_locate);

  return g_null_atom;
}

const AtomicString Element::ImageSourceURL() const {
  return getAttribute(srcAttr);
}

bool Element::LayoutObjectIsNeeded(const ComputedStyle& style) const {
  return style.Display() != EDisplay::kNone &&
         style.Display() != EDisplay::kContents;
}

LayoutObject* Element::CreateLayoutObject(const ComputedStyle& style) {
  return LayoutObject::CreateObject(this, style);
}

Node::InsertionNotificationRequest Element::InsertedInto(
    ContainerNode* insertion_point) {
  // need to do superclass processing first so isConnected() is true
  // by the time we reach updateId
  ContainerNode::InsertedInto(insertion_point);

  DCHECK(!HasRareData() || !GetElementRareData()->HasPseudoElements());

  if (!insertion_point->IsInTreeScope())
    return kInsertionDone;

  if (HasRareData()) {
    ElementRareData* rare_data = GetElementRareData();
    if (rare_data->IntersectionObserverData())
      rare_data->IntersectionObserverData()->ActivateValidIntersectionObservers(
          *this);
  }

  if (isConnected()) {
    if (GetCustomElementState() == CustomElementState::kCustom)
      CustomElement::EnqueueConnectedCallback(this);
    else if (IsUpgradedV0CustomElement())
      V0CustomElement::DidAttach(this, GetDocument());
    else if (GetCustomElementState() == CustomElementState::kUndefined)
      CustomElement::TryToUpgrade(this);
  }

  TreeScope& scope = insertion_point->GetTreeScope();
  if (scope != GetTreeScope())
    return kInsertionDone;

  const AtomicString& id_value = GetIdAttribute();
  if (!id_value.IsNull())
    UpdateId(scope, g_null_atom, id_value);

  const AtomicString& name_value = GetNameAttribute();
  if (!name_value.IsNull())
    UpdateName(g_null_atom, name_value);

  if (parentElement() && parentElement()->IsInCanvasSubtree())
    SetIsInCanvasSubtree(true);

  return kInsertionDone;
}

void Element::RemovedFrom(ContainerNode* insertion_point) {
  bool was_in_document = insertion_point->isConnected();

  DCHECK(!HasRareData() || !GetElementRareData()->HasPseudoElements());

  if (Fullscreen::IsFullscreenElement(*this)) {
    SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(false);
    if (insertion_point->IsElementNode()) {
      ToElement(insertion_point)->SetContainsFullScreenElement(false);
      ToElement(insertion_point)
          ->SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(
              false);
    }
  }

  if (Fullscreen* fullscreen = Fullscreen::FromIfExists(GetDocument()))
    fullscreen->ElementRemoved(*this);

  if (GetDocument().GetPage())
    GetDocument().GetPage()->GetPointerLockController().ElementRemoved(this);

  SetSavedLayerScrollOffset(ScrollOffset());

  if (insertion_point->IsInTreeScope() && GetTreeScope() == GetDocument()) {
    const AtomicString& id_value = GetIdAttribute();
    if (!id_value.IsNull())
      UpdateId(insertion_point->GetTreeScope(), id_value, g_null_atom);

    const AtomicString& name_value = GetNameAttribute();
    if (!name_value.IsNull())
      UpdateName(name_value, g_null_atom);
  }

  ContainerNode::RemovedFrom(insertion_point);
  if (was_in_document) {
    if (this == GetDocument().CssTarget())
      GetDocument().SetCSSTarget(nullptr);

    if (GetCustomElementState() == CustomElementState::kCustom)
      CustomElement::EnqueueDisconnectedCallback(this);
    else if (IsUpgradedV0CustomElement())
      V0CustomElement::DidDetach(this, insertion_point->GetDocument());

    if (NeedsStyleInvalidation()) {
      GetDocument()
          .GetStyleEngine()
          .GetPendingNodeInvalidations()
          .ClearInvalidation(*this);
    }
  }

  GetDocument().GetRootScrollerController().ElementRemoved(*this);

  GetDocument().RemoveFromTopLayer(this);

  ClearElementFlag(ElementFlags::kIsInCanvasSubtree);

  if (HasRareData()) {
    ElementRareData* data = GetElementRareData();

    data->ClearRestyleFlags();

    if (ElementAnimations* element_animations = data->GetElementAnimations())
      element_animations->CssAnimations().Cancel();

    if (data->IntersectionObserverData())
      data->IntersectionObserverData()->DeactivateAllIntersectionObservers(
          *this);
  }

  if (GetDocument().GetFrame())
    GetDocument().GetFrame()->GetEventHandler().ElementRemoved(this);
}

void Element::AttachLayoutTree(AttachContext& context) {
  DCHECK(GetDocument().InStyleRecalc());

  // We've already been through detach when doing an attach, but we might
  // need to clear any state that's been added since then.
  if (HasRareData() && NeedsAttach()) {
    ElementRareData* data = GetElementRareData();
    data->ClearComputedStyle();
  }

  if (CanParticipateInFlatTree()) {
    LayoutTreeBuilderForElement builder(*this, GetNonAttachedStyle());
    builder.CreateLayoutObjectIfNeeded();

    if (ComputedStyle* style = builder.ResolvedStyle()) {
      if (!GetLayoutObject() && ShouldStoreNonLayoutObjectComputedStyle(*style))
        StoreNonLayoutObjectComputedStyle(style);
    }
  }

  if (HasRareData() && !GetLayoutObject() &&
      !GetElementRareData()->GetComputedStyle()) {
    if (ElementAnimations* element_animations =
            GetElementRareData()->GetElementAnimations()) {
      element_animations->CssAnimations().Cancel();
      element_animations->SetAnimationStyleChange(false);
    }
  }

  SelectorFilterParentScope filter_scope(*this);

  AttachContext children_context(context);

  LayoutObject* layout_object = GetLayoutObject();
  if (layout_object)
    children_context.previous_in_flow = nullptr;
  children_context.use_previous_in_flow = true;

  ClearNeedsReattachLayoutTree();
  CreateAndAttachPseudoElementIfNeeded(kPseudoIdBefore, children_context);

  // When a shadow root exists, it does the work of attaching the children.
  if (ShadowRoot* shadow_root = GetShadowRoot()) {
    if (shadow_root->NeedsAttach())
      shadow_root->AttachLayoutTree(children_context);
  }

  ContainerNode::AttachLayoutTree(children_context);
  SetNonAttachedStyle(nullptr);
  AddCallbackSelectors();

  CreateAndAttachPseudoElementIfNeeded(kPseudoIdAfter, children_context);
  CreateAndAttachPseudoElementIfNeeded(kPseudoIdBackdrop, children_context);

  // We create the first-letter element after the :before, :after and
  // children are attached because the first letter text could come
  // from any of them.
  CreateAndAttachPseudoElementIfNeeded(kPseudoIdFirstLetter, children_context);

  if (layout_object) {
    if (!layout_object->IsFloatingOrOutOfFlowPositioned())
      context.previous_in_flow = layout_object;
  } else {
    context.previous_in_flow = children_context.previous_in_flow;
  }
}

void Element::DetachLayoutTree(const AttachContext& context) {
  HTMLFrameOwnerElement::PluginDisposeSuspendScope suspend_plugin_dispose;
  CancelFocusAppearanceUpdate();
  RemoveCallbackSelectors();
  if (HasRareData()) {
    ElementRareData* data = GetElementRareData();
    data->ClearPseudoElements();

    // attachLayoutTree() will clear the computed style for us when inside
    // recalcStyle.
    if (!GetDocument().InStyleRecalc())
      data->ClearComputedStyle();

    if (ElementAnimations* element_animations = data->GetElementAnimations()) {
      if (context.performing_reattach) {
        // FIXME: We call detach from within style recalc, so compositingState
        // is not up to date.
        // https://code.google.com/p/chromium/issues/detail?id=339847
        DisableCompositingQueryAsserts disabler;

        // FIXME: restart compositor animations rather than pull back to the
        // main thread
        element_animations->RestartAnimationOnCompositor();
      } else {
        element_animations->CssAnimations().Cancel();
        element_animations->SetAnimationStyleChange(false);
      }
      element_animations->ClearBaseComputedStyle();
    }

    if (ShadowRoot* shadow_root = data->GetShadowRoot())
      shadow_root->DetachLayoutTree(context);
  }

  ContainerNode::DetachLayoutTree(context);

  if (!context.performing_reattach && IsUserActionElement()) {
    if (IsHovered())
      GetDocument().HoveredElementDetached(*this);
    if (InActiveChain())
      GetDocument().ActiveChainNodeDetached(*this);
    GetDocument().UserActionElements().DidDetach(*this);
  }

  if (context.clear_invalidation) {
    GetDocument()
        .GetStyleEngine()
        .GetPendingNodeInvalidations()
        .ClearInvalidation(*this);
  }

  SetNeedsResizeObserverUpdate();

  DCHECK(NeedsAttach());
}

scoped_refptr<ComputedStyle> Element::StyleForLayoutObject() {
  DCHECK(GetDocument().InStyleRecalc());

  // FIXME: Instead of clearing updates that may have been added from calls to
  // StyleForElement outside RecalcStyle, we should just never set them if we're
  // not inside RecalcStyle.
  if (ElementAnimations* element_animations = GetElementAnimations())
    element_animations->CssAnimations().ClearPendingUpdate();

  scoped_refptr<ComputedStyle> style = HasCustomStyleCallbacks()
                                           ? CustomStyleForLayoutObject()
                                           : OriginalStyleForLayoutObject();
  if (!style) {
    DCHECK(IsBeforePseudoElement() || IsAfterPseudoElement());
    return nullptr;
  }

  // StyleForElement() might add active animations so we need to get it again.
  if (ElementAnimations* element_animations = GetElementAnimations()) {
    element_animations->CssAnimations().MaybeApplyPendingUpdate(this);
    element_animations->UpdateAnimationFlags(*style);
  }

  if (style->HasTransform()) {
    if (const CSSPropertyValueSet* inline_style = InlineStyle()) {
      style->SetHasInlineTransform(
          inline_style->HasProperty(CSSPropertyTransform) ||
          inline_style->HasProperty(CSSPropertyTranslate) ||
          inline_style->HasProperty(CSSPropertyRotate) ||
          inline_style->HasProperty(CSSPropertyScale));
    }
  }

  style->UpdateIsStackingContext(this == GetDocument().documentElement(),
                                 IsInTopLayer(),
                                 IsSVGForeignObjectElement(*this));

  return style;
}

scoped_refptr<ComputedStyle> Element::OriginalStyleForLayoutObject() {
  DCHECK(GetDocument().InStyleRecalc());
  return GetDocument().EnsureStyleResolver().StyleForElement(this);
}

void Element::RecalcStyle(StyleRecalcChange change) {
  DCHECK(GetDocument().InStyleRecalc());
  DCHECK(!GetDocument().Lifecycle().InDetach());
  DCHECK(!ParentOrShadowHostNode()->NeedsStyleRecalc());
  DCHECK(InActiveDocument());

  if (HasCustomStyleCallbacks())
    WillRecalcStyle(change);

  if (change >= kIndependentInherit || NeedsStyleRecalc()) {
    if (HasRareData()) {
      ElementRareData* data = GetElementRareData();
      if (change != kIndependentInherit) {
        // We keep the old computed style around for display: contents, option
        // and optgroup. This way we can call stylePropagationDiff accurately.
        //
        // We could clear it always, but we'd have more expensive restyles for
        // children.
        //
        // Note that we can't just keep stored other kind of non-layout object
        // computed style (like the one that gets set when getComputedStyle is
        // called on a display: none element), because that is a sizable memory
        // hit.
        //
        // Also, we don't want to leave a stale computed style, which may happen
        // if we don't end up calling recalcOwnStyle because there's no parent
        // style.
        const ComputedStyle* non_layout_style = NonLayoutObjectComputedStyle();
        if (!non_layout_style ||
            !ShouldStoreNonLayoutObjectComputedStyle(*non_layout_style) ||
            !ParentComputedStyle()) {
          data->ClearComputedStyle();
        }
      }

      if (change >= kIndependentInherit) {
        if (ElementAnimations* element_animations =
                data->GetElementAnimations())
          element_animations->SetAnimationStyleChange(false);
      }
    }
    if (ParentComputedStyle()) {
      change = RecalcOwnStyle(change);
    } else if (NeedsAttach()) {
      if (!CanParticipateInFlatTree()) {
        // Recalculate style for reattachment of Shadow DOM v0 <content>
        // fallback.
        RecalcShadowIncludingDescendantStylesForReattach();
      }
      SetNeedsReattachLayoutTree();
      change = kReattach;
    }

    // Needed because the rebuildLayoutTree code needs to see what the
    // styleChangeType() was on reattach roots. See Node::reattachLayoutTree()
    // for an example.
    if (change != kReattach)
      ClearNeedsStyleRecalc();
  }

  // If we are going to reattach we don't need to recalc the style of
  // our descendants anymore.
  if (change < kReattach &&
      (change >= kUpdatePseudoElements || ChildNeedsStyleRecalc())) {
    SelectorFilterParentScope filter_scope(*this);

    UpdatePseudoElement(kPseudoIdBefore, change);

    if (change > kUpdatePseudoElements || ChildNeedsStyleRecalc()) {
      if (ShadowRoot* root = GetShadowRoot()) {
        if (root->ShouldCallRecalcStyle(change))
          root->RecalcStyle(change);
      }
      RecalcDescendantStyles(change);
    }

    UpdatePseudoElement(kPseudoIdAfter, change);
    UpdatePseudoElement(kPseudoIdBackdrop, change);

    // If our children have changed then we need to force the first-letter
    // checks as we don't know if they effected the first letter or not.
    // This can be seen when a child transitions from floating to
    // non-floating we have to take it into account for the first letter.
    UpdatePseudoElement(kPseudoIdFirstLetter,
                        ChildNeedsStyleRecalc() ? kForce : change);

    ClearChildNeedsStyleRecalc();
  }

  if (HasCustomStyleCallbacks())
    DidRecalcStyle(change);
}

scoped_refptr<ComputedStyle> Element::PropagateInheritedProperties(
    StyleRecalcChange change) {
  if (change != kIndependentInherit)
    return nullptr;
  if (IsPseudoElement())
    return nullptr;
  if (NeedsStyleRecalc())
    return nullptr;
  if (HasAnimations())
    return nullptr;
  const ComputedStyle* parent_style = ParentComputedStyle();
  DCHECK(parent_style);
  const ComputedStyle* style = GetComputedStyle();
  if (!style || style->Animations() || style->Transitions())
    return nullptr;
  scoped_refptr<ComputedStyle> new_style = ComputedStyle::Clone(*style);
  new_style->PropagateIndependentInheritedProperties(*parent_style);
  INCREMENT_STYLE_STATS_COUNTER(GetDocument().GetStyleEngine(),
                                independent_inherited_styles_propagated, 1);
  return new_style;
}

StyleRecalcChange Element::RecalcOwnStyle(StyleRecalcChange change) {
  DCHECK(GetDocument().InStyleRecalc());
  DCHECK(!ParentOrShadowHostNode()->NeedsStyleRecalc());
  DCHECK(change >= kIndependentInherit || NeedsStyleRecalc());
  DCHECK(ParentComputedStyle());

  scoped_refptr<const ComputedStyle> old_style = GetComputedStyle();

  // When propagating inherited changes, we don't need to do a full style recalc
  // if the only changed properties are independent. In this case, we can simply
  // set these directly on the ComputedStyle object.
  scoped_refptr<ComputedStyle> new_style = PropagateInheritedProperties(change);
  if (!new_style)
    new_style = StyleForLayoutObject();
  if (!new_style)
    return kReattach;

  StyleRecalcChange local_change =
      ComputedStyle::StylePropagationDiff(old_style.get(), new_style.get());
  if (local_change == kNoChange) {
    INCREMENT_STYLE_STATS_COUNTER(GetDocument().GetStyleEngine(),
                                  styles_unchanged, 1);
  } else {
    INCREMENT_STYLE_STATS_COUNTER(GetDocument().GetStyleEngine(),
                                  styles_changed, 1);
    if (this == GetDocument().documentElement()) {
      if (GetDocument().GetStyleEngine().UpdateRemUnits(old_style.get(),
                                                        new_style.get())) {
        // Trigger a full document recalc on rem unit changes. We could keep
        // track of which elements depend on rem units like we do for viewport
        // styles, but we assume root font size changes are rare and just
        // recalculate everything.
        if (local_change < kForce)
          local_change = kForce;
      }
    }
  }

  if (local_change == kReattach) {
    SetNonAttachedStyle(new_style);
    SetNeedsReattachLayoutTree();
    if (LayoutObjectIsNeeded(*new_style) ||
        ShouldStoreNonLayoutObjectComputedStyle(*new_style)) {
      RecalcShadowIncludingDescendantStylesForReattach();
    }
    return kReattach;
  }

  DCHECK(old_style);

  if (local_change != kNoChange)
    UpdateCallbackSelectors(old_style.get(), new_style.get());

  if (LayoutObject* layout_object = GetLayoutObject()) {
    // kNoChange may mean that the computed style didn't change, but there are
    // additional flags in ComputedStyle which may have changed. For instance,
    // the AffectedBy* flags. We don't need to go through the visual
    // invalidation diffing in that case, but we replace the old ComputedStyle
    // object with the new one to ensure the mentioned flags are up to date.
    if (local_change == kNoChange)
      layout_object->SetStyleInternal(new_style.get());
    else
      layout_object->SetStyle(new_style.get());
  } else {
    if (ShouldStoreNonLayoutObjectComputedStyle(*new_style))
      StoreNonLayoutObjectComputedStyle(new_style);
    else if (HasRareData())
      GetElementRareData()->ClearComputedStyle();
  }

  if (GetStyleChangeType() >= kSubtreeStyleChange)
    return kForce;

  if (change > kInherit || local_change > kInherit)
    return max(local_change, change);

  if (local_change < kIndependentInherit) {
    if (old_style->HasChildDependentFlags()) {
      if (ChildNeedsStyleRecalc())
        return kInherit;
      new_style->CopyChildDependentFlagsFrom(*old_style);
    }
    if (old_style->HasPseudoElementStyle() ||
        new_style->HasPseudoElementStyle())
      return kUpdatePseudoElements;
  }

  return local_change;
}

void Element::RecalcStyleForReattach() {
  if (HasCustomStyleCallbacks())
    WillRecalcStyle(kReattach);

  bool recalc_descendants = false;
  if (ParentComputedStyle()) {
    scoped_refptr<ComputedStyle> non_attached_style = StyleForLayoutObject();
    SetNeedsReattachLayoutTree();
    SetNonAttachedStyle(non_attached_style);
    recalc_descendants =
        LayoutObjectIsNeeded(*non_attached_style) ||
        ShouldStoreNonLayoutObjectComputedStyle(*non_attached_style);
  } else {
    // Elements which cannot participate in the flat tree are <content> and
    // <slot> if SlotInFlatTree is not enabled. Even though we should not
    // compute their styles for re-attachment, we may need to compute their
    // children's style if fallback is rendered.
    recalc_descendants = !CanParticipateInFlatTree();
  }
  if (recalc_descendants)
    RecalcShadowIncludingDescendantStylesForReattach();

  if (HasCustomStyleCallbacks())
    DidRecalcStyle(kReattach);
}

void Element::RecalcShadowIncludingDescendantStylesForReattach() {
  if (!ChildrenCanHaveStyle())
    return;
  SelectorFilterParentScope filterScope(*this);
  RecalcShadowRootStylesForReattach();
  RecalcDescendantStylesForReattach();
}

void Element::RecalcShadowRootStylesForReattach() {
  if (ShadowRoot* root = GetShadowRoot())
    root->RecalcStylesForReattach();
}

void Element::RebuildLayoutTree(WhitespaceAttacher& whitespace_attacher) {
  DCHECK(InActiveDocument());
  DCHECK(parentNode());

  if (NeedsReattachLayoutTree()) {
    AttachContext reattach_context;
    ReattachLayoutTree(reattach_context);
    whitespace_attacher.DidReattachElement(this,
                                           reattach_context.previous_in_flow);
  } else {
    SelectorFilterParentScope filter_scope(*this);
    // We create a local WhitespaceAttacher when rebuilding children of an
    // element with a LayoutObject since whitespace nodes do not rely on layout
    // objects further up the tree. Also, if this Element's layout object is an
    // out-of-flow box, in-flow children should not affect whitespace siblings
    // of the out-of-flow box. However, if this element is a display:contents
    // element. Continue using the passed in attacher as display:contents
    // children may affect whitespace nodes further up the tree as they may be
    // layout tree siblings.
    WhitespaceAttacher local_attacher;
    WhitespaceAttacher* child_attacher;
    if (GetLayoutObject()) {
      whitespace_attacher.DidVisitElement(this);
      if (GetDocument().GetStyleEngine().NeedsWhitespaceReattachment(this))
        local_attacher.SetReattachAllWhitespaceNodes();
      child_attacher = &local_attacher;
    } else {
      child_attacher = &whitespace_attacher;
    }
    RebuildPseudoElementLayoutTree(kPseudoIdAfter, *child_attacher);
    if (GetShadowRoot())
      RebuildShadowRootLayoutTree(*child_attacher);
    else
      RebuildChildrenLayoutTrees(*child_attacher);
    RebuildPseudoElementLayoutTree(kPseudoIdBefore, *child_attacher);
    RebuildPseudoElementLayoutTree(kPseudoIdBackdrop, *child_attacher);
    RebuildPseudoElementLayoutTree(kPseudoIdFirstLetter, *child_attacher);
  }
  DCHECK(!NeedsStyleRecalc());
  DCHECK(!ChildNeedsStyleRecalc());
  DCHECK(!NeedsReattachLayoutTree());
  DCHECK(!GetNonAttachedStyle());
  DCHECK(!ChildNeedsReattachLayoutTree());
}

void Element::RebuildShadowRootLayoutTree(
    WhitespaceAttacher& whitespace_attacher) {
  DCHECK(IsShadowHost(this));
  ShadowRoot* root = GetShadowRoot();
  root->RebuildLayoutTree(whitespace_attacher);
  RebuildNonDistributedChildren();
}

void Element::RebuildPseudoElementLayoutTree(
    PseudoId pseudo_id,
    WhitespaceAttacher& whitespace_attacher) {
  PseudoElement* element = GetPseudoElement(pseudo_id);
  if (element) {
    if (pseudo_id == kPseudoIdFirstLetter && UpdateFirstLetter(element))
      return;
  } else {
    element = CreatePseudoElementIfNeeded(pseudo_id);
    if (!element)
      return;
  }

  if (element->NeedsRebuildLayoutTree(whitespace_attacher))
    element->RebuildLayoutTree(whitespace_attacher);
}

void Element::UpdateCallbackSelectors(const ComputedStyle* old_style,
                                      const ComputedStyle* new_style) {
  Vector<String> empty_vector;
  const Vector<String>& old_callback_selectors =
      old_style ? old_style->CallbackSelectors() : empty_vector;
  const Vector<String>& new_callback_selectors =
      new_style ? new_style->CallbackSelectors() : empty_vector;
  if (old_callback_selectors.IsEmpty() && new_callback_selectors.IsEmpty())
    return;
  if (old_callback_selectors != new_callback_selectors)
    CSSSelectorWatch::From(GetDocument())
        .UpdateSelectorMatches(old_callback_selectors, new_callback_selectors);
}

void Element::AddCallbackSelectors() {
  UpdateCallbackSelectors(nullptr, GetComputedStyle());
}

void Element::RemoveCallbackSelectors() {
  UpdateCallbackSelectors(GetComputedStyle(), nullptr);
}

ShadowRoot& Element::CreateAndAttachShadowRoot(ShadowRootType type) {
  EventDispatchForbiddenScope assert_no_event_dispatch;
  ScriptForbiddenScope forbid_script;

  DCHECK(!GetShadowRoot());

  ShadowRoot* shadow_root = ShadowRoot::Create(GetDocument(), type);
  EnsureElementRareData().SetShadowRoot(*shadow_root);
  shadow_root->SetParentOrShadowHostNode(this);
  shadow_root->SetParentTreeScope(GetTreeScope());
  if (type == ShadowRootType::V0) {
    shadow_root->SetNeedsDistributionRecalc();
  } else {
    for (Node& child : NodeTraversal::ChildrenOf(*this))
      child.LazyReattachIfAttached();
  }

  shadow_root->InsertedInto(this);
  SetChildNeedsStyleRecalc();
  SetNeedsStyleRecalc(kSubtreeStyleChange, StyleChangeReasonForTracing::Create(
                                               StyleChangeReason::kShadow));

  probe::didPushShadowRoot(this, shadow_root);

  return *shadow_root;
}

// TODO(kochi): inline this.
ShadowRoot* Element::GetShadowRoot() const {
  return HasRareData() ? GetElementRareData()->GetShadowRoot() : nullptr;
}

void Element::PseudoStateChanged(CSSSelector::PseudoType pseudo) {
  // We can't schedule invaliation sets from inside style recalc otherwise
  // we'd never process them.
  // TODO(esprehn): Make this an ASSERT and fix places that call into this
  // like HTMLSelectElement.
  if (GetDocument().InStyleRecalc())
    return;
  GetDocument().GetStyleEngine().PseudoStateChangedForElement(pseudo, *this);
}

void Element::SetAnimationStyleChange(bool animation_style_change) {
  if (animation_style_change && GetDocument().InStyleRecalc())
    return;
  if (!HasRareData())
    return;
  if (ElementAnimations* element_animations =
          GetElementRareData()->GetElementAnimations())
    element_animations->SetAnimationStyleChange(animation_style_change);
}

void Element::ClearAnimationStyleChange() {
  if (!HasRareData())
    return;
  if (ElementAnimations* element_animations =
          GetElementRareData()->GetElementAnimations())
    element_animations->SetAnimationStyleChange(false);
}

void Element::SetNeedsAnimationStyleRecalc() {
  if (GetStyleChangeType() != kNoStyleChange)
    return;

  SetNeedsStyleRecalc(kLocalStyleChange, StyleChangeReasonForTracing::Create(
                                             StyleChangeReason::kAnimation));
  SetAnimationStyleChange(true);
}

void Element::SetNeedsCompositingUpdate() {
  if (!GetDocument().IsActive())
    return;
  LayoutBoxModelObject* layout_object = GetLayoutBoxModelObject();
  if (!layout_object)
    return;
  if (!layout_object->HasLayer())
    return;
  layout_object->Layer()->SetNeedsCompositingInputsUpdate();
  // Changes in the return value of requiresAcceleratedCompositing change if
  // the PaintLayer is self-painting.
  layout_object->Layer()->UpdateSelfPaintingLayer();
}

void Element::V0SetCustomElementDefinition(
    V0CustomElementDefinition* definition) {
  if (!HasRareData() && !definition)
    return;
  DCHECK(!GetV0CustomElementDefinition());
  EnsureElementRareData().V0SetCustomElementDefinition(definition);
}

V0CustomElementDefinition* Element::GetV0CustomElementDefinition() const {
  if (HasRareData())
    return GetElementRareData()->GetV0CustomElementDefinition();
  return nullptr;
}

void Element::SetCustomElementDefinition(CustomElementDefinition* definition) {
  DCHECK(definition);
  DCHECK(!GetCustomElementDefinition());
  EnsureElementRareData().SetCustomElementDefinition(definition);
  SetCustomElementState(CustomElementState::kCustom);
}

CustomElementDefinition* Element::GetCustomElementDefinition() const {
  if (HasRareData())
    return GetElementRareData()->GetCustomElementDefinition();
  return nullptr;
}

void Element::SetIsValue(const AtomicString& is_value) {
  DCHECK(IsValue().IsNull()) << "SetIsValue() should be called at most once.";
  EnsureElementRareData().SetIsValue(is_value);
}

const AtomicString& Element::IsValue() const {
  if (HasRareData())
    return GetElementRareData()->IsValue();
  return g_null_atom;
}

ShadowRoot* Element::createShadowRoot(const ScriptState* script_state,
                                      ExceptionState& exception_state) {
  HostsUsingFeatures::CountMainWorldOnly(
      script_state, GetDocument(),
      HostsUsingFeatures::Feature::kElementCreateShadowRoot);
  if (ShadowRoot* root = GetShadowRoot()) {
    if (root->IsUserAgent()) {
      exception_state.ThrowDOMException(
          kInvalidStateError,
          "Shadow root cannot be created on a host which already hosts a "
          "user-agent shadow tree.");
    } else {
      exception_state.ThrowDOMException(
          kInvalidStateError,
          "Shadow root cannot be created on a host which already hosts a "
          "shadow tree.");
    }
    return nullptr;
  }
  if (AlwaysCreateUserAgentShadowRoot()) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "Shadow root cannot be created on a host which already hosts a "
        "user-agent shadow tree.");
    return nullptr;
  }
  // Some elements make assumptions about what kind of layoutObjects they allow
  // as children so we can't allow author shadows on them for now.
  if (!AreAuthorShadowsAllowed()) {
    exception_state.ThrowDOMException(
        kHierarchyRequestError,
        "Author-created shadow roots are disabled for this element.");
    return nullptr;
  }

  return &CreateShadowRootInternal();
}

bool Element::CanAttachShadowRoot() const {
  const AtomicString& tag_name = localName();
  // Checking Is{V0}CustomElement() here is just an optimization
  // because IsValidName is not cheap.
  return (IsCustomElement() && CustomElement::IsValidName(tag_name)) ||
         (IsV0CustomElement() && V0CustomElement::IsValidName(tag_name)) ||
         tag_name == HTMLNames::articleTag || tag_name == HTMLNames::asideTag ||
         tag_name == HTMLNames::blockquoteTag ||
         tag_name == HTMLNames::bodyTag || tag_name == HTMLNames::divTag ||
         tag_name == HTMLNames::footerTag || tag_name == HTMLNames::h1Tag ||
         tag_name == HTMLNames::h2Tag || tag_name == HTMLNames::h3Tag ||
         tag_name == HTMLNames::h4Tag || tag_name == HTMLNames::h5Tag ||
         tag_name == HTMLNames::h6Tag || tag_name == HTMLNames::headerTag ||
         tag_name == HTMLNames::navTag || tag_name == HTMLNames::mainTag ||
         tag_name == HTMLNames::pTag || tag_name == HTMLNames::sectionTag ||
         tag_name == HTMLNames::spanTag;
}

ShadowRoot* Element::attachShadow(const ScriptState* script_state,
                                  const ShadowRootInit& shadow_root_init_dict,
                                  ExceptionState& exception_state) {
  DCHECK(shadow_root_init_dict.hasMode());
  HostsUsingFeatures::CountMainWorldOnly(
      script_state, GetDocument(),
      HostsUsingFeatures::Feature::kElementAttachShadow);

  if (!CanAttachShadowRoot()) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "This element does not support attachShadow");
    return nullptr;
  }

  if (GetShadowRoot()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "Shadow root cannot be created on a host "
                                      "which already hosts a shadow tree.");
    return nullptr;
  }

  ShadowRootType type = shadow_root_init_dict.mode() == "open"
                            ? ShadowRootType::kOpen
                            : ShadowRootType::kClosed;

  if (type == ShadowRootType::kOpen)
    UseCounter::Count(GetDocument(), WebFeature::kElementAttachShadowOpen);
  else
    UseCounter::Count(GetDocument(), WebFeature::kElementAttachShadowClosed);

  DCHECK(!shadow_root_init_dict.hasMode() || !GetShadowRoot());
  bool delegates_focus = shadow_root_init_dict.hasDelegatesFocus() &&
                         shadow_root_init_dict.delegatesFocus();
  return &AttachShadowRootInternal(type, delegates_focus);
}

ShadowRoot& Element::CreateShadowRootInternal() {
  DCHECK(!ClosedShadowRoot());
  DCHECK(AreAuthorShadowsAllowed());
  DCHECK(!AlwaysCreateUserAgentShadowRoot());
  GetDocument().SetShadowCascadeOrder(ShadowCascadeOrder::kShadowCascadeV0);
  return CreateAndAttachShadowRoot(ShadowRootType::V0);
}

ShadowRoot& Element::CreateUserAgentShadowRoot() {
  DCHECK(!GetShadowRoot());
  return CreateAndAttachShadowRoot(ShadowRootType::kUserAgent);
}

ShadowRoot& Element::AttachShadowRootInternal(ShadowRootType type,
                                              bool delegates_focus) {
  // SVG <use> is a special case for using this API to create a closed shadow
  // root.
  DCHECK(CanAttachShadowRoot() || IsSVGUseElement(*this));
  DCHECK(type == ShadowRootType::kOpen || type == ShadowRootType::kClosed)
      << type;
  DCHECK(!AlwaysCreateUserAgentShadowRoot());

  GetDocument().SetShadowCascadeOrder(ShadowCascadeOrder::kShadowCascadeV1);
  ShadowRoot& shadow_root = CreateAndAttachShadowRoot(type);
  shadow_root.SetDelegatesFocus(delegates_focus);
  return shadow_root;
}

ShadowRoot* Element::OpenShadowRoot() const {
  ShadowRoot* root = GetShadowRoot();
  if (!root)
    return nullptr;
  return root->GetType() == ShadowRootType::V0 ||
                 root->GetType() == ShadowRootType::kOpen
             ? root
             : nullptr;
}

ShadowRoot* Element::ClosedShadowRoot() const {
  ShadowRoot* root = GetShadowRoot();
  if (!root)
    return nullptr;
  return root->GetType() == ShadowRootType::kClosed ? root : nullptr;
}

ShadowRoot* Element::AuthorShadowRoot() const {
  ShadowRoot* root = GetShadowRoot();
  if (!root)
    return nullptr;
  return !root->IsUserAgent() ? root : nullptr;
}

ShadowRoot* Element::UserAgentShadowRoot() const {
  ShadowRoot* root = GetShadowRoot();
  DCHECK(!root || root->IsUserAgent());
  return root;
}

ShadowRoot& Element::EnsureUserAgentShadowRoot() {
  if (ShadowRoot* shadow_root = UserAgentShadowRoot()) {
    DCHECK(shadow_root->GetType() == ShadowRootType::kUserAgent);
    return *shadow_root;
  }
  ShadowRoot& shadow_root =
      CreateAndAttachShadowRoot(ShadowRootType::kUserAgent);
  DidAddUserAgentShadowRoot(shadow_root);
  return shadow_root;
}

bool Element::ChildTypeAllowed(NodeType type) const {
  switch (type) {
    case kElementNode:
    case kTextNode:
    case kCommentNode:
    case kProcessingInstructionNode:
    case kCdataSectionNode:
      return true;
    default:
      break;
  }
  return false;
}

namespace {

bool HasSiblingsForNonEmpty(const Node* sibling,
                            Node* (*next_func)(const Node&)) {
  for (; sibling; sibling = next_func(*sibling)) {
    if (sibling->IsElementNode())
      return true;
    if (sibling->IsTextNode() && !ToText(sibling)->data().IsEmpty())
      return true;
  }
  return false;
}

}  // namespace

void Element::CheckForEmptyStyleChange(const Node* node_before_change,
                                       const Node* node_after_change) {
  if (!InActiveDocument())
    return;
  if (!StyleAffectedByEmpty())
    return;
  if (HasSiblingsForNonEmpty(node_before_change,
                             NodeTraversal::PreviousSibling) ||
      HasSiblingsForNonEmpty(node_after_change, NodeTraversal::NextSibling)) {
    return;
  }
  PseudoStateChanged(CSSSelector::kPseudoEmpty);
}

void Element::ChildrenChanged(const ChildrenChange& change) {
  ContainerNode::ChildrenChanged(change);

  CheckForEmptyStyleChange(change.sibling_before_change,
                           change.sibling_after_change);

  if (!change.by_parser && change.IsChildElementChange())
    CheckForSiblingStyleChanges(
        change.type == kElementRemoved ? kSiblingElementRemoved
                                       : kSiblingElementInserted,
        ToElement(change.sibling_changed), change.sibling_before_change,
        change.sibling_after_change);

  // TODO(hayato): Confirm that we can skip this if a shadow tree is v1.
  if (ShadowRoot* shadow_root = GetShadowRoot())
    shadow_root->SetNeedsDistributionRecalcWillBeSetNeedsAssignmentRecalc();
}

void Element::FinishParsingChildren() {
  SetIsFinishedParsingChildren(true);
  CheckForEmptyStyleChange(this, this);
  CheckForSiblingStyleChanges(kFinishedParsingChildren, nullptr, lastChild(),
                              nullptr);
}

AttrNodeList* Element::GetAttrNodeList() {
  return HasRareData() ? GetElementRareData()->GetAttrNodeList() : nullptr;
}

void Element::RemoveAttrNodeList() {
  DCHECK(GetAttrNodeList());
  if (HasRareData())
    GetElementRareData()->RemoveAttrNodeList();
}

Attr* Element::setAttributeNode(Attr* attr_node,
                                ExceptionState& exception_state) {
  Attr* old_attr_node = AttrIfExists(attr_node->GetQualifiedName());
  if (old_attr_node == attr_node)
    return attr_node;  // This Attr is already attached to the element.

  // InUseAttributeError: Raised if node is an Attr that is already an attribute
  // of another Element object.  The DOM user must explicitly clone Attr nodes
  // to re-use them in other elements.
  if (attr_node->ownerElement()) {
    exception_state.ThrowDOMException(
        kInUseAttributeError,
        "The node provided is an attribute node that is already an attribute "
        "of another Element; attribute nodes must be explicitly cloned.");
    return nullptr;
  }

  if (!IsHTMLElement() && attr_node->GetDocument().IsHTMLDocument() &&
      attr_node->name() != attr_node->name().LowerASCII())
    UseCounter::Count(
        GetDocument(),
        WebFeature::
            kNonHTMLElementSetAttributeNodeFromHTMLDocumentNameNotLowercase);

  SynchronizeAllAttributes();
  const UniqueElementData& element_data = EnsureUniqueElementData();

  AttributeCollection attributes = element_data.Attributes();
  size_t index = attributes.FindIndex(attr_node->GetQualifiedName());
  AtomicString local_name;
  if (index != kNotFound) {
    const Attribute& attr = attributes[index];

    // If the name of the ElementData attribute doesn't
    // (case-sensitively) match that of the Attr node, record it
    // on the Attr so that it can correctly resolve the value on
    // the Element.
    if (!attr.GetName().Matches(attr_node->GetQualifiedName()))
      local_name = attr.LocalName();

    if (old_attr_node) {
      DetachAttrNodeFromElementWithValue(old_attr_node, attr.Value());
    } else {
      // FIXME: using attrNode's name rather than the
      // Attribute's for the replaced Attr is compatible with
      // all but Gecko (and, arguably, the DOM Level1 spec text.)
      // Consider switching.
      old_attr_node = Attr::Create(GetDocument(), attr_node->GetQualifiedName(),
                                   attr.Value());
    }
  }

  SetAttributeInternal(index, attr_node->GetQualifiedName(), attr_node->value(),
                       kNotInSynchronizationOfLazyAttribute);

  attr_node->AttachToElement(this, local_name);
  GetTreeScope().AdoptIfNeeded(*attr_node);
  EnsureElementRareData().AddAttr(attr_node);

  return old_attr_node;
}

Attr* Element::setAttributeNodeNS(Attr* attr, ExceptionState& exception_state) {
  return setAttributeNode(attr, exception_state);
}

Attr* Element::removeAttributeNode(Attr* attr,
                                   ExceptionState& exception_state) {
  if (attr->ownerElement() != this) {
    exception_state.ThrowDOMException(
        kNotFoundError, "The node provided is owned by another element.");
    return nullptr;
  }

  DCHECK_EQ(GetDocument(), attr->GetDocument());

  SynchronizeAttribute(attr->GetQualifiedName());

  size_t index =
      GetElementData()->Attributes().FindIndex(attr->GetQualifiedName());
  if (index == kNotFound) {
    exception_state.ThrowDOMException(
        kNotFoundError, "The attribute was not found on this element.");
    return nullptr;
  }

  DetachAttrNodeAtIndex(attr, index);
  return attr;
}

void Element::ParseAttribute(const AttributeModificationParams& params) {
  if (params.name == tabindexAttr) {
    int tabindex = 0;
    if (params.new_value.IsEmpty() ||
        !ParseHTMLInteger(params.new_value, tabindex)) {
      ClearTabIndexExplicitlyIfNeeded();
    } else {
      // We only set when value is in integer range.
      SetTabIndexExplicitly();
    }
  } else if (params.name == XMLNames::langAttr) {
    PseudoStateChanged(CSSSelector::kPseudoLang);
  }
}

bool Element::ParseAttributeName(QualifiedName& out,
                                 const AtomicString& namespace_uri,
                                 const AtomicString& qualified_name,
                                 ExceptionState& exception_state) {
  AtomicString prefix, local_name;
  if (!Document::ParseQualifiedName(qualified_name, prefix, local_name,
                                    exception_state))
    return false;
  DCHECK(!exception_state.HadException());

  QualifiedName q_name(prefix, local_name, namespace_uri);

  if (!Document::HasValidNamespaceForAttributes(q_name)) {
    exception_state.ThrowDOMException(
        kNamespaceError,
        "'" + namespace_uri + "' is an invalid namespace for attributes.");
    return false;
  }

  out = q_name;
  return true;
}

void Element::setAttributeNS(const AtomicString& namespace_uri,
                             const AtomicString& qualified_name,
                             const AtomicString& value,
                             ExceptionState& exception_state) {
  QualifiedName parsed_name = g_any_name;
  if (!ParseAttributeName(parsed_name, namespace_uri, qualified_name,
                          exception_state))
    return;
  setAttribute(parsed_name, value);
}

void Element::RemoveAttributeInternal(
    size_t index,
    SynchronizationOfLazyAttribute in_synchronization_of_lazy_attribute) {
  MutableAttributeCollection attributes =
      EnsureUniqueElementData().Attributes();
  SECURITY_DCHECK(index < attributes.size());

  QualifiedName name = attributes[index].GetName();
  AtomicString value_being_removed = attributes[index].Value();

  if (!in_synchronization_of_lazy_attribute) {
    if (!value_being_removed.IsNull()) {
      WillModifyAttribute(name, value_being_removed, g_null_atom);
    } else if (GetCustomElementState() == CustomElementState::kCustom) {
      // This would otherwise be enqueued by willModifyAttribute.
      CustomElement::EnqueueAttributeChangedCallback(
          this, name, value_being_removed, g_null_atom);
    }
  }

  if (Attr* attr_node = AttrIfExists(name))
    DetachAttrNodeFromElementWithValue(attr_node, attributes[index].Value());

  attributes.Remove(index);

  if (!in_synchronization_of_lazy_attribute)
    DidRemoveAttribute(name, value_being_removed);
}

void Element::AppendAttributeInternal(
    const QualifiedName& name,
    const AtomicString& value,
    SynchronizationOfLazyAttribute in_synchronization_of_lazy_attribute) {
  if (!in_synchronization_of_lazy_attribute)
    WillModifyAttribute(name, g_null_atom, value);
  EnsureUniqueElementData().Attributes().Append(name, value);
  if (!in_synchronization_of_lazy_attribute)
    DidAddAttribute(name, value);
}

void Element::removeAttribute(const AtomicString& name) {
  if (!GetElementData())
    return;

  AtomicString local_name = LowercaseIfNecessary(name);
  size_t index = GetElementData()->Attributes().FindIndex(local_name);
  if (index == kNotFound) {
    if (UNLIKELY(local_name == styleAttr) &&
        GetElementData()->style_attribute_is_dirty_ && IsStyledElement())
      RemoveAllInlineStyleProperties();
    return;
  }

  RemoveAttributeInternal(index, kNotInSynchronizationOfLazyAttribute);
}

void Element::removeAttributeNS(const AtomicString& namespace_uri,
                                const AtomicString& local_name) {
  removeAttribute(QualifiedName(g_null_atom, local_name, namespace_uri));
}

Attr* Element::getAttributeNode(const AtomicString& local_name) {
  if (!GetElementData())
    return nullptr;
  SynchronizeAttribute(local_name);
  const Attribute* attribute =
      GetElementData()->Attributes().Find(LowercaseIfNecessary(local_name));
  if (!attribute)
    return nullptr;
  return EnsureAttr(attribute->GetName());
}

Attr* Element::getAttributeNodeNS(const AtomicString& namespace_uri,
                                  const AtomicString& local_name) {
  if (!GetElementData())
    return nullptr;
  QualifiedName q_name(g_null_atom, local_name, namespace_uri);
  SynchronizeAttribute(q_name);
  const Attribute* attribute = GetElementData()->Attributes().Find(q_name);
  if (!attribute)
    return nullptr;
  return EnsureAttr(attribute->GetName());
}

bool Element::hasAttribute(const AtomicString& local_name) const {
  if (!GetElementData())
    return false;
  SynchronizeAttribute(local_name);
  return GetElementData()->Attributes().FindIndex(
             LowercaseIfNecessary(local_name)) != kNotFound;
}

bool Element::hasAttributeNS(const AtomicString& namespace_uri,
                             const AtomicString& local_name) const {
  if (!GetElementData())
    return false;
  QualifiedName q_name(g_null_atom, local_name, namespace_uri);
  SynchronizeAttribute(q_name);
  return GetElementData()->Attributes().Find(q_name);
}

void Element::focus(FocusOptions options) {
  focus(FocusParams(SelectionBehaviorOnFocus::kRestore, kWebFocusTypeNone,
                    nullptr, options));
}

void Element::focus(const FocusParams& params) {
  if (!isConnected())
    return;

  if (GetDocument().FocusedElement() == this)
    return;

  if (!GetDocument().IsActive())
    return;

  if (IsFrameOwnerElement() &&
      ToHTMLFrameOwnerElement(this)->contentDocument() &&
      ToHTMLFrameOwnerElement(this)->contentDocument()->UnloadStarted())
    return;

  GetDocument().UpdateStyleAndLayoutTreeIgnorePendingStylesheets();
  if (!IsFocusable())
    return;

  if (AuthorShadowRoot() && AuthorShadowRoot()->delegatesFocus()) {
    if (IsShadowIncludingInclusiveAncestorOf(GetDocument().FocusedElement()))
      return;

    // Slide the focus to its inner node.
    Element* found = GetDocument()
                         .GetPage()
                         ->GetFocusController()
                         .FindFocusableElementInShadowHost(*this);
    if (found && IsShadowIncludingInclusiveAncestorOf(found)) {
      found->focus(FocusParams(SelectionBehaviorOnFocus::kReset,
                               kWebFocusTypeForward, nullptr, params.options));
      return;
    }
  }

  if (!GetDocument().GetPage()->GetFocusController().SetFocusedElement(
          this, GetDocument().GetFrame(), params))
    return;

  if (GetDocument().FocusedElement() == this &&
      GetDocument().GetFrame()->HasBeenActivated()) {
    // Bring up the keyboard in the context of anything triggered by a user
    // gesture. Since tracking that across arbitrary boundaries (eg.
    // animations) is difficult, for now we match IE's heuristic and bring
    // up the keyboard if there's been any gesture since load.
    GetDocument()
        .GetPage()
        ->GetChromeClient()
        .ShowVirtualKeyboardOnElementFocus(*GetDocument().GetFrame());
  }
}

void Element::UpdateFocusAppearance(
    SelectionBehaviorOnFocus selection_behavior) {
  UpdateFocusAppearanceWithOptions(selection_behavior, FocusOptions());
}

void Element::UpdateFocusAppearanceWithOptions(
    SelectionBehaviorOnFocus selection_behavior,
    const FocusOptions& options) {
  if (selection_behavior == SelectionBehaviorOnFocus::kNone)
    return;
  if (IsRootEditableElement(*this)) {
    LocalFrame* frame = GetDocument().GetFrame();
    if (!frame)
      return;

    // When focusing an editable element in an iframe, don't reset the selection
    // if it already contains a selection.
    if (this == frame->Selection()
                    .ComputeVisibleSelectionInDOMTreeDeprecated()
                    .RootEditableElement())
      return;

    // FIXME: We should restore the previous selection if there is one.
    // Passing DoNotSetFocus as this function is called after
    // FocusController::setFocusedElement() and we don't want to change the
    // focus to a new Element.
    frame->Selection().SetSelection(
        SelectionInDOMTree::Builder()
            .Collapse(FirstPositionInOrBeforeNode(*this))
            .Build(),
        SetSelectionOptions::Builder()
            .SetShouldCloseTyping(true)
            .SetShouldClearTypingStyle(true)
            .SetDoNotSetFocus(true)
            .Build());
    if (!options.preventScroll())
      frame->Selection().RevealSelection();
  } else if (GetLayoutObject() &&
             !GetLayoutObject()->IsLayoutEmbeddedContent()) {
    if (!options.preventScroll()) {
      GetLayoutObject()->ScrollRectToVisible(BoundingBoxForScrollIntoView(),
                                             WebScrollIntoViewParams());
    }
  }
}

void Element::blur() {
  CancelFocusAppearanceUpdate();
  if (AdjustedFocusedElementInTreeScope() == this) {
    Document& doc = GetDocument();
    if (doc.GetPage()) {
      doc.GetPage()->GetFocusController().SetFocusedElement(nullptr,
                                                            doc.GetFrame());
    } else {
      doc.ClearFocusedElement();
    }
  }
}

bool Element::SupportsFocus() const {
  // FIXME: supportsFocus() can be called when layout is not up to date.
  // Logic that deals with the layoutObject should be moved to
  // layoutObjectIsFocusable().
  // But supportsFocus must return true when the element is editable, or else
  // it won't be focusable. Furthermore, supportsFocus cannot just return true
  // always or else tabIndex() will change for all HTML elements.
  return HasElementFlag(ElementFlags::kTabIndexWasSetExplicitly) ||
         IsRootEditableElement(*this) ||
         (IsShadowHost(this) && AuthorShadowRoot() &&
          AuthorShadowRoot()->delegatesFocus()) ||
         SupportsSpatialNavigationFocus();
}

bool Element::SupportsSpatialNavigationFocus() const {
  // This function checks whether the element satisfies the extended criteria
  // for the element to be focusable, introduced by spatial navigation feature,
  // i.e. checks if click or keyboard event handler is specified.
  // This is the way to make it possible to navigate to (focus) elements
  // which web designer meant for being active (made them respond to click
  // events).
  if (!IsSpatialNavigationEnabled(GetDocument().GetFrame()) ||
      SpatialNavigationIgnoresEventHandlers(GetDocument().GetFrame()))
    return false;
  if (HasEventListeners(EventTypeNames::click) ||
      HasEventListeners(EventTypeNames::keydown) ||
      HasEventListeners(EventTypeNames::keypress) ||
      HasEventListeners(EventTypeNames::keyup))
    return true;
  if (!IsSVGElement())
    return false;
  return (HasEventListeners(EventTypeNames::focus) ||
          HasEventListeners(EventTypeNames::blur) ||
          HasEventListeners(EventTypeNames::focusin) ||
          HasEventListeners(EventTypeNames::focusout));
}

bool Element::IsFocusable() const {
  // Style cannot be cleared out for non-active documents, so in that case the
  // needsLayoutTreeUpdateForNode check is invalid.
  DCHECK(!GetDocument().IsActive() ||
         !GetDocument().NeedsLayoutTreeUpdateForNode(*this));
  return isConnected() && SupportsFocus() && !IsInert() && IsFocusableStyle();
}

bool Element::IsKeyboardFocusable() const {
  return IsFocusable() && tabIndex() >= 0;
}

bool Element::IsMouseFocusable() const {
  return IsFocusable();
}

bool Element::IsFocusedElementInDocument() const {
  return this == GetDocument().FocusedElement();
}

Element* Element::AdjustedFocusedElementInTreeScope() const {
  return IsInTreeScope() ? ContainingTreeScope().AdjustedFocusedElement()
                         : nullptr;
}

void Element::DispatchFocusEvent(Element* old_focused_element,
                                 WebFocusType type,
                                 InputDeviceCapabilities* source_capabilities) {
  DispatchEvent(FocusEvent::Create(EventTypeNames::focus, Event::Bubbles::kNo,
                                   GetDocument().domWindow(), 0,
                                   old_focused_element, source_capabilities));
}

void Element::DispatchBlurEvent(Element* new_focused_element,
                                WebFocusType type,
                                InputDeviceCapabilities* source_capabilities) {
  DispatchEvent(FocusEvent::Create(EventTypeNames::blur, Event::Bubbles::kNo,
                                   GetDocument().domWindow(), 0,
                                   new_focused_element, source_capabilities));
}

void Element::DispatchFocusInEvent(
    const AtomicString& event_type,
    Element* old_focused_element,
    WebFocusType,
    InputDeviceCapabilities* source_capabilities) {
#if DCHECK_IS_ON()
  DCHECK(!EventDispatchForbiddenScope::IsEventDispatchForbidden());
#endif
  DCHECK(event_type == EventTypeNames::focusin ||
         event_type == EventTypeNames::DOMFocusIn);
  DispatchScopedEvent(FocusEvent::Create(
      event_type, Event::Bubbles::kYes, GetDocument().domWindow(), 0,
      old_focused_element, source_capabilities));
}

void Element::DispatchFocusOutEvent(
    const AtomicString& event_type,
    Element* new_focused_element,
    InputDeviceCapabilities* source_capabilities) {
#if DCHECK_IS_ON()
  DCHECK(!EventDispatchForbiddenScope::IsEventDispatchForbidden());
#endif
  DCHECK(event_type == EventTypeNames::focusout ||
         event_type == EventTypeNames::DOMFocusOut);
  DispatchScopedEvent(FocusEvent::Create(
      event_type, Event::Bubbles::kYes, GetDocument().domWindow(), 0,
      new_focused_element, source_capabilities));
}

String Element::InnerHTMLAsString() const {
  return CreateMarkup(this, kChildrenOnly);
}

String Element::OuterHTMLAsString() const {
  return CreateMarkup(this);
}

void Element::innerHTML(StringOrTrustedHTML& result) const {
  result.SetString(InnerHTMLAsString());
}

void Element::outerHTML(StringOrTrustedHTML& result) const {
  result.SetString(OuterHTMLAsString());
}

void Element::SetInnerHTMLFromString(const String& html,
                                     ExceptionState& exception_state) {
  probe::breakableLocation(&GetDocument(), "Element.setInnerHTML");
  if (html.IsEmpty() && !HasNonInBodyInsertionMode()) {
    setTextContent(html);
  } else {
    if (DocumentFragment* fragment = CreateFragmentForInnerOuterHTML(
            html, this, kAllowScriptingContent, "innerHTML", exception_state)) {
      ContainerNode* container = this;
      if (auto* template_element = ToHTMLTemplateElementOrNull(*this))
        container = template_element->content();
      ReplaceChildrenWithFragment(container, fragment, exception_state);
    }
  }
}

void Element::SetInnerHTMLFromString(const String& html) {
  SetInnerHTMLFromString(html, ASSERT_NO_EXCEPTION);
}

void Element::setInnerHTML(const StringOrTrustedHTML& string_or_html,
                           ExceptionState& exception_state) {
  DCHECK(string_or_html.IsString() ||
         RuntimeEnabledFeatures::TrustedDOMTypesEnabled());

  if (string_or_html.IsString() && GetDocument().RequireTrustedTypes()) {
    exception_state.ThrowTypeError(
        "This document requires `TrustedHTML` assignment.");
    return;
  }

  String html = string_or_html.IsString()
                    ? string_or_html.GetAsString()
                    : string_or_html.GetAsTrustedHTML()->toString();

  SetInnerHTMLFromString(html, exception_state);
}

void Element::setInnerHTML(const StringOrTrustedHTML& string_or_html) {
  setInnerHTML(string_or_html, ASSERT_NO_EXCEPTION);
}

void Element::SetOuterHTMLFromString(const String& html,
                                     ExceptionState& exception_state) {
  Node* p = parentNode();
  if (!p) {
    exception_state.ThrowDOMException(kNoModificationAllowedError,
                                      "This element has no parent node.");
    return;
  }
  if (!p->IsElementNode()) {
    exception_state.ThrowDOMException(kNoModificationAllowedError,
                                      "This element's parent is of type '" +
                                          p->nodeName() +
                                          "', which is not an element node.");
    return;
  }

  Element* parent = ToElement(p);
  Node* prev = previousSibling();
  Node* next = nextSibling();

  DocumentFragment* fragment = CreateFragmentForInnerOuterHTML(
      html, parent, kAllowScriptingContent, "outerHTML", exception_state);
  if (exception_state.HadException())
    return;

  parent->ReplaceChild(fragment, this, exception_state);
  Node* node = next ? next->previousSibling() : nullptr;
  if (!exception_state.HadException() && node && node->IsTextNode())
    MergeWithNextTextNode(ToText(node), exception_state);

  if (!exception_state.HadException() && prev && prev->IsTextNode())
    MergeWithNextTextNode(ToText(prev), exception_state);
}

void Element::setOuterHTML(const StringOrTrustedHTML& string_or_html,
                           ExceptionState& exception_state) {
  DCHECK(string_or_html.IsString() ||
         RuntimeEnabledFeatures::TrustedDOMTypesEnabled());

  if (string_or_html.IsString() && GetDocument().RequireTrustedTypes()) {
    exception_state.ThrowTypeError(
        "This document requires `TrustedHTML` assignment.");
    return;
  }

  String html = string_or_html.IsString()
                    ? string_or_html.GetAsString()
                    : string_or_html.GetAsTrustedHTML()->toString();

  SetOuterHTMLFromString(html, exception_state);
}

Node* Element::InsertAdjacent(const String& where,
                              Node* new_child,
                              ExceptionState& exception_state) {
  if (DeprecatedEqualIgnoringCase(where, "beforeBegin")) {
    if (ContainerNode* parent = parentNode()) {
      parent->InsertBefore(new_child, this, exception_state);
      if (!exception_state.HadException())
        return new_child;
    }
    return nullptr;
  }

  if (DeprecatedEqualIgnoringCase(where, "afterBegin")) {
    InsertBefore(new_child, firstChild(), exception_state);
    return exception_state.HadException() ? nullptr : new_child;
  }

  if (DeprecatedEqualIgnoringCase(where, "beforeEnd")) {
    AppendChild(new_child, exception_state);
    return exception_state.HadException() ? nullptr : new_child;
  }

  if (DeprecatedEqualIgnoringCase(where, "afterEnd")) {
    if (ContainerNode* parent = parentNode()) {
      parent->InsertBefore(new_child, nextSibling(), exception_state);
      if (!exception_state.HadException())
        return new_child;
    }
    return nullptr;
  }

  exception_state.ThrowDOMException(
      kSyntaxError, "The value provided ('" + where +
                        "') is not one of 'beforeBegin', 'afterBegin', "
                        "'beforeEnd', or 'afterEnd'.");
  return nullptr;
}

ElementIntersectionObserverData* Element::IntersectionObserverData() const {
  if (HasRareData())
    return GetElementRareData()->IntersectionObserverData();
  return nullptr;
}

ElementIntersectionObserverData& Element::EnsureIntersectionObserverData() {
  return EnsureElementRareData().EnsureIntersectionObserverData();
}

HeapHashMap<TraceWrapperMember<ResizeObserver>, Member<ResizeObservation>>*
Element::ResizeObserverData() const {
  if (HasRareData())
    return GetElementRareData()->ResizeObserverData();
  return nullptr;
}

HeapHashMap<TraceWrapperMember<ResizeObserver>, Member<ResizeObservation>>&
Element::EnsureResizeObserverData() {
  return EnsureElementRareData().EnsureResizeObserverData();
}

void Element::SetNeedsResizeObserverUpdate() {
  if (auto* data = ResizeObserverData()) {
    for (auto& observation : data->Values())
      observation->ElementSizeChanged();
  }
}

void Element::WillBeginCustomizedScrollPhase(
    ScrollCustomization::ScrollDirection direction) {
  DCHECK(!GetScrollCustomizationCallbacks().InScrollPhase(this));
  LayoutBox* box = GetLayoutBox();
  if (!box)
    return;

  ScrollCustomization::ScrollDirection scroll_customization =
      box->Style()->ScrollCustomization();

  GetScrollCustomizationCallbacks().SetInScrollPhase(
      this, direction & scroll_customization);
}

void Element::DidEndCustomizedScrollPhase() {
  GetScrollCustomizationCallbacks().SetInScrollPhase(this, false);
}

// Step 1 of http://domparsing.spec.whatwg.org/#insertadjacenthtml()
static Element* ContextElementForInsertion(const String& where,
                                           Element* element,
                                           ExceptionState& exception_state) {
  if (DeprecatedEqualIgnoringCase(where, "beforeBegin") ||
      DeprecatedEqualIgnoringCase(where, "afterEnd")) {
    Element* parent = element->parentElement();
    if (!parent) {
      exception_state.ThrowDOMException(kNoModificationAllowedError,
                                        "The element has no parent.");
      return nullptr;
    }
    return parent;
  }
  if (DeprecatedEqualIgnoringCase(where, "afterBegin") ||
      DeprecatedEqualIgnoringCase(where, "beforeEnd"))
    return element;
  exception_state.ThrowDOMException(
      kSyntaxError, "The value provided ('" + where +
                        "') is not one of 'beforeBegin', 'afterBegin', "
                        "'beforeEnd', or 'afterEnd'.");
  return nullptr;
}

Element* Element::insertAdjacentElement(const String& where,
                                        Element* new_child,
                                        ExceptionState& exception_state) {
  Node* return_value = InsertAdjacent(where, new_child, exception_state);
  return ToElement(return_value);
}

void Element::insertAdjacentText(const String& where,
                                 const String& text,
                                 ExceptionState& exception_state) {
  InsertAdjacent(where, GetDocument().createTextNode(text), exception_state);
}

void Element::insertAdjacentHTML(const String& where,
                                 const String& markup,
                                 ExceptionState& exception_state) {
  Element* context_element =
      ContextElementForInsertion(where, this, exception_state);
  if (!context_element)
    return;

  DocumentFragment* fragment = CreateFragmentForInnerOuterHTML(
      markup, context_element, kAllowScriptingContent, "insertAdjacentHTML",
      exception_state);
  if (!fragment)
    return;
  InsertAdjacent(where, fragment, exception_state);
}

void Element::insertAdjacentHTML(const String& where,
                                 const StringOrTrustedHTML& string_or_html,
                                 ExceptionState& exception_state) {
  DCHECK(string_or_html.IsString() ||
         RuntimeEnabledFeatures::TrustedDOMTypesEnabled());

  if (string_or_html.IsString() && GetDocument().RequireTrustedTypes()) {
    exception_state.ThrowTypeError(
        "This document requires `TrustedHTML` assignment.");
    return;
  }

  String markup = string_or_html.IsString()
                      ? string_or_html.GetAsString()
                      : string_or_html.GetAsTrustedHTML()->toString();

  insertAdjacentHTML(where, markup, exception_state);
}

void Element::setPointerCapture(int pointer_id,
                                ExceptionState& exception_state) {
  if (GetDocument().GetFrame()) {
    if (!GetDocument().GetFrame()->GetEventHandler().IsPointerEventActive(
            pointer_id)) {
      exception_state.ThrowDOMException(kInvalidPointerId, "InvalidPointerId");
    } else if (!isConnected() ||
               (GetDocument().GetPage() && GetDocument()
                                               .GetPage()
                                               ->GetPointerLockController()
                                               .GetElement())) {
      exception_state.ThrowDOMException(kInvalidStateError,
                                        "InvalidStateError");
    } else {
      GetDocument().GetFrame()->GetEventHandler().SetPointerCapture(pointer_id,
                                                                    this);
    }
  }
}

void Element::releasePointerCapture(int pointer_id,
                                    ExceptionState& exception_state) {
  if (GetDocument().GetFrame()) {
    if (!GetDocument().GetFrame()->GetEventHandler().IsPointerEventActive(
            pointer_id))
      exception_state.ThrowDOMException(kInvalidPointerId, "InvalidPointerId");
    else
      GetDocument().GetFrame()->GetEventHandler().ReleasePointerCapture(
          pointer_id, this);
  }
}

bool Element::hasPointerCapture(int pointer_id) const {
  return GetDocument().GetFrame() &&
         GetDocument().GetFrame()->GetEventHandler().HasPointerCapture(
             pointer_id, this);
}

bool Element::HasProcessedPointerCapture(int pointer_id) const {
  return GetDocument().GetFrame() &&
         GetDocument().GetFrame()->GetEventHandler().HasProcessedPointerCapture(
             pointer_id, this);
}

String Element::innerText() {
  // We need to update layout, since plainText uses line boxes in the layout
  // tree.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(this);

  if (!GetLayoutObject())
    return textContent(true);

  return PlainText(
      EphemeralRange::RangeOfContents(*this),
      TextIteratorBehavior::Builder().SetForInnerText(true).Build());
}

String Element::outerText() {
  // Getting outerText is the same as getting innerText, only
  // setting is different. You would think this should get the plain
  // text for the outer range, but this is wrong, <br> for instance
  // would return different values for inner and outer text by such
  // a rule, but it doesn't in WinIE, and we want to match that.
  return innerText();
}

String Element::TextFromChildren() {
  Text* first_text_node = nullptr;
  bool found_multiple_text_nodes = false;
  unsigned total_length = 0;

  for (Node* child = firstChild(); child; child = child->nextSibling()) {
    if (!child->IsTextNode())
      continue;
    Text* text = ToText(child);
    if (!first_text_node)
      first_text_node = text;
    else
      found_multiple_text_nodes = true;
    unsigned length = text->data().length();
    if (length > std::numeric_limits<unsigned>::max() - total_length)
      return g_empty_string;
    total_length += length;
  }

  if (!first_text_node)
    return g_empty_string;

  if (first_text_node && !found_multiple_text_nodes) {
    first_text_node->Atomize();
    return first_text_node->data();
  }

  StringBuilder content;
  content.ReserveCapacity(total_length);
  for (Node* child = first_text_node; child; child = child->nextSibling()) {
    if (!child->IsTextNode())
      continue;
    content.Append(ToText(child)->data());
  }

  DCHECK_EQ(content.length(), total_length);
  return content.ToString();
}

const AtomicString& Element::ShadowPseudoId() const {
  if (ShadowRoot* root = ContainingShadowRoot()) {
    if (root->IsUserAgent())
      return FastGetAttribute(pseudoAttr);
  }
  return g_null_atom;
}

void Element::SetShadowPseudoId(const AtomicString& id) {
  DCHECK(CSSSelector::ParsePseudoType(id, false) ==
             CSSSelector::kPseudoWebKitCustomElement ||
         CSSSelector::ParsePseudoType(id, false) ==
             CSSSelector::kPseudoBlinkInternalElement);
  setAttribute(pseudoAttr, id);
}

bool Element::IsInDescendantTreeOf(const Element* shadow_host) const {
  DCHECK(shadow_host);
  DCHECK(IsShadowHost(shadow_host));

  for (const Element* ancestor_shadow_host = OwnerShadowHost();
       ancestor_shadow_host;
       ancestor_shadow_host = ancestor_shadow_host->OwnerShadowHost()) {
    if (ancestor_shadow_host == shadow_host)
      return true;
  }
  return false;
}

const ComputedStyle* Element::EnsureComputedStyle(
    PseudoId pseudo_element_specifier) {
  if (PseudoElement* element = GetPseudoElement(pseudo_element_specifier))
    return element->EnsureComputedStyle();

  if (!InActiveDocument()) {
    // FIXME: Try to do better than this. Ensure that styleForElement() works
    // for elements that are not in the document tree and figure out when to
    // destroy the computed style for such elements.
    return nullptr;
  }

  // FIXME: Find and use the layoutObject from the pseudo element instead of the
  // actual element so that the 'length' properties, which are only known by the
  // layoutObject because it did the layout, will be correct and so that the
  // values returned for the ":selection" pseudo-element will be correct.
  ComputedStyle* element_style = MutableComputedStyle();
  if (!element_style) {
    ElementRareData& rare_data = EnsureElementRareData();
    if (!rare_data.GetComputedStyle())
      rare_data.SetComputedStyle(
          GetDocument().StyleForElementIgnoringPendingStylesheets(this));
    element_style = rare_data.GetComputedStyle();
  }

  if (!pseudo_element_specifier)
    return element_style;

  if (ComputedStyle* pseudo_element_style =
          element_style->GetCachedPseudoStyle(pseudo_element_specifier))
    return pseudo_element_style;

  const ComputedStyle* layout_parent_style = element_style;
  if (HasDisplayContentsStyle()) {
    LayoutObject* parent_layout_object =
        LayoutTreeBuilderTraversal::ParentLayoutObject(*this);
    if (parent_layout_object)
      layout_parent_style = parent_layout_object->Style();
  }

  scoped_refptr<ComputedStyle> result =
      GetDocument().EnsureStyleResolver().PseudoStyleForElement(
          this,
          PseudoStyleRequest(pseudo_element_specifier,
                             PseudoStyleRequest::kForComputedStyle),
          element_style, layout_parent_style);
  DCHECK(result);
  return element_style->AddCachedPseudoStyle(std::move(result));
}

const ComputedStyle* Element::NonLayoutObjectComputedStyle() const {
  if (NeedsReattachLayoutTree())
    return GetNonAttachedStyle();

  if (!HasRareData())
    return nullptr;

  return GetElementRareData()->GetComputedStyle();
}

bool Element::HasDisplayContentsStyle() const {
  if (const ComputedStyle* style = NonLayoutObjectComputedStyle())
    return style->Display() == EDisplay::kContents;
  return false;
}

bool Element::ShouldStoreNonLayoutObjectComputedStyle(
    const ComputedStyle& style) const {
#if DCHECK_IS_ON()
  if (style.Display() == EDisplay::kContents && !NeedsReattachLayoutTree())
    DCHECK(!GetLayoutObject() || IsPseudoElement());
#endif
  if (style.Display() == EDisplay::kNone)
    return false;
  if (IsSVGElement()) {
    Element* parent_element = LayoutTreeBuilderTraversal::ParentElement(*this);
    if (parent_element && !parent_element->IsSVGElement())
      return false;
    if (IsSVGStopElement(*this))
      return true;
  }
  if (style.Display() == EDisplay::kContents)
    return true;
  return IsHTMLOptGroupElement(*this) || IsHTMLOptionElement(*this);
}

void Element::StoreNonLayoutObjectComputedStyle(
    scoped_refptr<ComputedStyle> style) {
  DCHECK(style);
  DCHECK(ShouldStoreNonLayoutObjectComputedStyle(*style));
  EnsureElementRareData().SetComputedStyle(std::move(style));
}

AtomicString Element::ComputeInheritedLanguage() const {
  const Node* n = this;
  AtomicString value;
  // The language property is inherited, so we iterate over the parents to find
  // the first language.
  do {
    if (n->IsElementNode()) {
      if (const ElementData* element_data = ToElement(n)->GetElementData()) {
        AttributeCollection attributes = element_data->Attributes();
        // Spec: xml:lang takes precedence -- http://www.w3.org/TR/xhtml1/#C_7
        if (const Attribute* attribute = attributes.Find(XMLNames::langAttr))
          value = attribute->Value();
        else if (const Attribute* attribute =
                     attributes.Find(HTMLNames::langAttr))
          value = attribute->Value();
      }
    } else if (n->IsDocumentNode()) {
      // checking the MIME content-language
      value = ToDocument(n)->ContentLanguage();
    }

    n = n->ParentOrShadowHostNode();
  } while (n && value.IsNull());

  return value;
}

Locale& Element::GetLocale() const {
  return GetDocument().GetCachedLocale(ComputeInheritedLanguage());
}

void Element::CancelFocusAppearanceUpdate() {
  if (GetDocument().FocusedElement() == this)
    GetDocument().CancelFocusAppearanceUpdate();
}

void Element::UpdatePseudoElement(PseudoId pseudo_id,
                                  StyleRecalcChange change) {
  DCHECK(!NeedsStyleRecalc());
  PseudoElement* element = GetPseudoElement(pseudo_id);

  if (element && (change == kUpdatePseudoElements ||
                  element->ShouldCallRecalcStyle(change))) {
    if (pseudo_id == kPseudoIdFirstLetter && UpdateFirstLetter(element))
      return;

    // Need to clear the cached style if the PseudoElement wants a recalc so it
    // computes a new style.
    if (element->NeedsStyleRecalc())
      MutableComputedStyle()->RemoveCachedPseudoStyle(pseudo_id);

    // PseudoElement styles hang off their parent element's style so if we
    // needed a style recalc we should Force one on the pseudo.
    element->RecalcStyle(change == kUpdatePseudoElements ? kForce : change);

    // Wait until our parent is not displayed or
    // PseudoElementLayoutObjectIsNeeded is false, otherwise we could
    // continuously create and destroy PseudoElements when
    // LayoutObject::IsChildAllowed on our parent returns false for the
    // PseudoElement's GetLayoutObject for each style recalc.
    if (!CanGeneratePseudoElement(pseudo_id) ||
        !PseudoElementLayoutObjectIsNeeded(
            PseudoStyle(PseudoStyleRequest(pseudo_id))))
      GetElementRareData()->SetPseudoElement(pseudo_id, nullptr);
  } else if (pseudo_id == kPseudoIdFirstLetter && element &&
             change >= kUpdatePseudoElements &&
             !FirstLetterPseudoElement::FirstLetterTextLayoutObject(*element)) {
    // This can happen if we change to a float, for example. We need to cleanup
    // the first-letter pseudoElement and then fix the text of the original
    // remaining text layoutObject.  This can be seen in Test 7 of
    // fast/css/first-letter-removed-added.html
    GetElementRareData()->SetPseudoElement(pseudo_id, nullptr);
  } else if (change >= kUpdatePseudoElements) {
    if (PseudoElement* new_pseudo = CreatePseudoElementIfNeeded(pseudo_id))
      new_pseudo->SetNeedsReattachLayoutTree();
  }
}

// If we're updating first letter, and the current first letter layoutObject
// is not the same as the one we're currently using we need to re-create
// the first letter layoutObject.
bool Element::UpdateFirstLetter(Element* element) {
  LayoutObject* remaining_text_layout_object =
      FirstLetterPseudoElement::FirstLetterTextLayoutObject(*element);
  if (!remaining_text_layout_object ||
      remaining_text_layout_object !=
          ToFirstLetterPseudoElement(element)->RemainingTextLayoutObject()) {
    // We have to clear out the old first letter here because when it is
    // disposed it will set the original text back on the remaining text
    // layoutObject. If we dispose after creating the new one we will get
    // incorrect results due to setting the first letter back.
    if (remaining_text_layout_object)
      element->ReattachLayoutTree();
    else
      GetElementRareData()->SetPseudoElement(kPseudoIdFirstLetter, nullptr);
    return true;
  }
  return false;
}

PseudoElement* Element::CreatePseudoElementIfNeeded(PseudoId pseudo_id) {
  if (IsPseudoElement())
    return nullptr;

  // Document::ensureStyleResolver is not inlined and shows up on profiles,
  // avoid it here.
  PseudoElement* element = GetDocument()
                               .GetStyleEngine()
                               .EnsureResolver()
                               .CreatePseudoElementIfNeeded(*this, pseudo_id);
  if (!element)
    return nullptr;

  if (pseudo_id == kPseudoIdBackdrop)
    GetDocument().AddToTopLayer(element, this);
  element->InsertedInto(this);

  probe::pseudoElementCreated(element);

  EnsureElementRareData().SetPseudoElement(pseudo_id, element);
  return element;
}

void Element::CreateAndAttachPseudoElementIfNeeded(PseudoId pseudo_id,
                                                   AttachContext& context) {
  if (PseudoElement* pseudo_element = CreatePseudoElementIfNeeded(pseudo_id))
    pseudo_element->AttachLayoutTree(context);
}

PseudoElement* Element::GetPseudoElement(PseudoId pseudo_id) const {
  return HasRareData() ? GetElementRareData()->GetPseudoElement(pseudo_id)
                       : nullptr;
}

LayoutObject* Element::PseudoElementLayoutObject(PseudoId pseudo_id) const {
  if (PseudoElement* element = GetPseudoElement(pseudo_id))
    return element->GetLayoutObject();
  return nullptr;
}

ComputedStyle* Element::PseudoStyle(const PseudoStyleRequest& request,
                                    const ComputedStyle* parent_style) {
  ComputedStyle* style = MutableComputedStyle();

  if (!style || (request.pseudo_id < kFirstInternalPseudoId &&
                 !style->HasPseudoStyle(request.pseudo_id))) {
    return nullptr;
  }

  if (ComputedStyle* cached = style->GetCachedPseudoStyle(request.pseudo_id))
    return cached;

  scoped_refptr<ComputedStyle> result =
      GetUncachedPseudoStyle(request, parent_style);
  if (result)
    return style->AddCachedPseudoStyle(std::move(result));
  return nullptr;
}

scoped_refptr<ComputedStyle> Element::GetUncachedPseudoStyle(
    const PseudoStyleRequest& request,
    const ComputedStyle* parent_style) {
  const ComputedStyle* style = GetComputedStyle();
  const bool is_before_or_after = request.pseudo_id == kPseudoIdBefore ||
                                  request.pseudo_id == kPseudoIdAfter;

  DCHECK(style);
  DCHECK(!parent_style || !is_before_or_after);

  if (is_before_or_after) {
    LayoutObject* parent_layout_object = GetLayoutObject();
    if (!parent_layout_object && HasDisplayContentsStyle()) {
      parent_layout_object =
          LayoutTreeBuilderTraversal::ParentLayoutObject(*this);
    }
    if (!parent_layout_object)
      return nullptr;
    return GetDocument().EnsureStyleResolver().PseudoStyleForElement(
        this, request, style, parent_layout_object->Style());
  }

  if (!GetLayoutObject())
    return nullptr;

  if (!parent_style)
    parent_style = style;

  if (request.pseudo_id == kPseudoIdFirstLineInherited) {
    scoped_refptr<ComputedStyle> result =
        GetDocument().EnsureStyleResolver().StyleForElement(this, parent_style,
                                                            parent_style);
    result->SetStyleType(kPseudoIdFirstLineInherited);
    return result;
  }

  return GetDocument().EnsureStyleResolver().PseudoStyleForElement(
      this, request, parent_style, parent_style);
}
// For display: contents elements, we still need to generate ::before and
// ::after, but the rest of the pseudo-elements should only be used for elements
// with an actual layout object.
bool Element::CanGeneratePseudoElement(PseudoId pseudo_id) const {
  if (HasDisplayContentsStyle())
    return pseudo_id == kPseudoIdBefore || pseudo_id == kPseudoIdAfter;
  return !!GetLayoutObject();
}

bool Element::matches(const AtomicString& selectors,
                      ExceptionState& exception_state) {
  SelectorQuery* selector_query = GetDocument().GetSelectorQueryCache().Add(
      selectors, GetDocument(), exception_state);
  if (!selector_query)
    return false;
  return selector_query->Matches(*this);
}

bool Element::matches(const AtomicString& selectors) {
  return matches(selectors, ASSERT_NO_EXCEPTION);
}

Element* Element::closest(const AtomicString& selectors,
                          ExceptionState& exception_state) {
  SelectorQuery* selector_query = GetDocument().GetSelectorQueryCache().Add(
      selectors, GetDocument(), exception_state);
  if (!selector_query)
    return nullptr;
  return selector_query->Closest(*this);
}

Element* Element::closest(const AtomicString& selectors) {
  return closest(selectors, ASSERT_NO_EXCEPTION);
}

DOMTokenList& Element::classList() {
  ElementRareData& rare_data = EnsureElementRareData();
  if (!rare_data.GetClassList()) {
    DOMTokenList* class_list = DOMTokenList::Create(*this, classAttr);
    class_list->DidUpdateAttributeValue(g_null_atom, getAttribute(classAttr));
    rare_data.SetClassList(class_list);
  }
  return *rare_data.GetClassList();
}

DOMStringMap& Element::dataset() {
  ElementRareData& rare_data = EnsureElementRareData();
  if (!rare_data.Dataset())
    rare_data.SetDataset(DatasetDOMStringMap::Create(this));
  return *rare_data.Dataset();
}

KURL Element::HrefURL() const {
  // FIXME: These all have href() or url(), but no common super class. Why
  // doesn't <link> implement URLUtils?
  if (IsHTMLAnchorElement(*this) || IsHTMLAreaElement(*this) ||
      IsHTMLLinkElement(*this))
    return GetURLAttribute(hrefAttr);
  if (auto* svg_a = ToSVGAElementOrNull(*this))
    return svg_a->LegacyHrefURL(GetDocument());
  return KURL();
}

KURL Element::GetURLAttribute(const QualifiedName& name) const {
#if DCHECK_IS_ON()
  if (GetElementData()) {
    if (const Attribute* attribute = Attributes().Find(name))
      DCHECK(IsURLAttribute(*attribute));
  }
#endif
  return GetDocument().CompleteURL(
      StripLeadingAndTrailingHTMLSpaces(getAttribute(name)));
}

void Element::GetURLAttribute(const QualifiedName& name,
                              StringOrTrustedScriptURL& result) const {
  KURL url = GetURLAttribute(name);
  result.SetString(url.GetString());
}

KURL Element::GetNonEmptyURLAttribute(const QualifiedName& name) const {
#if DCHECK_IS_ON()
  if (GetElementData()) {
    if (const Attribute* attribute = Attributes().Find(name))
      DCHECK(IsURLAttribute(*attribute));
  }
#endif
  String value = StripLeadingAndTrailingHTMLSpaces(getAttribute(name));
  if (value.IsEmpty())
    return KURL();
  return GetDocument().CompleteURL(value);
}

int Element::GetIntegralAttribute(const QualifiedName& attribute_name) const {
  int integral_value = 0;
  ParseHTMLInteger(getAttribute(attribute_name), integral_value);
  return integral_value;
}

void Element::SetIntegralAttribute(const QualifiedName& attribute_name,
                                   int value) {
  setAttribute(attribute_name, AtomicString::Number(value));
}

void Element::SetUnsignedIntegralAttribute(const QualifiedName& attribute_name,
                                           unsigned value,
                                           unsigned default_value) {
  // Range restrictions are enforced for unsigned IDL attributes that
  // reflect content attributes,
  //   http://www.whatwg.org/specs/web-apps/current-work/multipage/common-dom-interfaces.html#reflecting-content-attributes-in-idl-attributes
  if (value > 0x7fffffffu)
    value = default_value;
  setAttribute(attribute_name, AtomicString::Number(value));
}

double Element::GetFloatingPointAttribute(const QualifiedName& attribute_name,
                                          double fallback_value) const {
  return ParseToDoubleForNumberType(getAttribute(attribute_name),
                                    fallback_value);
}

void Element::SetFloatingPointAttribute(const QualifiedName& attribute_name,
                                        double value) {
  setAttribute(attribute_name, AtomicString::Number(value));
}

void Element::SetContainsFullScreenElement(bool flag) {
  SetElementFlag(ElementFlags::kContainsFullScreenElement, flag);
  // When exiting fullscreen, the element's document may not be active.
  if (flag) {
    DCHECK(GetDocument().IsActive());
    GetDocument().GetStyleEngine().EnsureUAStyleForFullscreen();
  }
  PseudoStateChanged(CSSSelector::kPseudoFullScreenAncestor);
}

// Unlike Node::parentOrShadowHostElement, this can cross frame boundaries.
static Element* NextAncestorElement(Element* element) {
  DCHECK(element);
  if (element->ParentOrShadowHostElement())
    return element->ParentOrShadowHostElement();

  Frame* frame = element->GetDocument().GetFrame();
  if (!frame || !frame->Owner())
    return nullptr;

  // Find the next LocalFrame on the ancestor chain, and return the
  // corresponding <iframe> element for the remote child if it exists.
  while (frame->Tree().Parent() && frame->Tree().Parent()->IsRemoteFrame())
    frame = frame->Tree().Parent();

  if (frame->Owner() && frame->Owner()->IsLocal())
    return ToHTMLFrameOwnerElement(frame->Owner());

  return nullptr;
}

void Element::SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(
    bool flag) {
  for (Element* element = NextAncestorElement(this); element;
       element = NextAncestorElement(element))
    element->SetContainsFullScreenElement(flag);
}

void Element::SetContainsPersistentVideo(bool value) {
  SetElementFlag(ElementFlags::kContainsPersistentVideo, value);
  PseudoStateChanged(CSSSelector::kPseudoVideoPersistentAncestor);

  // In some rare situations, when the persistent video has been removed from
  // the tree, part of the tree might still carry the flag.
  if (!value && Fullscreen::IsFullscreenElement(*this)) {
    for (Node* node = firstChild(); node;) {
      if (!node->IsElementNode() ||
          !ToElement(node)->ContainsPersistentVideo()) {
        node = node->nextSibling();
        break;
      }

      ToElement(node)->SetContainsPersistentVideo(false);
      node = node->firstChild();
    }
  }
}

void Element::SetIsInTopLayer(bool in_top_layer) {
  if (IsInTopLayer() == in_top_layer)
    return;
  SetElementFlag(ElementFlags::kIsInTopLayer, in_top_layer);

  // We must ensure a reattach occurs so the layoutObject is inserted in the
  // correct sibling order under LayoutView according to its top layer position,
  // or in its usual place if not in the top layer.
  LazyReattachIfAttached();
}

void Element::requestPointerLock() {
  if (GetDocument().GetPage())
    GetDocument().GetPage()->GetPointerLockController().RequestPointerLock(
        this);
}

SpellcheckAttributeState Element::GetSpellcheckAttributeState() const {
  const AtomicString& value = FastGetAttribute(spellcheckAttr);
  if (value == g_null_atom)
    return kSpellcheckAttributeDefault;
  if (DeprecatedEqualIgnoringCase(value, "true") ||
      DeprecatedEqualIgnoringCase(value, ""))
    return kSpellcheckAttributeTrue;
  if (DeprecatedEqualIgnoringCase(value, "false"))
    return kSpellcheckAttributeFalse;

  return kSpellcheckAttributeDefault;
}

bool Element::IsSpellCheckingEnabled() const {
  for (const Element* element = this; element;
       element = element->ParentOrShadowHostElement()) {
    switch (element->GetSpellcheckAttributeState()) {
      case kSpellcheckAttributeTrue:
        return true;
      case kSpellcheckAttributeFalse:
        return false;
      case kSpellcheckAttributeDefault:
        break;
    }
  }

  if (!GetDocument().GetPage())
    return true;

  return GetDocument().GetPage()->GetSettings().GetSpellCheckEnabledByDefault();
}

#if DCHECK_IS_ON()
bool Element::FastAttributeLookupAllowed(const QualifiedName& name) const {
  if (name == HTMLNames::styleAttr)
    return false;

  if (IsSVGElement())
    return !ToSVGElement(this)->IsAnimatableAttribute(name);

  return true;
}
#endif

#ifdef DUMP_NODE_STATISTICS
bool Element::HasNamedNodeMap() const {
  return HasRareData() && GetElementRareData()->AttributeMap();
}
#endif

inline void Element::UpdateName(const AtomicString& old_name,
                                const AtomicString& new_name) {
  if (!IsInDocumentTree())
    return;

  if (old_name == new_name)
    return;

  NamedItemType type = GetNamedItemType();
  if (type != NamedItemType::kNone)
    UpdateNamedItemRegistration(type, old_name, new_name);
}

inline void Element::UpdateId(const AtomicString& old_id,
                              const AtomicString& new_id) {
  if (!IsInTreeScope())
    return;

  if (old_id == new_id)
    return;

  UpdateId(ContainingTreeScope(), old_id, new_id);
}

inline void Element::UpdateId(TreeScope& scope,
                              const AtomicString& old_id,
                              const AtomicString& new_id) {
  DCHECK(IsInTreeScope());
  DCHECK_NE(old_id, new_id);

  if (!old_id.IsEmpty())
    scope.RemoveElementById(old_id, this);
  if (!new_id.IsEmpty())
    scope.AddElementById(new_id, this);

  NamedItemType type = GetNamedItemType();
  if (type == NamedItemType::kNameOrId ||
      type == NamedItemType::kNameOrIdWithName)
    UpdateIdNamedItemRegistration(type, old_id, new_id);
}

void Element::WillModifyAttribute(const QualifiedName& name,
                                  const AtomicString& old_value,
                                  const AtomicString& new_value) {
  if (name == HTMLNames::nameAttr) {
    UpdateName(old_value, new_value);
  }

  if (GetCustomElementState() == CustomElementState::kCustom) {
    CustomElement::EnqueueAttributeChangedCallback(this, name, old_value,
                                                   new_value);
  }

  if (old_value != new_value) {
    GetDocument().GetStyleEngine().AttributeChangedForElement(name, *this);
    if (IsUpgradedV0CustomElement()) {
      V0CustomElement::AttributeDidChange(this, name.LocalName(), old_value,
                                          new_value);
    }
  }

  if (MutationObserverInterestGroup* recipients =
          MutationObserverInterestGroup::CreateForAttributesMutation(*this,
                                                                     name))
    recipients->EnqueueMutationRecord(
        MutationRecord::CreateAttributes(this, name, old_value));

  probe::willModifyDOMAttr(this, old_value, new_value);
}

DISABLE_CFI_PERF
void Element::DidAddAttribute(const QualifiedName& name,
                              const AtomicString& value) {
  if (name == HTMLNames::idAttr)
    UpdateId(g_null_atom, value);
  AttributeChanged(AttributeModificationParams(
      name, g_null_atom, value, AttributeModificationReason::kDirectly));
  probe::didModifyDOMAttr(this, name, value);
  DispatchSubtreeModifiedEvent();
}

void Element::DidModifyAttribute(const QualifiedName& name,
                                 const AtomicString& old_value,
                                 const AtomicString& new_value) {
  if (name == HTMLNames::idAttr)
    UpdateId(old_value, new_value);
  AttributeChanged(AttributeModificationParams(
      name, old_value, new_value, AttributeModificationReason::kDirectly));
  probe::didModifyDOMAttr(this, name, new_value);
  // Do not dispatch a DOMSubtreeModified event here; see bug 81141.
}

void Element::DidRemoveAttribute(const QualifiedName& name,
                                 const AtomicString& old_value) {
  if (name == HTMLNames::idAttr)
    UpdateId(old_value, g_null_atom);
  AttributeChanged(AttributeModificationParams(
      name, old_value, g_null_atom, AttributeModificationReason::kDirectly));
  probe::didRemoveDOMAttr(this, name);
  DispatchSubtreeModifiedEvent();
}

static bool NeedsURLResolutionForInlineStyle(const Element& element,
                                             const Document& old_document,
                                             const Document& new_document) {
  if (old_document == new_document)
    return false;
  if (old_document.BaseURL() == new_document.BaseURL())
    return false;
  const CSSPropertyValueSet* style = element.InlineStyle();
  if (!style)
    return false;
  for (unsigned i = 0; i < style->PropertyCount(); ++i) {
    if (style->PropertyAt(i).Value().MayContainUrl())
      return true;
  }
  return false;
}

static void ReResolveURLsInInlineStyle(const Document& document,
                                       MutableCSSPropertyValueSet& style) {
  for (unsigned i = 0; i < style.PropertyCount(); ++i) {
    const CSSValue& value = style.PropertyAt(i).Value();
    if (value.MayContainUrl())
      value.ReResolveUrl(document);
  }
}

void Element::DidMoveToNewDocument(Document& old_document) {
  Node::DidMoveToNewDocument(old_document);

  // If the documents differ by quirks mode then they differ by case sensitivity
  // for class and id names so we need to go through the attribute change logic
  // to pick up the new casing in the ElementData.
  if (old_document.InQuirksMode() != GetDocument().InQuirksMode()) {
    // TODO(tkent): If new owner Document has a ShareableElementData matching to
    // this element's attributes, we shouldn't make UniqueElementData, and this
    // element should point to the shareable one.
    EnsureUniqueElementData();

    if (HasID())
      SetIdAttribute(GetIdAttribute());
    if (HasClass())
      setAttribute(HTMLNames::classAttr, GetClassAttribute());
  }
  // TODO(tkent): Even if Documents' modes are same, keeping
  // ShareableElementData owned by old_document isn't right.

  if (NeedsURLResolutionForInlineStyle(*this, old_document, GetDocument()))
    ReResolveURLsInInlineStyle(GetDocument(), EnsureMutableInlineStyle());
}

void Element::UpdateNamedItemRegistration(NamedItemType type,
                                          const AtomicString& old_name,
                                          const AtomicString& new_name) {
  if (!GetDocument().IsHTMLDocument())
    return;
  HTMLDocument& doc = ToHTMLDocument(GetDocument());

  if (!old_name.IsEmpty())
    doc.RemoveNamedItem(old_name);

  if (!new_name.IsEmpty())
    doc.AddNamedItem(new_name);

  if (type == NamedItemType::kNameOrIdWithName) {
    const AtomicString id = GetIdAttribute();
    if (!id.IsEmpty()) {
      if (!old_name.IsEmpty() && new_name.IsEmpty())
        doc.RemoveNamedItem(id);
      else if (old_name.IsEmpty() && !new_name.IsEmpty())
        doc.AddNamedItem(id);
    }
  }
}

void Element::UpdateIdNamedItemRegistration(NamedItemType type,
                                            const AtomicString& old_id,
                                            const AtomicString& new_id) {
  if (!GetDocument().IsHTMLDocument())
    return;

  if (type == NamedItemType::kNameOrIdWithName && GetNameAttribute().IsEmpty())
    return;

  if (!old_id.IsEmpty())
    ToHTMLDocument(GetDocument()).RemoveNamedItem(old_id);

  if (!new_id.IsEmpty())
    ToHTMLDocument(GetDocument()).AddNamedItem(new_id);
}

ScrollOffset Element::SavedLayerScrollOffset() const {
  return HasRareData() ? GetElementRareData()->SavedLayerScrollOffset()
                       : ScrollOffset();
}

void Element::SetSavedLayerScrollOffset(const ScrollOffset& size) {
  if (size.IsZero() && !HasRareData())
    return;
  EnsureElementRareData().SetSavedLayerScrollOffset(size);
}

Attr* Element::AttrIfExists(const QualifiedName& name) {
  if (AttrNodeList* attr_node_list = GetAttrNodeList()) {
    for (const auto& attr : *attr_node_list) {
      if (attr->GetQualifiedName().Matches(name))
        return attr.Get();
    }
  }
  return nullptr;
}

Attr* Element::EnsureAttr(const QualifiedName& name) {
  Attr* attr_node = AttrIfExists(name);
  if (!attr_node) {
    attr_node = Attr::Create(*this, name);
    GetTreeScope().AdoptIfNeeded(*attr_node);
    EnsureElementRareData().AddAttr(attr_node);
  }
  return attr_node;
}

void Element::DetachAttrNodeFromElementWithValue(Attr* attr_node,
                                                 const AtomicString& value) {
  DCHECK(GetAttrNodeList());
  attr_node->DetachFromElementWithValue(value);

  AttrNodeList* list = GetAttrNodeList();
  size_t index = list->Find(attr_node);
  DCHECK_NE(index, kNotFound);
  list->EraseAt(index);
  if (list->IsEmpty())
    RemoveAttrNodeList();
}

void Element::DetachAllAttrNodesFromElement() {
  AttrNodeList* list = GetAttrNodeList();
  if (!list)
    return;

  AttributeCollection attributes = GetElementData()->Attributes();
  for (const Attribute& attr : attributes) {
    if (Attr* attr_node = AttrIfExists(attr.GetName()))
      attr_node->DetachFromElementWithValue(attr.Value());
  }

  RemoveAttrNodeList();
}

Node::InsertionNotificationRequest Node::InsertedInto(
    ContainerNode* insertion_point) {
  DCHECK(!ChildNeedsStyleInvalidation());
  DCHECK(!NeedsStyleInvalidation());
  DCHECK(insertion_point->isConnected() || insertion_point->IsInShadowTree() ||
         IsContainerNode());
  if (insertion_point->isConnected()) {
    SetFlag(kIsConnectedFlag);
    insertion_point->GetDocument().IncrementNodeCount();
  }
  if (ParentOrShadowHostNode()->IsInShadowTree())
    SetFlag(kIsInShadowTreeFlag);
  if (ChildNeedsDistributionRecalc() &&
      !insertion_point->ChildNeedsDistributionRecalc())
    insertion_point->MarkAncestorsWithChildNeedsDistributionRecalc();
  return kInsertionDone;
}

void Node::RemovedFrom(ContainerNode* insertion_point) {
  DCHECK(insertion_point->isConnected() || IsContainerNode() ||
         IsInShadowTree());
  if (insertion_point->isConnected()) {
    ClearFlag(kIsConnectedFlag);
    insertion_point->GetDocument().DecrementNodeCount();
  }
  if (IsInShadowTree() && !ContainingTreeScope().RootNode().IsShadowRoot())
    ClearFlag(kIsInShadowTreeFlag);
  if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache())
    cache->Remove(this);
}

void Element::WillRecalcStyle(StyleRecalcChange) {
  DCHECK(HasCustomStyleCallbacks());
}

void Element::DidRecalcStyle(StyleRecalcChange) {
  DCHECK(HasCustomStyleCallbacks());
}

scoped_refptr<ComputedStyle> Element::CustomStyleForLayoutObject() {
  DCHECK(HasCustomStyleCallbacks());
  return OriginalStyleForLayoutObject();
}

void Element::CloneAttributesFrom(const Element& other) {
  if (HasRareData())
    DetachAllAttrNodesFromElement();

  other.SynchronizeAllAttributes();
  if (!other.element_data_) {
    element_data_.Clear();
    return;
  }

  const AtomicString& old_id = GetIdAttribute();
  const AtomicString& new_id = other.GetIdAttribute();

  if (!old_id.IsNull() || !new_id.IsNull())
    UpdateId(old_id, new_id);

  const AtomicString& old_name = GetNameAttribute();
  const AtomicString& new_name = other.GetNameAttribute();

  if (!old_name.IsNull() || !new_name.IsNull())
    UpdateName(old_name, new_name);

  // Quirks mode makes class and id not case sensitive. We can't share the
  // ElementData if the idForStyleResolution and the className need different
  // casing.
  bool owner_documents_have_different_case_sensitivity = false;
  if (other.HasClass() || other.HasID())
    owner_documents_have_different_case_sensitivity =
        other.GetDocument().InQuirksMode() != GetDocument().InQuirksMode();

  // If 'other' has a mutable ElementData, convert it to an immutable one so we
  // can share it between both elements.
  // We can only do this if there are no presentation attributes and sharing the
  // data won't result in different case sensitivity of class or id.
  if (other.element_data_->IsUnique() &&
      !owner_documents_have_different_case_sensitivity &&
      !other.element_data_->PresentationAttributeStyle())
    const_cast<Element&>(other).element_data_ =
        ToUniqueElementData(other.element_data_)->MakeShareableCopy();

  if (!other.element_data_->IsUnique() &&
      !owner_documents_have_different_case_sensitivity &&
      !NeedsURLResolutionForInlineStyle(other, other.GetDocument(),
                                        GetDocument()))
    element_data_ = other.element_data_;
  else
    element_data_ = other.element_data_->MakeUniqueCopy();

  for (const Attribute& attr : element_data_->Attributes()) {
    AttributeChanged(
        AttributeModificationParams(attr.GetName(), g_null_atom, attr.Value(),
                                    AttributeModificationReason::kByCloning));
  }

  if (other.nonce() != g_null_atom)
    setNonce(other.nonce());
}

void Element::CreateUniqueElementData() {
  if (!element_data_) {
    element_data_ = UniqueElementData::Create();
  } else {
    DCHECK(!element_data_->IsUnique());
    element_data_ = ToShareableElementData(element_data_)->MakeUniqueCopy();
  }
}

void Element::SynchronizeStyleAttributeInternal() const {
  DCHECK(IsStyledElement());
  DCHECK(GetElementData());
  DCHECK(GetElementData()->style_attribute_is_dirty_);
  GetElementData()->style_attribute_is_dirty_ = false;
  const CSSPropertyValueSet* inline_style = InlineStyle();
  const_cast<Element*>(this)->SetSynchronizedLazyAttribute(
      styleAttr,
      inline_style ? AtomicString(inline_style->AsText()) : g_empty_atom);
}

CSSStyleDeclaration* Element::style() {
  if (!IsStyledElement())
    return nullptr;
  return &EnsureElementRareData().EnsureInlineCSSStyleDeclaration(this);
}

StylePropertyMap* Element::attributeStyleMap() {
  if (!IsStyledElement())
    return nullptr;
  return &EnsureElementRareData().EnsureInlineStylePropertyMap(this);
}

StylePropertyMapReadOnly* Element::ComputedStyleMap() {
  return GetDocument().ComputedStyleMap(this);
}

MutableCSSPropertyValueSet& Element::EnsureMutableInlineStyle() {
  DCHECK(IsStyledElement());
  Member<CSSPropertyValueSet>& inline_style =
      EnsureUniqueElementData().inline_style_;
  if (!inline_style) {
    CSSParserMode mode = (!IsHTMLElement() || GetDocument().InQuirksMode())
                             ? kHTMLQuirksMode
                             : kHTMLStandardMode;
    inline_style = MutableCSSPropertyValueSet::Create(mode);
  } else if (!inline_style->IsMutable()) {
    inline_style = inline_style->MutableCopy();
  }
  return *ToMutableCSSPropertyValueSet(inline_style);
}

void Element::ClearMutableInlineStyleIfEmpty() {
  if (EnsureMutableInlineStyle().IsEmpty()) {
    EnsureUniqueElementData().inline_style_.Clear();
  }
}

inline void Element::SetInlineStyleFromString(
    const AtomicString& new_style_string) {
  DCHECK(IsStyledElement());
  Member<CSSPropertyValueSet>& inline_style = GetElementData()->inline_style_;

  // Avoid redundant work if we're using shared attribute data with already
  // parsed inline style.
  if (inline_style && !GetElementData()->IsUnique())
    return;

  // We reconstruct the property set instead of mutating if there is no CSSOM
  // wrapper.  This makes wrapperless property sets immutable and so cacheable.
  if (inline_style && !inline_style->IsMutable())
    inline_style.Clear();

  if (!inline_style) {
    inline_style =
        CSSParser::ParseInlineStyleDeclaration(new_style_string, this);
  } else {
    DCHECK(inline_style->IsMutable());
    static_cast<MutableCSSPropertyValueSet*>(inline_style.Get())
        ->ParseDeclarationList(new_style_string,
                               GetDocument().GetSecureContextMode(),
                               GetDocument().ElementSheet().Contents());
  }
}

void Element::StyleAttributeChanged(
    const AtomicString& new_style_string,
    AttributeModificationReason modification_reason) {
  DCHECK(IsStyledElement());
  WTF::OrdinalNumber start_line_number = WTF::OrdinalNumber::BeforeFirst();
  if (GetDocument().GetScriptableDocumentParser() &&
      !GetDocument().IsInDocumentWrite())
    start_line_number =
        GetDocument().GetScriptableDocumentParser()->LineNumber();

  if (new_style_string.IsNull()) {
    EnsureUniqueElementData().inline_style_.Clear();
  } else if (modification_reason == AttributeModificationReason::kByCloning ||
             ContentSecurityPolicy::ShouldBypassMainWorld(&GetDocument()) ||
             (ContainingShadowRoot() &&
              ContainingShadowRoot()->IsUserAgent()) ||
             GetDocument().GetContentSecurityPolicy()->AllowInlineStyle(
                 this, GetDocument().Url(), String(), start_line_number,
                 new_style_string, ContentSecurityPolicy::InlineType::kBlock)) {
    SetInlineStyleFromString(new_style_string);
  }

  GetElementData()->style_attribute_is_dirty_ = false;

  SetNeedsStyleRecalc(kLocalStyleChange,
                      StyleChangeReasonForTracing::Create(
                          StyleChangeReason::kStyleSheetChange));
  probe::didInvalidateStyleAttr(this);
}

void Element::InlineStyleChanged() {
  DCHECK(IsStyledElement());
  SetNeedsStyleRecalc(kLocalStyleChange, StyleChangeReasonForTracing::Create(
                                             StyleChangeReason::kInline));
  DCHECK(GetElementData());
  GetElementData()->style_attribute_is_dirty_ = true;
  probe::didInvalidateStyleAttr(this);

  if (MutationObserverInterestGroup* recipients =
          MutationObserverInterestGroup::CreateForAttributesMutation(
              *this, styleAttr)) {
    // We don't use getAttribute() here to get a style attribute value
    // before the change.
    AtomicString old_value;
    if (const Attribute* attribute =
            GetElementData()->Attributes().Find(styleAttr))
      old_value = attribute->Value();
    recipients->EnqueueMutationRecord(
        MutationRecord::CreateAttributes(this, styleAttr, old_value));
    // Need to synchronize every time so that following MutationRecords will
    // have correct oldValues.
    SynchronizeAttribute(styleAttr);
  }
}

void Element::SetInlineStyleProperty(CSSPropertyID property_id,
                                     CSSValueID identifier,
                                     bool important) {
  SetInlineStyleProperty(property_id, *CSSIdentifierValue::Create(identifier),
                         important);
}

void Element::SetInlineStyleProperty(CSSPropertyID property_id,
                                     double value,
                                     CSSPrimitiveValue::UnitType unit,
                                     bool important) {
  SetInlineStyleProperty(property_id, *CSSPrimitiveValue::Create(value, unit),
                         important);
}

void Element::SetInlineStyleProperty(CSSPropertyID property_id,
                                     const CSSValue& value,
                                     bool important) {
  DCHECK(IsStyledElement());
  EnsureMutableInlineStyle().SetProperty(property_id, value, important);
  InlineStyleChanged();
}

bool Element::SetInlineStyleProperty(CSSPropertyID property_id,
                                     const String& value,
                                     bool important) {
  DCHECK(IsStyledElement());
  bool did_change = EnsureMutableInlineStyle()
                        .SetProperty(property_id, value, important,
                                     GetDocument().GetSecureContextMode(),
                                     GetDocument().ElementSheet().Contents())
                        .did_change;
  if (did_change)
    InlineStyleChanged();
  return did_change;
}

bool Element::RemoveInlineStyleProperty(CSSPropertyID property_id) {
  DCHECK(IsStyledElement());
  if (!InlineStyle())
    return false;
  bool did_change = EnsureMutableInlineStyle().RemoveProperty(property_id);
  if (did_change)
    InlineStyleChanged();
  return did_change;
}

bool Element::RemoveInlineStyleProperty(const AtomicString& property_name) {
  DCHECK(IsStyledElement());
  if (!InlineStyle())
    return false;
  bool did_change = EnsureMutableInlineStyle().RemoveProperty(property_name);
  if (did_change)
    InlineStyleChanged();
  return did_change;
}

void Element::RemoveAllInlineStyleProperties() {
  DCHECK(IsStyledElement());
  if (!InlineStyle())
    return;
  EnsureMutableInlineStyle().Clear();
  InlineStyleChanged();
}

void Element::UpdatePresentationAttributeStyle() {
  SynchronizeAllAttributes();
  // ShareableElementData doesn't store presentation attribute style, so make
  // sure we have a UniqueElementData.
  UniqueElementData& element_data = EnsureUniqueElementData();
  element_data.presentation_attribute_style_is_dirty_ = false;
  element_data.presentation_attribute_style_ =
      ComputePresentationAttributeStyle(*this);
}

void Element::AddPropertyToPresentationAttributeStyle(
    MutableCSSPropertyValueSet* style,
    CSSPropertyID property_id,
    CSSValueID identifier) {
  DCHECK(IsStyledElement());
  style->SetProperty(property_id, *CSSIdentifierValue::Create(identifier));
}

void Element::AddPropertyToPresentationAttributeStyle(
    MutableCSSPropertyValueSet* style,
    CSSPropertyID property_id,
    double value,
    CSSPrimitiveValue::UnitType unit) {
  DCHECK(IsStyledElement());
  style->SetProperty(property_id, *CSSPrimitiveValue::Create(value, unit));
}

void Element::AddPropertyToPresentationAttributeStyle(
    MutableCSSPropertyValueSet* style,
    CSSPropertyID property_id,
    const String& value) {
  DCHECK(IsStyledElement());
  Document& document = GetDocument();
  style->SetProperty(property_id, value, false, document.GetSecureContextMode(),
                     document.ElementSheet().Contents());
}

void Element::AddPropertyToPresentationAttributeStyle(
    MutableCSSPropertyValueSet* style,
    CSSPropertyID property_id,
    const CSSValue& value) {
  DCHECK(IsStyledElement());
  style->SetProperty(property_id, value);
}

void Element::LogAddElementIfIsolatedWorldAndInDocument(
    const char element[],
    const QualifiedName& attr1) {
  if (!isConnected())
    return;
  V8DOMActivityLogger* activity_logger =
      V8DOMActivityLogger::CurrentActivityLoggerIfIsolatedWorldForMainThread();
  if (!activity_logger)
    return;
  Vector<String, 2> argv;
  argv.push_back(element);
  argv.push_back(FastGetAttribute(attr1));
  activity_logger->LogEvent("blinkAddElement", argv.size(), argv.data());
}

void Element::LogAddElementIfIsolatedWorldAndInDocument(
    const char element[],
    const QualifiedName& attr1,
    const QualifiedName& attr2) {
  if (!isConnected())
    return;
  V8DOMActivityLogger* activity_logger =
      V8DOMActivityLogger::CurrentActivityLoggerIfIsolatedWorldForMainThread();
  if (!activity_logger)
    return;
  Vector<String, 3> argv;
  argv.push_back(element);
  argv.push_back(FastGetAttribute(attr1));
  argv.push_back(FastGetAttribute(attr2));
  activity_logger->LogEvent("blinkAddElement", argv.size(), argv.data());
}

void Element::LogAddElementIfIsolatedWorldAndInDocument(
    const char element[],
    const QualifiedName& attr1,
    const QualifiedName& attr2,
    const QualifiedName& attr3) {
  if (!isConnected())
    return;
  V8DOMActivityLogger* activity_logger =
      V8DOMActivityLogger::CurrentActivityLoggerIfIsolatedWorldForMainThread();
  if (!activity_logger)
    return;
  Vector<String, 4> argv;
  argv.push_back(element);
  argv.push_back(FastGetAttribute(attr1));
  argv.push_back(FastGetAttribute(attr2));
  argv.push_back(FastGetAttribute(attr3));
  activity_logger->LogEvent("blinkAddElement", argv.size(), argv.data());
}

void Element::LogUpdateAttributeIfIsolatedWorldAndInDocument(
    const char element[],
    const AttributeModificationParams& params) {
  if (!isConnected())
    return;
  V8DOMActivityLogger* activity_logger =
      V8DOMActivityLogger::CurrentActivityLoggerIfIsolatedWorldForMainThread();
  if (!activity_logger)
    return;
  Vector<String, 4> argv;
  argv.push_back(element);
  argv.push_back(params.name.ToString());
  argv.push_back(params.old_value);
  argv.push_back(params.new_value);
  activity_logger->LogEvent("blinkSetAttribute", argv.size(), argv.data());
}

void Element::Trace(blink::Visitor* visitor) {
  if (HasRareData())
    visitor->Trace(GetElementRareData());
  visitor->Trace(element_data_);
  ContainerNode::Trace(visitor);
}

void Element::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  if (HasRareData()) {
    visitor->TraceWrappersWithManualWriteBarrier(GetElementRareData());
  }
  ContainerNode::TraceWrappers(visitor);
}

bool Element::HasPartName() const {
  if (!RuntimeEnabledFeatures::CSSPartPseudoElementEnabled())
    return false;
  if (HasRareData()) {
    if (auto* part_names = GetElementRareData()->PartNames()) {
      return part_names->size() > 0;
    }
  }
  return false;
}

const SpaceSplitString* Element::PartNames() const {
  return RuntimeEnabledFeatures::CSSPartPseudoElementEnabled() && HasRareData()
             ? GetElementRareData()->PartNames()
             : nullptr;
}

bool Element::HasPartNamesMap() const {
  const NamesMap* names_map = PartNamesMap();
  return names_map && names_map->size() > 0;
}

const NamesMap* Element::PartNamesMap() const {
  return RuntimeEnabledFeatures::CSSPartPseudoElementEnabled() && HasRareData()
             ? GetElementRareData()->PartNamesMap()
             : nullptr;
}

}  // namespace blink
