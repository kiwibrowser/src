// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_XML_CFX_XMLTEXT_H_
#define CORE_FXCRT_XML_CFX_XMLTEXT_H_

#include <memory>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/xml/cfx_xmlnode.h"

class CFX_XMLDocument;

class CFX_XMLText : public CFX_XMLNode {
 public:
  explicit CFX_XMLText(const WideString& wsText);
  ~CFX_XMLText() override;

  // CFX_XMLNode
  FX_XMLNODETYPE GetType() const override;
  CFX_XMLNode* Clone(CFX_XMLDocument* doc) override;
  void Save(const RetainPtr<IFX_SeekableWriteStream>& pXMLStream) override;

  WideString GetText() const { return m_wsText; }
  void SetText(const WideString& wsText) { m_wsText = wsText; }

 private:
  WideString m_wsText;
};

#endif  // CORE_FXCRT_XML_CFX_XMLTEXT_H_
