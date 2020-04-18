// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_CSS_PROPERTY_PRIORITY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_CSS_PROPERTY_PRIORITY_H_

#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

// The values of high priority properties affect the values of low priority
// properties. For example, the value of the high priority property 'font-size'
// decides the pixel value of low priority properties with 'em' units.

// TODO(sashab): Generate the methods in this file.

enum CSSPropertyPriority {
  kResolveVariables = 0,
  kAnimationPropertyPriority,
  kHighPropertyPriority,
  kLowPropertyPriority,
  kPropertyPriorityCount,
};

template <CSSPropertyPriority priority>
class CSSPropertyPriorityData {
  STATIC_ONLY(CSSPropertyPriorityData);

 public:
  static inline CSSPropertyID First();
  static inline CSSPropertyID Last();
  static inline bool PropertyHasPriority(CSSPropertyID prop) {
    return First() <= prop && prop <= Last();
  }
};

template <>
inline CSSPropertyID CSSPropertyPriorityData<kResolveVariables>::First() {
  static_assert(
      CSSPropertyVariable == firstCSSProperty - 1,
      "CSSPropertyVariable should be directly before the first CSS property.");
  return CSSPropertyVariable;
}

template <>
inline CSSPropertyID CSSPropertyPriorityData<kResolveVariables>::Last() {
  return CSSPropertyVariable;
}

template <>
inline CSSPropertyID
CSSPropertyPriorityData<kAnimationPropertyPriority>::First() {
  static_assert(CSSPropertyAnimationDelay == firstCSSProperty,
                "CSSPropertyAnimationDelay should be the first animation "
                "priority property");
  return CSSPropertyAnimationDelay;
}

template <>
inline CSSPropertyID
CSSPropertyPriorityData<kAnimationPropertyPriority>::Last() {
  static_assert(
      CSSPropertyTransitionTimingFunction == CSSPropertyAnimationDelay + 11,
      "CSSPropertyTransitionTimingFunction should be the end of the high "
      "priority property range");
  static_assert(
      CSSPropertyColor == CSSPropertyTransitionTimingFunction + 1,
      "CSSPropertyTransitionTimingFunction should be immediately before "
      "CSSPropertyColor");
  return CSSPropertyTransitionTimingFunction;
}

template <>
inline CSSPropertyID CSSPropertyPriorityData<kHighPropertyPriority>::First() {
  static_assert(CSSPropertyColor == CSSPropertyTransitionTimingFunction + 1,
                "CSSPropertyColor should be the first high priority property");
  return CSSPropertyColor;
}

template <>
inline CSSPropertyID CSSPropertyPriorityData<kHighPropertyPriority>::Last() {
  static_assert(
      CSSPropertyZoom == CSSPropertyColor + 22,
      "CSSPropertyZoom should be the end of the high priority property range");
  static_assert(CSSPropertyWritingMode == CSSPropertyZoom - 1,
                "CSSPropertyWritingMode should be immediately before "
                "CSSPropertyZoom");
  return CSSPropertyZoom;
}

template <>
inline CSSPropertyID CSSPropertyPriorityData<kLowPropertyPriority>::First() {
  static_assert(
      CSSPropertyAlignContent == CSSPropertyZoom + 1,
      "CSSPropertyAlignContent should be the first low priority property");
  return CSSPropertyAlignContent;
}

template <>
inline CSSPropertyID CSSPropertyPriorityData<kLowPropertyPriority>::Last() {
  return static_cast<CSSPropertyID>(lastCSSProperty);
}

inline CSSPropertyPriority PriorityForProperty(CSSPropertyID property) {
  if (CSSPropertyPriorityData<kLowPropertyPriority>::PropertyHasPriority(
          property)) {
    return kLowPropertyPriority;
  }
  if (CSSPropertyPriorityData<kHighPropertyPriority>::PropertyHasPriority(
          property)) {
    return kHighPropertyPriority;
  }
  if (CSSPropertyPriorityData<kAnimationPropertyPriority>::PropertyHasPriority(
          property)) {
    return kAnimationPropertyPriority;
  }
  DCHECK(CSSPropertyPriorityData<kResolveVariables>::PropertyHasPriority(
      property));
  return kResolveVariables;
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_CSS_PROPERTY_PRIORITY_H_
