// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/xml/cfx_xmlinstruction.h"

#include <utility>

#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"

CFX_XMLInstruction::CFX_XMLInstruction(const WideString& wsTarget)
    : CFX_XMLNode(), name_(wsTarget) {}

CFX_XMLInstruction::~CFX_XMLInstruction() = default;

FX_XMLNODETYPE CFX_XMLInstruction::GetType() const {
  return FX_XMLNODE_Instruction;
}

CFX_XMLNode* CFX_XMLInstruction::Clone(CFX_XMLDocument* doc) {
  auto* node = doc->CreateNode<CFX_XMLInstruction>(name_);
  node->m_TargetData = m_TargetData;
  return node;
}

void CFX_XMLInstruction::AppendData(const WideString& wsData) {
  m_TargetData.push_back(wsData);
}

bool CFX_XMLInstruction::IsOriginalXFAVersion() const {
  return name_ == L"originalXFAVersion";
}

bool CFX_XMLInstruction::IsAcrobat() const {
  return name_ == L"acrobat";
}

void CFX_XMLInstruction::Save(
    const RetainPtr<IFX_SeekableWriteStream>& pXMLStream) {
  if (name_.CompareNoCase(L"xml") == 0) {
    pXMLStream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    return;
  }

  pXMLStream->WriteString("<?");
  pXMLStream->WriteString(name_.UTF8Encode().AsStringView());
  pXMLStream->WriteString(" ");

  for (const WideString& target : m_TargetData) {
    pXMLStream->WriteString(target.UTF8Encode().AsStringView());
    pXMLStream->WriteString(" ");
  }

  pXMLStream->WriteString("?>\n");
}
