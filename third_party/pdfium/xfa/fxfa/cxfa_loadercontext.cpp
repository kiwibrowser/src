// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_loadercontext.h"

CXFA_LoaderContext::CXFA_LoaderContext()
    : m_bSaveLineHeight(false),
      m_fWidth(0),
      m_fHeight(0),
      m_fLastPos(0),
      m_fStartLineOffset(0),
      m_iChar(0),
      m_iTotalLines(-1),
      m_dwFlags(0),
      m_pXMLNode(nullptr),
      m_pNode(nullptr) {}

CXFA_LoaderContext::~CXFA_LoaderContext() {}
