/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc.
 * All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_TRAVERSAL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_TRAVERSAL_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/container_node.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

template <class Iterator>
class TraversalRange;
template <class TraversalNext>
class TraversalAncestorsIterator;
template <class TraversalNext>
class TraversalInclusiveAncestorsIterator;
template <class TraversalNext>
class TraversalChildrenIterator;
template <class TraversalNext>
class TraversalDescendantIterator;
template <class TraversalNext>
class TraversalInclusiveDescendantIterator;
template <class TraversalNext>
class TraversalNextIterator;

class NodeTraversal {
  STATIC_ONLY(NodeTraversal);

 public:
  using TraversalNodeType = Node;

  // Does a pre-order traversal of the tree to find the next node after this
  // one.  This uses the same order that tags appear in the source file. If the
  // stayWithin argument is non-null, the traversal will stop once the specified
  // node is reached.  This can be used to restrict traversal to a particular
  // sub-tree.
  static Node* Next(const Node& current) {
    return TraverseNextTemplate(current);
  }
  static Node* Next(const ContainerNode& current) {
    return TraverseNextTemplate(current);
  }
  static Node* Next(const Node& current, const Node* stay_within) {
    return TraverseNextTemplate(current, stay_within);
  }
  static Node* Next(const ContainerNode& current, const Node* stay_within) {
    return TraverseNextTemplate(current, stay_within);
  }

  // Like next, but skips children and starts with the next sibling.
  static Node* NextSkippingChildren(const Node&);
  static Node* NextSkippingChildren(const Node&, const Node* stay_within);

  static Node* FirstWithin(const Node& current) { return current.firstChild(); }

  static Node* LastWithin(const ContainerNode&);
  static Node& LastWithinOrSelf(Node&);

  // Does a reverse pre-order traversal to find the node that comes before the
  // current one in document order
  static Node* Previous(const Node&, const Node* stay_within = nullptr);

  // Like previous, but skips children and starts with the next sibling.
  static Node* PreviousSkippingChildren(const Node&,
                                        const Node* stay_within = nullptr);

  // Like next, but visits parents after their children.
  static Node* NextPostOrder(const Node&, const Node* stay_within = nullptr);

  // Like previous, but visits parents before their children.
  static Node* PreviousPostOrder(const Node&,
                                 const Node* stay_within = nullptr);

  // Pre-order traversal including the pseudo-elements.
  static Node* PreviousIncludingPseudo(const Node&,
                                       const Node* stay_within = nullptr);
  static Node* NextIncludingPseudo(const Node&,
                                   const Node* stay_within = nullptr);
  static Node* NextIncludingPseudoSkippingChildren(
      const Node&,
      const Node* stay_within = nullptr);

  CORE_EXPORT static Node* NextAncestorSibling(const Node&);
  CORE_EXPORT static Node* NextAncestorSibling(const Node&,
                                               const Node* stay_within);
  static Node& HighestAncestorOrSelf(const Node&);

  // Children traversal.
  static Node* ChildAt(const Node& parent, unsigned index) {
    return ChildAtTemplate(parent, index);
  }
  static Node* ChildAt(const ContainerNode& parent, unsigned index) {
    return ChildAtTemplate(parent, index);
  }

  // These functions are provided for matching with |FlatTreeTraversal|.
  static bool HasChildren(const Node& parent) { return FirstChild(parent); }
  static bool IsDescendantOf(const Node& node, const Node& other) {
    return node.IsDescendantOf(&other);
  }
  static Node* FirstChild(const Node& parent) { return parent.firstChild(); }
  static Node* LastChild(const Node& parent) { return parent.lastChild(); }
  static Node* NextSibling(const Node& node) { return node.nextSibling(); }
  static Node* PreviousSibling(const Node& node) {
    return node.previousSibling();
  }
  static ContainerNode* Parent(const Node& node) { return node.parentNode(); }
  static Node* CommonAncestor(const Node& node_a, const Node& node_b);
  static unsigned Index(const Node& node) { return node.NodeIndex(); }
  static unsigned CountChildren(const Node& parent) {
    return parent.CountChildren();
  }
  static ContainerNode* ParentOrShadowHostNode(const Node& node) {
    return node.ParentOrShadowHostNode();
  }

  static TraversalRange<TraversalAncestorsIterator<NodeTraversal>> AncestorsOf(
      const Node&);
  static TraversalRange<TraversalInclusiveAncestorsIterator<NodeTraversal>>
  InclusiveAncestorsOf(const Node&);
  static TraversalRange<TraversalChildrenIterator<NodeTraversal>> ChildrenOf(
      const Node&);
  static TraversalRange<TraversalDescendantIterator<NodeTraversal>>
  DescendantsOf(const Node&);
  static TraversalRange<TraversalInclusiveDescendantIterator<NodeTraversal>>
  InclusiveDescendantsOf(const Node&);
  static TraversalRange<TraversalNextIterator<NodeTraversal>> StartsAt(
      const Node&);
  static TraversalRange<TraversalNextIterator<NodeTraversal>> StartsAfter(
      const Node&);

