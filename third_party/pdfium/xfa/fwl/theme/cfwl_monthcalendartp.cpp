// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/theme/cfwl_monthcalendartp.h"

#include "xfa/fde/cfde_textout.h"
#include "xfa/fwl/cfwl_monthcalendar.h"
#include "xfa/fwl/cfwl_themebackground.h"
#include "xfa/fwl/cfwl_themetext.h"
#include "xfa/fwl/cfwl_widget.h"
#include "xfa/fwl/ifwl_themeprovider.h"
#include "xfa/fxgraphics/cxfa_gecolor.h"
#include "xfa/fxgraphics/cxfa_gepath.h"

CFWL_MonthCalendarTP::CFWL_MonthCalendarTP() : m_pThemeData(new MCThemeData) {
  SetThemeData();
}

CFWL_MonthCalendarTP::~CFWL_MonthCalendarTP() {}

void CFWL_MonthCalendarTP::Initialize() {
  CFWL_WidgetTP::Initialize();
  InitTTO();
}

void CFWL_MonthCalendarTP::Finalize() {
  FinalizeTTO();
  CFWL_WidgetTP::Finalize();
}

void CFWL_MonthCalendarTP::DrawBackground(CFWL_ThemeBackground* pParams) {
  if (!pParams)
    return;

  switch (pParams->m_iPart) {
    case CFWL_Part::Border: {
      DrawBorder(pParams->m_pGraphics, &pParams->m_rtPart, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::Background: {
      DrawTotalBK(pParams, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::Header: {
      DrawHeadBk(pParams, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::LBtn: {
      FWLTHEME_STATE eState = GetState(pParams->m_dwStates);
      DrawArrowBtn(pParams->m_pGraphics, &pParams->m_rtPart,
                   FWLTHEME_DIRECTION_Left, eState, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::RBtn: {
      FWLTHEME_STATE eState = GetState(pParams->m_dwStates);
      DrawArrowBtn(pParams->m_pGraphics, &pParams->m_rtPart,
                   FWLTHEME_DIRECTION_Right, eState, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::HSeparator: {
      DrawHSeperator(pParams, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::DatesIn: {
      DrawDatesInBK(pParams, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::TodayCircle: {
      DrawTodayCircle(pParams, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::DateInCircle: {
      DrawDatesInCircle(pParams, &pParams->m_matrix);
      break;
    }
    case CFWL_Part::WeekNumSep: {
      DrawWeekNumSep(pParams, &pParams->m_matrix);
      break;
    }
    default:
      break;
  }
}

void CFWL_MonthCalendarTP::DrawText(CFWL_ThemeText* pParams) {
  if (!m_pTextOut)
    return;

  if ((pParams->m_iPart == CFWL_Part::DatesIn) &&
      !(pParams->m_dwStates & FWL_ITEMSTATE_MCD_Flag) &&
      (pParams->m_dwStates &
       (CFWL_PartState_Hovered | CFWL_PartState_Selected))) {
    m_pTextOut->SetTextColor(0xFFFFFFFF);
  } else if (pParams->m_iPart == CFWL_Part::Caption) {
    m_pTextOut->SetTextColor(m_pThemeData->clrCaption);
  } else {
    m_pTextOut->SetTextColor(0xFF000000);
  }
  CFWL_WidgetTP::DrawText(pParams);
}

void CFWL_MonthCalendarTP::DrawTotalBK(CFWL_ThemeBackground* pParams,
                                       CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtTotal(pParams->m_rtPart);
  path.AddRectangle(rtTotal.left, rtTotal.top, rtTotal.width, rtTotal.height);
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetFillColor(CXFA_GEColor(m_pThemeData->clrBK));
  pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawHeadBk(CFWL_ThemeBackground* pParams,
                                      CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtHead = pParams->m_rtPart;
  path.AddRectangle(rtHead.left, rtHead.top, rtHead.width, rtHead.height);
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetFillColor(CXFA_GEColor(m_pThemeData->clrBK));
  pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawLButton(CFWL_ThemeBackground* pParams,
                                       CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtLBtn = pParams->m_rtPart;
  path.AddRectangle(rtLBtn.left, rtLBtn.top, rtLBtn.width, rtLBtn.height);
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(ArgbEncode(0xff, 205, 219, 243)));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  if (pParams->m_dwStates & CFWL_PartState_Pressed) {
    pParams->m_pGraphics->SetFillColor(
        CXFA_GEColor(ArgbEncode(0xff, 174, 198, 242)));
    pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  } else {
    pParams->m_pGraphics->SetFillColor(
        CXFA_GEColor(ArgbEncode(0xff, 227, 235, 249)));
    pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  }

  path.Clear();
  path.MoveTo(CFX_PointF(rtLBtn.left + rtLBtn.Width() / 3 * 2,
                         rtLBtn.top + rtLBtn.height / 4));
  path.LineTo(CFX_PointF(rtLBtn.left + rtLBtn.Width() / 3,
                         rtLBtn.top + rtLBtn.height / 2));
  path.LineTo(CFX_PointF(rtLBtn.left + rtLBtn.Width() / 3 * 2,
                         rtLBtn.bottom() - rtLBtn.height / 4));

  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(ArgbEncode(0xff, 50, 104, 205)));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawRButton(CFWL_ThemeBackground* pParams,
                                       CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtRBtn = pParams->m_rtPart;
  path.AddRectangle(rtRBtn.left, rtRBtn.top, rtRBtn.width, rtRBtn.height);
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(ArgbEncode(0xff, 205, 219, 243)));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  if (pParams->m_dwStates & CFWL_PartState_Pressed) {
    pParams->m_pGraphics->SetFillColor(
        CXFA_GEColor(ArgbEncode(0xff, 174, 198, 242)));
    pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  } else {
    pParams->m_pGraphics->SetFillColor(
        CXFA_GEColor(ArgbEncode(0xff, 227, 235, 249)));
    pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  }

  path.Clear();
  path.MoveTo(CFX_PointF(rtRBtn.left + rtRBtn.Width() / 3,
                         rtRBtn.top + rtRBtn.height / 4));
  path.LineTo(CFX_PointF(rtRBtn.left + rtRBtn.Width() / 3 * 2,
                         rtRBtn.top + rtRBtn.height / 2));
  path.LineTo(CFX_PointF(rtRBtn.left + rtRBtn.Width() / 3,
                         rtRBtn.bottom() - rtRBtn.height / 4));

  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(ArgbEncode(0xff, 50, 104, 205)));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawHSeperator(CFWL_ThemeBackground* pParams,
                                          CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtHSep = pParams->m_rtPart;
  path.MoveTo(CFX_PointF(rtHSep.left, rtHSep.top + rtHSep.height / 2));
  path.LineTo(CFX_PointF(rtHSep.right(), rtHSep.top + rtHSep.height / 2));
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(m_pThemeData->clrSeperator));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawWeekNumSep(CFWL_ThemeBackground* pParams,
                                          CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtWeekSep = pParams->m_rtPart;
  path.MoveTo(rtWeekSep.TopLeft());
  path.LineTo(rtWeekSep.BottomLeft());
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(m_pThemeData->clrSeperator));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawDatesInBK(CFWL_ThemeBackground* pParams,
                                         CFX_Matrix* pMatrix) {
  pParams->m_pGraphics->SaveGraphState();
  if (pParams->m_dwStates & CFWL_PartState_Selected) {
    CXFA_GEPath path;
    CFX_RectF rtSelDay = pParams->m_rtPart;
    path.AddRectangle(rtSelDay.left, rtSelDay.top, rtSelDay.width,
                      rtSelDay.height);
    pParams->m_pGraphics->SetFillColor(
        CXFA_GEColor(m_pThemeData->clrDatesSelectedBK));
    pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  } else if (pParams->m_dwStates & CFWL_PartState_Hovered) {
    CXFA_GEPath path;
    CFX_RectF rtSelDay = pParams->m_rtPart;
    path.AddRectangle(rtSelDay.left, rtSelDay.top, rtSelDay.width,
                      rtSelDay.height);
    pParams->m_pGraphics->SetFillColor(
        CXFA_GEColor(m_pThemeData->clrDatesHoverBK));
    pParams->m_pGraphics->FillPath(&path, FXFILL_WINDING, pMatrix);
  }
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawDatesInCircle(CFWL_ThemeBackground* pParams,
                                             CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtSelDay = pParams->m_rtPart;
  path.AddRectangle(rtSelDay.left, rtSelDay.top, rtSelDay.width,
                    rtSelDay.height);
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(m_pThemeData->clrDatesCircle));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

void CFWL_MonthCalendarTP::DrawTodayCircle(CFWL_ThemeBackground* pParams,
                                           CFX_Matrix* pMatrix) {
  CXFA_GEPath path;
  CFX_RectF rtTodayCircle = pParams->m_rtPart;
  path.AddRectangle(rtTodayCircle.left, rtTodayCircle.top, rtTodayCircle.width,
                    rtTodayCircle.height);
  pParams->m_pGraphics->SaveGraphState();
  pParams->m_pGraphics->SetStrokeColor(
      CXFA_GEColor(m_pThemeData->clrDatesCircle));
  pParams->m_pGraphics->StrokePath(&path, pMatrix);
  pParams->m_pGraphics->RestoreGraphState();
}

FWLTHEME_STATE CFWL_MonthCalendarTP::GetState(uint32_t dwFWLStates) {
  if (dwFWLStates & CFWL_PartState_Hovered)
    return FWLTHEME_STATE_Hover;
  if (dwFWLStates & CFWL_PartState_Pressed)
    return FWLTHEME_STATE_Pressed;
  return FWLTHEME_STATE_Normal;
}

void CFWL_MonthCalendarTP::SetThemeData() {
  m_pThemeData->clrCaption = ArgbEncode(0xff, 0, 153, 255);
  m_pThemeData->clrSeperator = ArgbEncode(0xff, 141, 161, 239);
  m_pThemeData->clrDatesHoverBK = ArgbEncode(0xff, 193, 211, 251);
  m_pThemeData->clrDatesSelectedBK = ArgbEncode(0xff, 173, 188, 239);
  m_pThemeData->clrDatesCircle = ArgbEncode(0xff, 103, 144, 209);
  m_pThemeData->clrToday = ArgbEncode(0xff, 0, 0, 0);
  m_pThemeData->clrBK = ArgbEncode(0xff, 255, 255, 255);
}
