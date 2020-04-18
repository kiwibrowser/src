/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#include "third_party/blink/renderer/core/svg/svg_string_list.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg/svg_parser_utilities.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

SVGStringList::SVGStringList() = default;

SVGStringList::~SVGStringList() = default;

void SVGStringList::Initialize(const String& item) {
  values_.clear();
  values_.push_back(item);
}

String SVGStringList::GetItem(size_t index, ExceptionState& exception_state) {
  if (!CheckIndexBound(index, exception_state))
    return String();

  return values_.at(index);
}

void SVGStringList::InsertItemBefore(const String& new_item, size_t index) {
  // Spec: If the index is greater than or equal to numberOfItems, then the new
  // item is appended to the end of the list.
  if (index > values_.size())
    index = values_.size();

  // Spec: Inserts a new item into the list at the specified position. The index
  // of the item before which the new item is to be inserted. The first item is
  // number 0. If the index is equal to 0, then the new item is inserted at the
  // front of the list.
  values_.insert(index, new_item);
}

String SVGStringList::RemoveItem(size_t index,
                                 ExceptionState& exception_state) {
  if (!CheckIndexBound(index, exception_state))
    return String();

  String old_item = values_.at(index);
  values_.EraseAt(index);
  return old_item;
}

void SVGStringList::AppendItem(const String& new_item) {
  values_.push_back(new_item);
}

void SVGStringList::ReplaceItem(const String& new_item,
                                size_t index,
                                ExceptionState& exception_state) {
  if (!CheckIndexBound(index, exception_state))
    return;

  // Update the value at the desired position 'index'.
  values_[index] = new_item;
}

template <typename CharType>
void SVGStringList::ParseInternal(const CharType*& ptr, const CharType* end) {
  const UChar kDelimiter = ' ';

  while (ptr < end) {
    const CharType* start = ptr;
    while (ptr < end && *ptr != kDelimiter && !IsHTMLSpace<CharType>(*ptr))
      ptr++;
    if (ptr == start)
      break;
    values_.push_back(String(start, ptr - start));
    SkipOptionalSVGSpacesOrDelimiter(ptr, end, kDelimiter);
  }
}

SVGParsingError SVGStringList::SetValueAsString(const String& data) {
  // FIXME: Add more error checking and reporting.
  values_.clear();

  if (data.IsEmpty())
    return SVGParseStatus::kNoError;

  if (data.Is8Bit()) {
    const LChar* ptr = data.Characters8();
    const LChar* end = ptr + data.length();
    ParseInternal(ptr, end);
  } else {
    const UChar* ptr = data.Characters16();
    const UChar* end = ptr + data.length();
    ParseInternal(ptr, end);
  }
  return SVGParseStatus::kNoError;
}

String SVGStringList::ValueAsString() const {
  StringBuilder builder;

  Vector<String>::const_iterator it = values_.begin();
  Vector<String>::const_iterator it_end = values_.end();
  if (it != it_end) {
    builder.Append(*it);
    ++it;

    for (; it != it_end; ++it) {
      builder.Append(' ');
      builder.Append(*it);
    }
  }

  return builder.ToString();
}

bool SVGStringList::CheckIndexBound(size_t index,
                                    ExceptionState& exception_state) {
  if (index >= values_.size()) {
    exception_state.ThrowDOMException(
        kIndexSizeError, ExceptionMessages::IndexExceedsMaximumBound(
                             "index", index, values_.size()));
    return false;
  }

  return true;
}

void SVGStringList::Add(SVGPropertyBase* other, SVGElement* context_element) {
  // SVGStringList is never animated.
  NOTREACHED();
}

void SVGStringList::CalculateAnimatedValue(SVGAnimationElement*,
                                           float,
                                           unsigned,
                                           SVGPropertyBase*,
                                           SVGPropertyBase*,
                                           SVGPropertyBase*,
                                           SVGElement*) {
  // SVGStringList is never animated.
  NOTREACHED();
}

float SVGStringList::CalculateDistance(SVGPropertyBase*, SVGElement*) {
  // SVGStringList is never animated.
  NOTREACHED();

  return -1.0f;
}

}  // namespace blink