 private:
  template <class NodeType>
  static Node* TraverseNextTemplate(NodeType&);
  template <class NodeType>
  static Node* TraverseNextTemplate(NodeType&, const Node* stay_within);
  template <class NodeType>
  static Node* ChildAtTemplate(NodeType&, unsigned);
  static Node* PreviousAncestorSiblingPostOrder(const Node& current,
                                                const Node* stay_within);
};

template <class Iterator>
class TraversalRange {
  STACK_ALLOCATED();

 public:
  using StartNodeType = typename Iterator::StartNodeType;
  explicit TraversalRange(const StartNodeType* start) : start_(start) {}
  Iterator begin() { return Iterator(start_); }
  Iterator end() { return Iterator::End(); }

 private:
  Member<const StartNodeType> start_;
};

template <class TraversalNext>
class TraversalIteratorBase {
  STACK_ALLOCATED();

 public:
  using NodeType = typename TraversalNext::TraversalNodeType;
  NodeType& operator*() { return *current_; }
  bool operator!=(const TraversalIteratorBase& rval) const {
    return current_ != rval.current_;
  }

 protected:
  explicit TraversalIteratorBase(NodeType* current) : current_(current) {}

  Member<NodeType> current_;
};

template <class TraversalNext>
class TraversalAncestorsIterator : public TraversalIteratorBase<TraversalNext> {
  STACK_ALLOCATED();

 public:
  using StartNodeType = Node;
  using TraversalIteratorBase<TraversalNext>::current_;
  explicit TraversalAncestorsIterator(const StartNodeType* start)
      : TraversalIteratorBase<TraversalNext>(TraversalNext::Parent(*start)) {}
  void operator++() { current_ = TraversalNext::Parent(*current_); }
  static TraversalAncestorsIterator End() {
    return TraversalAncestorsIterator();
  }

 private:
  TraversalAncestorsIterator()
      : TraversalIteratorBase<TraversalNext>(nullptr) {}
};

template <class TraversalNext>
class TraversalInclusiveAncestorsIterator
    : public TraversalIteratorBase<TraversalNext> {
  STACK_ALLOCATED();

 public:
  using StartNodeType = Node;
  using TraversalIteratorBase<TraversalNext>::current_;
  explicit TraversalInclusiveAncestorsIterator(const StartNodeType* start)
      : TraversalIteratorBase<TraversalNext>(
            const_cast<StartNodeType*>(start)) {}
  void operator++() { current_ = TraversalNext::Parent(*current_); }
  static TraversalInclusiveAncestorsIterator End() {
    return TraversalInclusiveAncestorsIterator();
  }

 private:
  TraversalInclusiveAncestorsIterator()
      : TraversalIteratorBase<TraversalNext>(nullptr) {}
};

template <class TraversalNext>
class TraversalChildrenIterator : public TraversalIteratorBase<TraversalNext> {
  STACK_ALLOCATED();

 public:
  using StartNodeType = Node;
  using TraversalIteratorBase<TraversalNext>::current_;
  explicit TraversalChildrenIterator(const StartNodeType* start)
      : TraversalIteratorBase<TraversalNext>(
            TraversalNext::FirstChild(*start)) {}
  void operator++() { current_ = TraversalNext::NextSibling(*current_); }
  static TraversalChildrenIterator End() { return TraversalChildrenIterator(); }

 private:
  TraversalChildrenIterator() : TraversalIteratorBase<TraversalNext>(nullptr) {}
};

template <class TraversalNext>
class TraversalNextIterator : public TraversalIteratorBase<TraversalNext> {
  STACK_ALLOCATED();

 public:
  using StartNodeType = typename TraversalNext::TraversalNodeType;
  using TraversalIteratorBase<TraversalNext>::current_;
  explicit TraversalNextIterator(const StartNodeType* start)
      : TraversalIteratorBase<TraversalNext>(
            const_cast<StartNodeType*>(start)) {}
  void operator++() { current_ = TraversalNext::Next(*current_); }
  static TraversalNextIterator End() { return TraversalNextIterator(nullptr); }
};

