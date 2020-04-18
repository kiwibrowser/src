// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfwl_event.h"

CFWL_Event::CFWL_Event(CFWL_Event::Type type)
    : CFWL_Event(type, nullptr, nullptr) {}

CFWL_Event::CFWL_Event(Type type, CFWL_Widget* pSrcTarget)
    : CFWL_Event(type, pSrcTarget, nullptr) {}

CFWL_Event::CFWL_Event(Type type,
                       CFWL_Widget* pSrcTarget,
                       CFWL_Widget* pDstTarget)
    : m_pSrcTarget(pSrcTarget), m_pDstTarget(pDstTarget), m_type(type) {}

CFWL_Event::~CFWL_Event() {}
