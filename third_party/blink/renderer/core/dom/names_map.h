// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NAMES_MAP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NAMES_MAP_H_

#include <memory>

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/space_split_string.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Parses and stores mappings from part name to ordered set of part names as in
// http://drafts.csswg.org/css-shadow-parts/ (with modifications).
// TODO(crbug/805271): Deduplicate identical maps as SpaceSplitString does so
// that elements with identical partmap attributes share instances.
class CORE_EXPORT NamesMap {
 public:
  NamesMap() = default;
  explicit NamesMap(const AtomicString& string);

  // Clears any existing mapping, parses the string and sets the mapping from
  // that.  This implements a modified version from the spec, where the key is
  // first and the value is second and "=>" is not used to separate key and
  // value. It also allows an ident token on its own as a short-hand for
  // forwarding with the same name. So "a b, a c, d e, f" becomes
  //
  // a: {b, c}
  // d: {e}
  // f: {f}
  void Set(const AtomicString&);
  void Clear() { data_.clear(); };
  // Inserts value into the ordered set under key.
  void Add(const AtomicString& key, const AtomicString& value);
  base::Optional<SpaceSplitString> Get(const AtomicString& key) const;

  size_t size() const { return data_.size(); }

 private:
  template <typename CharacterType>
  void Set(const AtomicString&, const CharacterType*);

  HashMap<AtomicString, base::Optional<SpaceSplitString>> data_;

  DISALLOW_COPY_AND_ASSIGN(NamesMap);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NAMES_MAP_H_
