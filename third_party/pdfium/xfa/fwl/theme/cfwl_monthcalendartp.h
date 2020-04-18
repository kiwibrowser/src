// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_THEME_CFWL_MONTHCALENDARTP_H_
#define XFA_FWL_THEME_CFWL_MONTHCALENDARTP_H_

#include <memory>

#include "xfa/fwl/theme/cfwl_widgettp.h"

class CFWL_MonthCalendarTP : public CFWL_WidgetTP {
 public:
  CFWL_MonthCalendarTP();
  ~CFWL_MonthCalendarTP() override;

  // CFWL_WidgetTP
  void Initialize() override;
  void Finalize() override;
  void DrawBackground(CFWL_ThemeBackground* pParams) override;
  void DrawText(CFWL_ThemeText* pParams) override;

 protected:
  struct MCThemeData {
    FX_ARGB clrCaption;
    FX_ARGB clrSeperator;
    FX_ARGB clrDatesHoverBK;
    FX_ARGB clrDatesSelectedBK;
    FX_ARGB clrDatesCircle;
    FX_ARGB clrToday;
    FX_ARGB clrBK;
  };

  void DrawTotalBK(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawHeadBk(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawLButton(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawRButton(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawDatesInBK(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawDatesInCircle(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawTodayCircle(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawHSeperator(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  void DrawWeekNumSep(CFWL_ThemeBackground* pParams, CFX_Matrix* pMatrix);
  FWLTHEME_STATE GetState(uint32_t dwFWLStates);

  std::unique_ptr<MCThemeData> m_pThemeData;
  WideString wsResource;

 private:
  void SetThemeData();
};

#endif  // XFA_FWL_THEME_CFWL_MONTHCALENDARTP_H_
