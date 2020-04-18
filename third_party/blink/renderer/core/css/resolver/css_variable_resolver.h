// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_CSS_VARIABLE_RESOLVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_CSS_VARIABLE_RESOLVER_H_

#include "third_party/blink/renderer/core/css/parser/css_parser_token.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"

namespace blink {

class CSSCustomPropertyDeclaration;
class CSSParserTokenRange;
class CSSPendingSubstitutionValue;
class CSSVariableData;
class CSSVariableReferenceValue;
class PropertyRegistry;
class StyleResolverState;
class StyleInheritedVariables;
class StyleNonInheritedVariables;

class CSSVariableResolver {
  STACK_ALLOCATED();

 public:
  CSSVariableResolver(const StyleResolverState&);

  scoped_refptr<CSSVariableData> ResolveCustomPropertyAnimationKeyframe(
      const CSSCustomPropertyDeclaration& keyframe,
      bool& cycle_detected);

  void ResolveVariableDefinitions();

  // Shorthand properties are not supported.
  const CSSValue* ResolveVariableReferences(CSSPropertyID,
                                            const CSSValue&,
                                            bool disallow_animation_tainted);

  void ComputeRegisteredVariables();

 private:
  const CSSValue* ResolvePendingSubstitutions(
      CSSPropertyID,
      const CSSPendingSubstitutionValue&,
      bool disallow_animation_tainted);
  const CSSValue* ResolveVariableReferences(CSSPropertyID,
                                            const CSSVariableReferenceValue&,
                                            bool disallow_animation_tainted);

  // These return false if we encounter a reference to an invalid variable with
  // no fallback.

  // Resolves a range which may contain var() references.
  bool ResolveTokenRange(CSSParserTokenRange,
                         bool disallow_animation_tainted,
                         Vector<CSSParserToken>& result,
                         Vector<String>& result_backing_strings,
                         bool& result_is_animation_tainted);
  // Resolves the fallback (if present) of a var() reference, starting from the
  // comma.
  bool ResolveFallback(CSSParserTokenRange,
                       bool disallow_animation_tainted,
                       Vector<CSSParserToken>& result,
                       Vector<String>& result_backing_strings,
                       bool& result_is_animation_tainted);
  // Resolves the contents of a var() reference.
  bool ResolveVariableReference(CSSParserTokenRange,
                                bool disallow_animation_tainted,
                                Vector<CSSParserToken>& result,
                                Vector<String>& result_backing_strings,
                                bool& result_is_animation_tainted);

  // These return null if the custom property is invalid.

  // Returns the CSSVariableData for a custom property, resolving and storing it
  // if necessary.
  CSSVariableData* ValueForCustomProperty(AtomicString name);
  // Resolves the CSSVariableData from a custom property declaration.
  scoped_refptr<CSSVariableData> ResolveCustomProperty(AtomicString name,
                                                       const CSSVariableData&,
                                                       bool& cycle_detected);

  const StyleResolverState& state_;
  StyleInheritedVariables* inherited_variables_;
  StyleNonInheritedVariables* non_inherited_variables_;
  Member<const PropertyRegistry> registry_;
  HashSet<AtomicString> variables_seen_;
  // Resolution doesn't finish when a cycle is detected. Fallbacks still
  // need to be tracked for additional cycles, and invalidation only
  // applies back to cycle starts.
  HashSet<AtomicString> cycle_start_points_;
};

}  // namespace blink

#endif  // CSSVariableResolver
