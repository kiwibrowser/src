// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <cstdint>
#include <memory>

#include "core/fxcrt/cfx_memorystream.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlparser.h"
#include "third_party/base/ptr_util.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FX_SAFE_SIZE_T safe_size = size;
  if (!safe_size.IsValid())
    return 0;

  auto stream = pdfium::MakeRetain<CFX_MemoryStream>(const_cast<uint8_t*>(data),
                                                     size, false);

  CFX_XMLParser parser(stream);
  std::unique_ptr<CFX_XMLDocument> doc = parser.Parse();
  if (!doc || !doc->GetRoot())
    return 0;

  for (CFX_XMLNode* pXMLNode = doc->GetRoot()->GetFirstChild(); pXMLNode;
       pXMLNode = pXMLNode->GetNextSibling()) {
    if (pXMLNode->GetType() == FX_XMLNODE_Element)
      break;
  }
  return 0;
}
