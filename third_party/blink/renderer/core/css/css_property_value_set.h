/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PROPERTY_VALUE_SET_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PROPERTY_VALUE_SET_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/css_primitive_value.h"
#include "third_party/blink/renderer/core/css/css_property_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_mode.h"
#include "third_party/blink/renderer/core/css/property_set_css_style_declaration.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class CSSStyleDeclaration;
class ImmutableCSSPropertyValueSet;
class MutableCSSPropertyValueSet;
class PropertyRegistry;
class StyleSheetContents;
enum class SecureContextMode;

class CORE_EXPORT CSSPropertyValueSet
    : public GarbageCollectedFinalized<CSSPropertyValueSet> {
  friend class PropertyReference;

 public:
  void FinalizeGarbageCollectedObject();

  class PropertyReference {
    STACK_ALLOCATED();

   public:
    PropertyReference(const CSSPropertyValueSet& property_set, unsigned index)
        : property_set_(&property_set), index_(index) {}

    CSSPropertyID Id() const {
      return static_cast<CSSPropertyID>(
          PropertyMetadata().Property().PropertyID());
    }
    const CSSProperty& Property() const {
      return PropertyMetadata().Property();
    }
    CSSPropertyID ShorthandID() const {
      return PropertyMetadata().ShorthandID();
    }

    bool IsImportant() const { return PropertyMetadata().important_; }
    bool IsInherited() const { return PropertyMetadata().inherited_; }
    bool IsImplicit() const { return PropertyMetadata().implicit_; }

    const CSSValue& Value() const { return PropertyValue(); }

    const CSSPropertyValueMetadata& PropertyMetadata() const;

   private:
    const CSSValue& PropertyValue() const;

    Member<const CSSPropertyValueSet> property_set_;
    unsigned index_;
  };

  unsigned PropertyCount() const;
  bool IsEmpty() const;
  PropertyReference PropertyAt(unsigned index) const {
    return PropertyReference(*this, index);
  }

  template <typename T>  // CSSPropertyID or AtomicString
  int FindPropertyIndex(T property) const;

  bool HasProperty(CSSPropertyID property) const {
    return FindPropertyIndex(property) != -1;
  }

  template <typename T>  // CSSPropertyID or AtomicString
  const CSSValue* GetPropertyCSSValue(T property) const;

  template <typename T>  // CSSPropertyID or AtomicString
  String GetPropertyValue(T property) const;

  template <typename T>  // CSSPropertyID or AtomicString
  bool PropertyIsImportant(T property) const;

  bool ShorthandIsImportant(CSSPropertyID) const;
  bool ShorthandIsImportant(AtomicString custom_property_name) const;

  CSSPropertyID GetPropertyShorthand(CSSPropertyID) const;
  bool IsPropertyImplicit(CSSPropertyID) const;

  CSSParserMode CssParserMode() const {
    return static_cast<CSSParserMode>(css_parser_mode_);
  }

  MutableCSSPropertyValueSet* MutableCopy() const;
  ImmutableCSSPropertyValueSet* ImmutableCopyIfNeeded() const;

  MutableCSSPropertyValueSet* CopyPropertiesInSet(
      const Vector<const CSSProperty*>&) const;

  String AsText() const;

  bool IsMutable() const { return is_mutable_; }

  bool HasFailedOrCanceledSubresources() const;

  static unsigned AverageSizeInBytes();

#ifndef NDEBUG
  void ShowStyle();
#endif

  bool PropertyMatches(CSSPropertyID, const CSSValue&) const;

  void Trace(blink::Visitor*);
  void TraceAfterDispatch(blink::Visitor* visitor) {}

 protected:
  enum { kMaxArraySize = (1 << 28) - 1 };

  CSSPropertyValueSet(CSSParserMode css_parser_mode)
      : css_parser_mode_(css_parser_mode), is_mutable_(true), array_size_(0) {}

  CSSPropertyValueSet(CSSParserMode css_parser_mode,
                      unsigned immutable_array_size)
      : css_parser_mode_(css_parser_mode), is_mutable_(false) {
    // Avoid min()/max() from std here in the header, because that would require
    // inclusion of <algorithm>, which is slow to compile.
    if (immutable_array_size < unsigned(kMaxArraySize))
      array_size_ = immutable_array_size;
    else
      array_size_ = unsigned(kMaxArraySize);
  }

  unsigned css_parser_mode_ : 3;
  mutable unsigned is_mutable_ : 1;
  unsigned array_size_ : 28;

  friend class PropertySetCSSStyleDeclaration;
  DISALLOW_COPY_AND_ASSIGN(CSSPropertyValueSet);
};

