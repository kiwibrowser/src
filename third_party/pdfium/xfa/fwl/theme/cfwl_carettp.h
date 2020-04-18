// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_THEME_CFWL_CARETTP_H_
#define XFA_FWL_THEME_CFWL_CARETTP_H_

#include "xfa/fwl/theme/cfwl_widgettp.h"

class CFWL_CaretTP : public CFWL_WidgetTP {
 public:
  CFWL_CaretTP();
  ~CFWL_CaretTP() override;

  // CFWL_WidgetTP
  void DrawBackground(CFWL_ThemeBackground* pParams) override;

 protected:
  void DrawCaretBK(CXFA_Graphics* pGraphics,
                   uint32_t dwStates,
                   const CFX_RectF* pRect,
                   CXFA_GEColor* crFill,
                   CFX_Matrix* pMatrix = nullptr);
};

#endif  // XFA_FWL_THEME_CFWL_CARETTP_H_