template <class TraversalNext>
class TraversalDescendantIterator
    : public TraversalIteratorBase<TraversalNext> {
  STACK_ALLOCATED();

 public:
  using StartNodeType = Node;
  using TraversalIteratorBase<TraversalNext>::current_;
  explicit TraversalDescendantIterator(const StartNodeType* start)
      : TraversalIteratorBase<TraversalNext>(
            TraversalNext::FirstWithin(*start)),
        root_(start) {}
  void operator++() { current_ = TraversalNext::Next(*current_, root_); }
  static TraversalDescendantIterator End() {
    return TraversalDescendantIterator();
  }

 private:
  TraversalDescendantIterator()
      : TraversalIteratorBase<TraversalNext>(nullptr), root_(nullptr) {}
  Member<const Node> root_;
};

template <class TraversalNext>
class TraversalInclusiveDescendantIterator
    : public TraversalIteratorBase<TraversalNext> {
  STACK_ALLOCATED();

 public:
  using StartNodeType = typename TraversalNext::TraversalNodeType;
  using NodeType = typename TraversalNext::TraversalNodeType;
  using TraversalIteratorBase<TraversalNext>::current_;
  explicit TraversalInclusiveDescendantIterator(const StartNodeType* start)
      : TraversalIteratorBase<TraversalNext>(const_cast<NodeType*>(start)),
        root_(start) {}
  void operator++() { current_ = TraversalNext::Next(*current_, root_); }
  static TraversalInclusiveDescendantIterator End() {
    return TraversalInclusiveDescendantIterator(nullptr);
  }

 private:
  Member<const StartNodeType> root_;
};

inline TraversalRange<TraversalAncestorsIterator<NodeTraversal>>
NodeTraversal::AncestorsOf(const Node& node) {
  return TraversalRange<TraversalAncestorsIterator<NodeTraversal>>(&node);
}

inline TraversalRange<TraversalInclusiveAncestorsIterator<NodeTraversal>>
NodeTraversal::InclusiveAncestorsOf(const Node& node) {
  return TraversalRange<TraversalInclusiveAncestorsIterator<NodeTraversal>>(
      &node);
}

inline TraversalRange<TraversalChildrenIterator<NodeTraversal>>
NodeTraversal::ChildrenOf(const Node& parent) {
  return TraversalRange<TraversalChildrenIterator<NodeTraversal>>(&parent);
}

inline TraversalRange<TraversalDescendantIterator<NodeTraversal>>
NodeTraversal::DescendantsOf(const Node& root) {
  return TraversalRange<TraversalDescendantIterator<NodeTraversal>>(&root);
}

inline TraversalRange<TraversalInclusiveDescendantIterator<NodeTraversal>>
NodeTraversal::InclusiveDescendantsOf(const Node& root) {
  return TraversalRange<TraversalInclusiveDescendantIterator<NodeTraversal>>(
      &root);
}

inline TraversalRange<TraversalNextIterator<NodeTraversal>>
NodeTraversal::StartsAt(const Node& start) {
  return TraversalRange<TraversalNextIterator<NodeTraversal>>(&start);
};

inline TraversalRange<TraversalNextIterator<NodeTraversal>>
NodeTraversal::StartsAfter(const Node& start) {
  return TraversalRange<TraversalNextIterator<NodeTraversal>>(
      NodeTraversal::Next(start));
};

template <class NodeType>
inline Node* NodeTraversal::TraverseNextTemplate(NodeType& current) {
  if (current.hasChildren())
    return current.firstChild();
  if (current.nextSibling())
    return current.nextSibling();
  return NextAncestorSibling(current);
}

template <class NodeType>
inline Node* NodeTraversal::TraverseNextTemplate(NodeType& current,
                                                 const Node* stay_within) {
  if (current.hasChildren())
    return current.firstChild();
  if (current == stay_within)
    return nullptr;
  if (current.nextSibling())
    return current.nextSibling();
  return NextAncestorSibling(current, stay_within);
}

inline Node* NodeTraversal::NextSkippingChildren(const Node& current) {
  if (current.nextSibling())
    return current.nextSibling();
  return NextAncestorSibling(current);
}

inline Node* NodeTraversal::NextSkippingChildren(const Node& current,
                                                 const Node* stay_within) {
  if (current == stay_within)
    return nullptr;
  if (current.nextSibling())
    return current.nextSibling();
  return NextAncestorSibling(current, stay_within);
}

inline Node& NodeTraversal::HighestAncestorOrSelf(const Node& current) {
  Node* highest = const_cast<Node*>(&current);
  while (highest->parentNode())
    highest = highest->parentNode();
  return *highest;
}

template <class NodeType>
inline Node* NodeTraversal::ChildAtTemplate(NodeType& parent, unsigned index) {
  Node* child = parent.firstChild();
  while (child && index--)
    child = child->nextSibling();
  return child;
}

}  // namespace blink

#endif