// Used for lazily parsing properties.
class CSSLazyPropertyParser
    : public GarbageCollectedFinalized<CSSLazyPropertyParser> {
 public:
  CSSLazyPropertyParser() = default;
  virtual ~CSSLazyPropertyParser() = default;
  virtual CSSPropertyValueSet* ParseProperties() = 0;
  virtual void Trace(blink::Visitor*);
  DISALLOW_COPY_AND_ASSIGN(CSSLazyPropertyParser);
};

class CORE_EXPORT ImmutableCSSPropertyValueSet : public CSSPropertyValueSet {
 public:
  ~ImmutableCSSPropertyValueSet();
  static ImmutableCSSPropertyValueSet*
  Create(const CSSPropertyValue* properties, unsigned count, CSSParserMode);

  unsigned PropertyCount() const { return array_size_; }

  const Member<const CSSValue>* ValueArray() const;
  const CSSPropertyValueMetadata* MetadataArray() const;

  template <typename T>  // CSSPropertyID or AtomicString
  int FindPropertyIndex(T property) const;

  void TraceAfterDispatch(blink::Visitor*);

  void* operator new(std::size_t, void* location) { return location; }

  void* storage_;

 private:
  ImmutableCSSPropertyValueSet(const CSSPropertyValue*,
                               unsigned count,
                               CSSParserMode);
};

inline const Member<const CSSValue>* ImmutableCSSPropertyValueSet::ValueArray()
    const {
  return reinterpret_cast<const Member<const CSSValue>*>(
      const_cast<const void**>(&(this->storage_)));
}

inline const CSSPropertyValueMetadata*
ImmutableCSSPropertyValueSet::MetadataArray() const {
  return reinterpret_cast<const CSSPropertyValueMetadata*>(
      &reinterpret_cast<const char*>(
          &(this->storage_))[array_size_ * sizeof(Member<CSSValue>)]);
}

DEFINE_TYPE_CASTS(ImmutableCSSPropertyValueSet,
                  CSSPropertyValueSet,
                  set,
                  !set->IsMutable(),
                  !set.IsMutable());

class CORE_EXPORT MutableCSSPropertyValueSet : public CSSPropertyValueSet {
 public:
  ~MutableCSSPropertyValueSet() = default;
  static MutableCSSPropertyValueSet* Create(CSSParserMode);
  static MutableCSSPropertyValueSet* Create(const CSSPropertyValue* properties,
                                            unsigned count);

  unsigned PropertyCount() const { return property_vector_.size(); }

  // Returns whether this style set was changed.
  bool AddParsedProperties(const HeapVector<CSSPropertyValue, 256>&);
  bool AddRespectingCascade(const CSSPropertyValue&);

  struct SetResult {
    bool did_parse;
    bool did_change;
  };
  // These expand shorthand properties into multiple properties.
  SetResult SetProperty(CSSPropertyID unresolved_property,
                        const String& value,
                        bool important,
                        SecureContextMode,
                        StyleSheetContents* context_style_sheet = nullptr);
  SetResult SetProperty(const AtomicString& custom_property_name,
                        const PropertyRegistry*,
                        const String& value,
                        bool important,
                        SecureContextMode,
                        StyleSheetContents* context_style_sheet,
                        bool is_animation_tainted);
  void SetProperty(CSSPropertyID, const CSSValue&, bool important = false);

