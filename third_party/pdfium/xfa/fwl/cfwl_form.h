// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_FORM_H_
#define XFA_FWL_CFWL_FORM_H_

#include <memory>

#include "core/fxcrt/fx_system.h"
#include "xfa/fwl/cfwl_widget.h"
#include "xfa/fwl/cfwl_widgetproperties.h"

#define FWL_CLASS_Form L"FWL_FORM"

class CFWL_MessageMouse;
class CFWL_NoteLoop;
class CFWL_Widget;
class IFWL_ThemeProvider;

class CFWL_Form : public CFWL_Widget {
 public:
  CFWL_Form(const CFWL_App* app,
            std::unique_ptr<CFWL_WidgetProperties> properties,
            CFWL_Widget* pOuter);
  ~CFWL_Form() override;

  // CFWL_Widget
  FWL_Type GetClassID() const override;
  bool IsInstance(const WideStringView& wsClass) const override;
  CFX_RectF GetClientRect() override;
  void Update() override;
  FWL_WidgetHit HitTest(const CFX_PointF& point) override;
  void DrawWidget(CXFA_Graphics* pGraphics, const CFX_Matrix& matrix) override;
  void OnProcessMessage(CFWL_Message* pMessage) override;
  void OnDrawWidget(CXFA_Graphics* pGraphics,
                    const CFX_Matrix& matrix) override;

  CFWL_Widget* DoModal();
  void EndDoModal();

  CFWL_Widget* GetSubFocus() const { return m_pSubFocus; }
  void SetSubFocus(CFWL_Widget* pWidget) { m_pSubFocus = pWidget; }

 private:
  void DrawBackground(CXFA_Graphics* pGraphics, IFWL_ThemeProvider* pTheme);
  CFX_RectF GetEdgeRect();
  void SetWorkAreaRect();
  void Layout();
  void RegisterForm();
  void UnRegisterForm();
  void OnLButtonDown(CFWL_MessageMouse* pMsg);
  void OnLButtonUp(CFWL_MessageMouse* pMsg);

  CFX_RectF m_rtRestore;
  CFX_RectF m_rtRelative;
  std::unique_ptr<CFWL_NoteLoop> m_pNoteLoop;
  CFWL_Widget* m_pSubFocus;
  float m_fCXBorder;
  float m_fCYBorder;
};

#endif  // XFA_FWL_CFWL_FORM_H_
