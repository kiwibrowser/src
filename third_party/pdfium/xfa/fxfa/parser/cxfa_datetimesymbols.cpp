// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_datetimesymbols.h"

namespace {

constexpr wchar_t kDateTimeSymbolsName[] = L"dateTimeSymbols";

}  // namespace

CXFA_DateTimeSymbols::CXFA_DateTimeSymbols(CXFA_Document* doc,
                                           XFA_PacketType packet)
    : CXFA_Node(doc,
                packet,
                XFA_XDPPACKET_LocaleSet,
                XFA_ObjectType::ContentNode,
                XFA_Element::DateTimeSymbols,
                nullptr,
                nullptr,
                kDateTimeSymbolsName) {}

CXFA_DateTimeSymbols::~CXFA_DateTimeSymbols() {}