  // These do not. FIXME: This is too messy, we can do better.
  bool SetProperty(CSSPropertyID,
                   CSSValueID identifier,
                   bool important = false);
  bool SetProperty(const CSSPropertyValue&, CSSPropertyValue* slot = nullptr);

  template <typename T>  // CSSPropertyID or AtomicString
  bool RemoveProperty(T property, String* return_text = nullptr);
  bool RemovePropertiesInSet(const CSSProperty** set, unsigned length);
  void RemoveEquivalentProperties(const CSSPropertyValueSet*);
  void RemoveEquivalentProperties(const CSSStyleDeclaration*);

  void MergeAndOverrideOnConflict(const CSSPropertyValueSet*);

  void Clear();
  void ParseDeclarationList(const String& style_declaration,
                            SecureContextMode,
                            StyleSheetContents* context_style_sheet);

  CSSStyleDeclaration* EnsureCSSStyleDeclaration();

  template <typename T>  // CSSPropertyID or AtomicString
  int FindPropertyIndex(T property) const;

  void TraceAfterDispatch(blink::Visitor*);

 private:
  explicit MutableCSSPropertyValueSet(CSSParserMode);
  explicit MutableCSSPropertyValueSet(const CSSPropertyValueSet&);
  MutableCSSPropertyValueSet(const CSSPropertyValue* properties,
                             unsigned count);

  bool RemovePropertyAtIndex(int, String* return_text);

  bool RemoveShorthandProperty(CSSPropertyID);
  bool RemoveShorthandProperty(const AtomicString& custom_property_name) {
    return false;
  }
  CSSPropertyValue* FindCSSPropertyWithID(
      CSSPropertyID,
      const AtomicString& custom_property_name = g_null_atom);
  Member<PropertySetCSSStyleDeclaration> cssom_wrapper_;

  friend class CSSPropertyValueSet;

  HeapVector<CSSPropertyValue, 4> property_vector_;
};

DEFINE_TYPE_CASTS(MutableCSSPropertyValueSet,
                  CSSPropertyValueSet,
                  set,
                  set->IsMutable(),
                  set.IsMutable());

inline MutableCSSPropertyValueSet* ToMutableCSSPropertyValueSet(
    const Persistent<CSSPropertyValueSet>& set) {
  return ToMutableCSSPropertyValueSet(set.Get());
}

inline MutableCSSPropertyValueSet* ToMutableCSSPropertyValueSet(
    const Member<CSSPropertyValueSet>& set) {
  return ToMutableCSSPropertyValueSet(set.Get());
}

inline const CSSPropertyValueMetadata&
CSSPropertyValueSet::PropertyReference::PropertyMetadata() const {
  if (property_set_->IsMutable()) {
    return ToMutableCSSPropertyValueSet(*property_set_)
        .property_vector_.at(index_)
        .Metadata();
  }
  return ToImmutableCSSPropertyValueSet(*property_set_).MetadataArray()[index_];
}

inline const CSSValue& CSSPropertyValueSet::PropertyReference::PropertyValue()
    const {
  if (property_set_->IsMutable()) {
    return *ToMutableCSSPropertyValueSet(*property_set_)
                .property_vector_.at(index_)
                .Value();
  }
  return *ToImmutableCSSPropertyValueSet(*property_set_).ValueArray()[index_];
}

inline unsigned CSSPropertyValueSet::PropertyCount() const {
  if (is_mutable_)
    return ToMutableCSSPropertyValueSet(this)->property_vector_.size();
  return array_size_;
}

inline bool CSSPropertyValueSet::IsEmpty() const {
  return !PropertyCount();
}

template <typename T>
inline int CSSPropertyValueSet::FindPropertyIndex(T property) const {
  if (is_mutable_)
    return ToMutableCSSPropertyValueSet(this)->FindPropertyIndex(property);
  return ToImmutableCSSPropertyValueSet(this)->FindPropertyIndex(property);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PROPERTY_VALUE_SET_H_
