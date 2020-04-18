/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009 Apple Inc.
 *               All rights reserved.
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

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_LIST_ITEM_ORDINAL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_LIST_ITEM_ORDINAL_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"

namespace blink {

class HTMLOListElement;
class LayoutObject;
class Node;

// Represents an "ordinal value" and its related algorithms:
// https://html.spec.whatwg.org/multipage/grouping-content.html#ordinal-value
//
// The ordinal value is determined by the DOM tree order. However, since any
// elements with 'display: list-item' can be list items, the layout tree
// provides the storage for the instances of this class, and is responsible for
// firing events for insertions and removals.
class CORE_EXPORT ListItemOrdinal {
 public:
  ListItemOrdinal();

  // Get the corresponding instance for a node.
  static ListItemOrdinal* Get(const Node&);

  // Get the "ordinal value".
  int Value(const Node&) const;

  // Get/set/clear the explicit value; i.e., the 'value' attribute of an <li>
  // element.
  base::Optional<int> ExplicitValue() const;
  void SetExplicitValue(int, const Node&);
  void ClearExplicitValue(const Node&);

  // Get/set whether this item is in a list or not.
  bool NotInList() const { return not_in_list_; }
  void SetNotInList(bool);

  static bool IsList(const Node&);
  static bool IsListItem(const Node&);
  static bool IsListItem(const LayoutObject*);

  // Compute the total item count of a list.
  static unsigned ItemCountForOrderedList(const HTMLOListElement*);

  // Invalidate all ordinal values of a list.
  static void InvalidateAllItemsForOrderedList(const HTMLOListElement*);

  // Invalidate items that are affected by an insertion or a removal.
  static void ItemInsertedOrRemoved(const LayoutObject*);

 private:
  enum ValueType { kNeedsUpdate, kUpdated, kExplicit };
  ValueType Type() const { return static_cast<ValueType>(type_); }
  void SetType(ValueType type) const { type_ = type; }
  bool HasExplicitValue() const { return type_ == kExplicit; }

  static Node* EnclosingList(const Node*);
  struct NodeAndOrdinal {
    STACK_ALLOCATED();
    Persistent<const Node> node;
    ListItemOrdinal* ordinal = nullptr;
    operator bool() const { return node; }
  };
  static NodeAndOrdinal NextListItem(const Node* list_node,
                                     const Node* item_node = nullptr);
  static NodeAndOrdinal PreviousListItem(const Node* list_node,
                                         const Node* item_node);
  static NodeAndOrdinal NextOrdinalItem(bool is_reversed,
                                        const Node* list_node,
                                        const Node* item_node = nullptr);

  int CalcValue(const Node&) const;

  void InvalidateSelf(const Node&, ValueType = kNeedsUpdate);
  static void InvalidateAfter(const Node* list_node, const Node* item_node);
  static void InvalidateOrdinalsAfter(bool is_reversed,
                                      const Node* list_node,
                                      const Node* item_node);

  mutable int value_ = 0;
  mutable unsigned type_ : 2;  // ValueType
  unsigned not_in_list_ : 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_LIST_ITEM_ORDINAL_H_
