/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CDATA_SECTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CDATA_SECTION_H_

#include "third_party/blink/renderer/core/dom/text.h"

namespace blink {

class CDATASection final : public Text {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static CDATASection* Create(Document&, const String&);

 private:
  CDATASection(Document&, const String&);

  String nodeName() const override;
  NodeType getNodeType() const override;
  Text* CloneWithData(Document&, const String&) const override;
};

DEFINE_NODE_TYPE_CASTS(CDATASection, getNodeType() == Node::kCdataSectionNode);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CDATA_SECTION_H_
