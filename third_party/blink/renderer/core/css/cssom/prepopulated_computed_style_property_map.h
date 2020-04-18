// Copyright 2018 the Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_PREPOPULATED_COMPUTED_STYLE_PROPERTY_MAP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_PREPOPULATED_COMPUTED_STYLE_PROPERTY_MAP_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/css/css_property_id_templates.h"
#include "third_party/blink/renderer/core/css/cssom/style_property_map_read_only.h"

namespace blink {

// This class has the same behaviour as the ComputedStylePropertyMap, except it
// only contains the properties given to the constructor.
//
// It is to be used with the Houdini APIs (css-paint-api, css-layout-api) which
// require style maps with a subset of properties.
//
// It will pre-populate internal maps (property->CSSValue), as the above APIs
// have a high probability of querying the values inside the map, however as a
// result when the ComputedStyle changes UpdateStyle needs to be called to
// re-populate the internal maps.
class CORE_EXPORT PrepopulatedComputedStylePropertyMap
    : public StylePropertyMapReadOnly {
 public:
  // NOTE: styled_node may be null, in the case where this map is for an
  // anonymous box.
  PrepopulatedComputedStylePropertyMap(
      const Document&,
      const ComputedStyle&,
      Node* styled_node,
      const Vector<CSSPropertyID>& native_properties,
      const Vector<AtomicString>& custom_properties);

  // Updates the values of the properties based on the new computed style.
  void UpdateStyle(const Document&, const ComputedStyle&);

  unsigned size() override;
  void Trace(blink::Visitor*) override;

 protected:
  const CSSValue* GetProperty(CSSPropertyID) override;
  const CSSValue* GetCustomProperty(AtomicString) override;
  void ForEachProperty(const IterationCallback&) override;

  String SerializationForShorthand(const CSSProperty&) override;

 private:
  void UpdateNativeProperty(const ComputedStyle&, CSSPropertyID);
  void UpdateCustomProperty(const Document&,
                            const ComputedStyle&,
                            const AtomicString& property_name);

  Member<Node> styled_node_;
  HeapHashMap<CSSPropertyID, Member<const CSSValue>> native_values_;
  HeapHashMap<AtomicString, Member<const CSSValue>> custom_values_;

  DISALLOW_COPY_AND_ASSIGN(PrepopulatedComputedStylePropertyMap);
};

}  // namespace blink

#endif
