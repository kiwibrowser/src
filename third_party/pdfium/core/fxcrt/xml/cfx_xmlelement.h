// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_XML_CFX_XMLELEMENT_H_
#define CORE_FXCRT_XML_CFX_XMLELEMENT_H_

#include <map>
#include <memory>
#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/xml/cfx_xmlnode.h"

class CFX_XMLDocument;

class CFX_XMLElement : public CFX_XMLNode {
 public:
  explicit CFX_XMLElement(const WideString& wsTag);
  ~CFX_XMLElement() override;

  // CFX_XMLNode
  FX_XMLNODETYPE GetType() const override;
  CFX_XMLNode* Clone(CFX_XMLDocument* doc) override;
  void Save(const RetainPtr<IFX_SeekableWriteStream>& pXMLStream) override;

  WideString GetName() const { return name_; }

  const std::map<WideString, WideString>& GetAttributes() const {
    return attrs_;
  }
  bool HasAttribute(const WideString& name) const;
  void SetAttribute(const WideString& name, const WideString& value);
  WideString GetAttribute(const WideString& name) const;

  void RemoveAttribute(const WideString& name);

  CFX_XMLElement* GetFirstChildNamed(const WideStringView& name) const;
  CFX_XMLElement* GetNthChildNamed(const WideStringView& name,
                                   size_t idx) const;

  WideString GetLocalTagName() const;
  WideString GetNamespacePrefix() const;
  WideString GetNamespaceURI() const;

  WideString GetTextData() const;

 private:
  WideString AttributeToString(const WideString& name, const WideString& value);

  WideString name_;
  std::map<WideString, WideString> attrs_;
};

#endif  // CORE_FXCRT_XML_CFX_XMLELEMENT_H_
