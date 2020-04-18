// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_WIDGETPROPERTIES_H_
#define XFA_FWL_CFWL_WIDGETPROPERTIES_H_

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_system.h"
#include "xfa/fwl/cfwl_widget.h"
#include "xfa/fwl/fwl_widgetdef.h"

class IFWL_ThemeProvider;
class CFWL_Widget;

class CFWL_WidgetProperties {
 public:
  CFWL_WidgetProperties();
  ~CFWL_WidgetProperties();

  CFX_RectF m_rtWidget;
  uint32_t m_dwStyles;
  uint32_t m_dwStyleExes;
  uint32_t m_dwStates;
  IFWL_ThemeProvider* m_pThemeProvider;
  CFWL_Widget* m_pParent;
  CFWL_Widget* m_pOwner;
};

#endif  // XFA_FWL_CFWL_WIDGETPROPERTIES_H_
