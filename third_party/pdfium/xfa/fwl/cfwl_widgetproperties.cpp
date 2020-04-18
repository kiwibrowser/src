// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfwl_widgetproperties.h"

CFWL_WidgetProperties::CFWL_WidgetProperties()
    : m_dwStyles(FWL_WGTSTYLE_Child),
      m_dwStyleExes(0),
      m_dwStates(0),
      m_pThemeProvider(nullptr),
      m_pParent(nullptr),
      m_pOwner(nullptr) {}

CFWL_WidgetProperties::~CFWL_WidgetProperties() {}
