// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/xml/cfx_xmlinstruction.h"
#include "third_party/base/ptr_util.h"

CFX_XMLDocument::CFX_XMLDocument() {
  root_ = CreateNode<CFX_XMLElement>(L"root");
}

CFX_XMLDocument::~CFX_XMLDocument() = default;
