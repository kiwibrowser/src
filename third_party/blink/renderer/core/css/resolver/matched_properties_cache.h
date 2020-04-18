/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.
 * All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_MATCHED_PROPERTIES_CACHE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_RESOLVER_MATCHED_PROPERTIES_CACHE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/resolver/match_result.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class ComputedStyle;
class StyleResolverState;

class CachedMatchedProperties final
    : public GarbageCollectedFinalized<CachedMatchedProperties> {
 public:
  HeapVector<MatchedProperties> matched_properties;
  scoped_refptr<ComputedStyle> computed_style;
  scoped_refptr<ComputedStyle> parent_computed_style;

  void Set(const ComputedStyle&,
           const ComputedStyle& parent_style,
           const MatchedPropertiesVector&);
  void Clear();
  void Trace(blink::Visitor* visitor) { visitor->Trace(matched_properties); }
};

// Specialize the HashTraits for CachedMatchedProperties to check for dead
// entries in the MatchedPropertiesCache.
struct CachedMatchedPropertiesHashTraits
    : HashTraits<Member<CachedMatchedProperties>> {
  static const WTF::WeakHandlingFlag kWeakHandlingFlag = WTF::kWeakHandling;

  static bool IsAlive(Member<CachedMatchedProperties>& cached_properties) {
    // Semantics see |CachedMatchedPropertiesHashTraits::TraceInCollection|.
    if (cached_properties) {
      for (const auto& matched_properties :
           cached_properties->matched_properties) {
        if (!ThreadHeap::IsHeapObjectAlive(matched_properties.properties)) {
          return false;
        }
      }
    }
    return true;
  }

  template <typename VisitorDispatcher>
  static bool TraceInCollection(
      VisitorDispatcher visitor,
      Member<CachedMatchedProperties>& cached_properties,
      WTF::WeakHandlingFlag weakness) {
    // Only honor the cache's weakness semantics if the collection is traced
    // with |kWeakPointersActWeak|. Otherwise just trace the cachedProperties
    // strongly, i.e., call trace on it.
    if (cached_properties && weakness == WTF::kWeakHandling) {
      // A given cache entry is only kept alive if none of the MatchedProperties
      // in the CachedMatchedProperties value contain a dead "properties" field.
      // If there is a dead field the entire cache entry is removed.
      for (const auto& matched_properties :
           cached_properties->matched_properties) {
        if (!ThreadHeap::IsHeapObjectAlive(matched_properties.properties)) {
          // For now report the cache entry as dead. This might not
          // be the final result if in a subsequent call for this entry,
          // the "properties" field has been marked via another path.
          return true;
        }
      }
    }
    // At this point none of the entries in the matchedProperties vector
    // had a dead "properties" field so trace CachedMatchedProperties strongly.
    visitor->Trace(cached_properties);
    return false;
  }
};

class MatchedPropertiesCache {
  DISALLOW_NEW();

 public:
  MatchedPropertiesCache();
  ~MatchedPropertiesCache() { DCHECK(cache_.IsEmpty()); }

  const CachedMatchedProperties* Find(unsigned hash,
                                      const StyleResolverState&,
                                      const MatchedPropertiesVector&);
  void Add(const ComputedStyle&,
           const ComputedStyle& parent_style,
           unsigned hash,
           const MatchedPropertiesVector&);

  void Clear();
  void ClearViewportDependent();

  static bool IsCacheable(const StyleResolverState&);
  static bool IsStyleCacheable(const ComputedStyle&);

  void Trace(blink::Visitor*);

 private:
  using Cache = HeapHashMap<unsigned,
                            Member<CachedMatchedProperties>,
                            DefaultHash<unsigned>::Hash,
                            HashTraits<unsigned>,
                            CachedMatchedPropertiesHashTraits>;
  Cache cache_;
  DISALLOW_COPY_AND_ASSIGN(MatchedPropertiesCache);
};

}  // namespace blink

#endif
