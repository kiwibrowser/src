// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/formfiller/cffl_interactiveformfiller.h"

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fxcrt/autorestorer.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_pathdata.h"
#include "core/fxge/cfx_renderdevice.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_interform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/cpdfsdk_widget.h"
#include "fpdfsdk/formfiller/cffl_checkbox.h"
#include "fpdfsdk/formfiller/cffl_combobox.h"
#include "fpdfsdk/formfiller/cffl_formfiller.h"
#include "fpdfsdk/formfiller/cffl_listbox.h"
#include "fpdfsdk/formfiller/cffl_pushbutton.h"
#include "fpdfsdk/formfiller/cffl_radiobutton.h"
#include "fpdfsdk/formfiller/cffl_textfield.h"
#include "third_party/base/stl_util.h"

CFFL_InteractiveFormFiller::CFFL_InteractiveFormFiller(
    CPDFSDK_FormFillEnvironment* pFormFillEnv)
    : m_pFormFillEnv(pFormFillEnv), m_bNotifying(false) {}

CFFL_InteractiveFormFiller::~CFFL_InteractiveFormFiller() {}

bool CFFL_InteractiveFormFiller::Annot_HitTest(CPDFSDK_PageView* pPageView,
                                               CPDFSDK_Annot* pAnnot,
                                               const CFX_PointF& point) {
  return pAnnot->GetRect().Contains(point);
}

FX_RECT CFFL_InteractiveFormFiller::GetViewBBox(CPDFSDK_PageView* pPageView,
                                                CPDFSDK_Annot* pAnnot) {
  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false))
    return pFormFiller->GetViewBBox(pPageView, pAnnot);

  ASSERT(pPageView);

  CPDF_Annot* pPDFAnnot = pAnnot->GetPDFAnnot();
  CFX_FloatRect rcWin = pPDFAnnot->GetRect();
  if (!rcWin.IsEmpty()) {
    rcWin.Inflate(1, 1);
    rcWin.Normalize();
  }
  return rcWin.GetOuterRect();
}

void CFFL_InteractiveFormFiller::OnDraw(CPDFSDK_PageView* pPageView,
                                        CPDFSDK_Annot* pAnnot,
                                        CFX_RenderDevice* pDevice,
                                        CFX_Matrix* pUser2Device) {
  ASSERT(pPageView);
  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot);
  if (!IsVisible(pWidget))
    return;

  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  if (pFormFiller && pFormFiller->IsValid()) {
    pFormFiller->OnDraw(pPageView, pAnnot, pDevice, *pUser2Device);
    pAnnot->GetPDFPage();

    if (m_pFormFillEnv->GetFocusAnnot() != pAnnot)
      return;

    CFX_FloatRect rcFocus = pFormFiller->GetFocusBox(pPageView);
    if (rcFocus.IsEmpty())
      return;

    CFX_PathData path;
    path.AppendPoint(CFX_PointF(rcFocus.left, rcFocus.top), FXPT_TYPE::MoveTo,
                     false);
    path.AppendPoint(CFX_PointF(rcFocus.left, rcFocus.bottom),
                     FXPT_TYPE::LineTo, false);
    path.AppendPoint(CFX_PointF(rcFocus.right, rcFocus.bottom),
                     FXPT_TYPE::LineTo, false);
    path.AppendPoint(CFX_PointF(rcFocus.right, rcFocus.top), FXPT_TYPE::LineTo,
                     false);
    path.AppendPoint(CFX_PointF(rcFocus.left, rcFocus.top), FXPT_TYPE::LineTo,
                     false);

    CFX_GraphStateData gsd;
    gsd.SetDashCount(1);
    gsd.m_DashArray[0] = 1.0f;
    gsd.m_DashPhase = 0;
    gsd.m_LineWidth = 1.0f;
    pDevice->DrawPath(&path, pUser2Device, &gsd, 0, ArgbEncode(255, 0, 0, 0),
                      FXFILL_ALTERNATE);
    return;
  }

  pFormFiller = GetFormFiller(pAnnot, false);
  if (pFormFiller) {
    pFormFiller->OnDrawDeactive(pPageView, pAnnot, pDevice, *pUser2Device);
  } else {
    pWidget->DrawAppearance(pDevice, *pUser2Device, CPDF_Annot::Normal,
                            nullptr);
  }

  if (!IsReadOnly(pWidget) && IsFillingAllowed(pWidget))
    pWidget->DrawShadow(pDevice, pPageView);
}

void CFFL_InteractiveFormFiller::OnDelete(CPDFSDK_Annot* pAnnot) {
  UnRegisterFormFiller(pAnnot);
}

void CFFL_InteractiveFormFiller::OnMouseEnter(
    CPDFSDK_PageView* pPageView,
    CPDFSDK_Annot::ObservedPtr* pAnnot,
    uint32_t nFlag) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  if (!m_bNotifying) {
    CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
    if (pWidget->GetAAction(CPDF_AAction::CursorEnter).GetDict()) {
      m_bNotifying = true;

      uint32_t nValueAge = pWidget->GetValueAge();
      pWidget->ClearAppModified();
      ASSERT(pPageView);

      CPDFSDK_FieldAction fa;
      fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
      fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
      pWidget->OnAAction(CPDF_AAction::CursorEnter, &fa, pPageView);
      m_bNotifying = false;
      if (!(*pAnnot))
        return;

      if (pWidget->IsAppModified()) {
        if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false)) {
          pFormFiller->ResetPDFWindow(pPageView,
                                      pWidget->GetValueAge() == nValueAge);
        }
      }
    }
  }
  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), true))
    pFormFiller->OnMouseEnter(pPageView, pAnnot->Get());
}

void CFFL_InteractiveFormFiller::OnMouseExit(CPDFSDK_PageView* pPageView,
                                             CPDFSDK_Annot::ObservedPtr* pAnnot,
                                             uint32_t nFlag) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  if (!m_bNotifying) {
    CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
    if (pWidget->GetAAction(CPDF_AAction::CursorExit).GetDict()) {
      m_bNotifying = true;

      uint32_t nValueAge = pWidget->GetValueAge();
      pWidget->ClearAppModified();
      ASSERT(pPageView);

      CPDFSDK_FieldAction fa;
      fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
      fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
      pWidget->OnAAction(CPDF_AAction::CursorExit, &fa, pPageView);
      m_bNotifying = false;
      if (!(*pAnnot))
        return;

      if (pWidget->IsAppModified()) {
        if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false)) {
          pFormFiller->ResetPDFWindow(pPageView,
                                      nValueAge == pWidget->GetValueAge());
        }
      }
    }
  }
  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false))
    pFormFiller->OnMouseExit(pPageView, pAnnot->Get());
}

bool CFFL_InteractiveFormFiller::OnLButtonDown(
    CPDFSDK_PageView* pPageView,
    CPDFSDK_Annot::ObservedPtr* pAnnot,
    uint32_t nFlags,
    const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  if (!m_bNotifying) {
    CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
    if (Annot_HitTest(pPageView, pAnnot->Get(), point) &&
        pWidget->GetAAction(CPDF_AAction::ButtonDown).GetDict()) {
      m_bNotifying = true;

      uint32_t nValueAge = pWidget->GetValueAge();
      pWidget->ClearAppModified();
      ASSERT(pPageView);

      CPDFSDK_FieldAction fa;
      fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlags);
      fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlags);
      pWidget->OnAAction(CPDF_AAction::ButtonDown, &fa, pPageView);
      m_bNotifying = false;
      if (!(*pAnnot))
        return true;

      if (!IsValidAnnot(pPageView, pAnnot->Get()))
        return true;

      if (pWidget->IsAppModified()) {
        if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false)) {
          pFormFiller->ResetPDFWindow(pPageView,
                                      nValueAge == pWidget->GetValueAge());
        }
      }
    }
  }
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  return pFormFiller &&
         pFormFiller->OnLButtonDown(pPageView, pAnnot->Get(), nFlags, point);
}

bool CFFL_InteractiveFormFiller::OnLButtonUp(CPDFSDK_PageView* pPageView,
                                             CPDFSDK_Annot::ObservedPtr* pAnnot,
                                             uint32_t nFlags,
                                             const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());

  bool bSetFocus;
  switch (pWidget->GetFieldType()) {
    case FormFieldType::kPushButton:
    case FormFieldType::kCheckBox:
    case FormFieldType::kRadioButton: {
      FX_RECT bbox = GetViewBBox(pPageView, pAnnot->Get());
      bSetFocus =
          bbox.Contains(static_cast<int>(point.x), static_cast<int>(point.y));
      break;
    }
    default:
      bSetFocus = true;
      break;
  }
  if (bSetFocus)
    m_pFormFillEnv->SetFocusAnnot(pAnnot);

  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  bool bRet = pFormFiller &&
              pFormFiller->OnLButtonUp(pPageView, pAnnot->Get(), nFlags, point);
  if (m_pFormFillEnv->GetFocusAnnot() != pAnnot->Get())
    return bRet;
  if (OnButtonUp(pAnnot, pPageView, nFlags) || !pAnnot)
    return true;
#ifdef PDF_ENABLE_XFA
  if (OnClick(pAnnot, pPageView, nFlags) || !pAnnot)
    return true;
#endif  // PDF_ENABLE_XFA
  return bRet;
}

bool CFFL_InteractiveFormFiller::OnButtonUp(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                            CPDFSDK_PageView* pPageView,
                                            uint32_t nFlag) {
  if (m_bNotifying)
    return false;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->GetAAction(CPDF_AAction::ButtonUp).GetDict())
    return false;

  m_bNotifying = true;

  uint32_t nAge = pWidget->GetAppearanceAge();
  uint32_t nValueAge = pWidget->GetValueAge();
  ASSERT(pPageView);

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
  pWidget->OnAAction(CPDF_AAction::ButtonUp, &fa, pPageView);
  m_bNotifying = false;
  if (!(*pAnnot) || !IsValidAnnot(pPageView, pWidget))
    return true;
  if (nAge == pWidget->GetAppearanceAge())
    return false;

  CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false);
  if (pFormFiller)
    pFormFiller->ResetPDFWindow(pPageView, nValueAge == pWidget->GetValueAge());
  return true;
}

bool CFFL_InteractiveFormFiller::OnLButtonDblClk(
    CPDFSDK_PageView* pPageView,
    CPDFSDK_Annot::ObservedPtr* pAnnot,
    uint32_t nFlags,
    const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  return pFormFiller &&
         pFormFiller->OnLButtonDblClk(pPageView, pAnnot->Get(), nFlags, point);
}

bool CFFL_InteractiveFormFiller::OnMouseMove(CPDFSDK_PageView* pPageView,
                                             CPDFSDK_Annot::ObservedPtr* pAnnot,
                                             uint32_t nFlags,
                                             const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), true);
  return pFormFiller &&
         pFormFiller->OnMouseMove(pPageView, pAnnot->Get(), nFlags, point);
}

bool CFFL_InteractiveFormFiller::OnMouseWheel(
    CPDFSDK_PageView* pPageView,
    CPDFSDK_Annot::ObservedPtr* pAnnot,
    uint32_t nFlags,
    short zDelta,
    const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  return pFormFiller &&
         pFormFiller->OnMouseWheel(pPageView, pAnnot->Get(), nFlags, zDelta,
                                   point);
}

bool CFFL_InteractiveFormFiller::OnRButtonDown(
    CPDFSDK_PageView* pPageView,
    CPDFSDK_Annot::ObservedPtr* pAnnot,
    uint32_t nFlags,
    const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  return pFormFiller &&
         pFormFiller->OnRButtonDown(pPageView, pAnnot->Get(), nFlags, point);
}

bool CFFL_InteractiveFormFiller::OnRButtonUp(CPDFSDK_PageView* pPageView,
                                             CPDFSDK_Annot::ObservedPtr* pAnnot,
                                             uint32_t nFlags,
                                             const CFX_PointF& point) {
  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  return pFormFiller &&
         pFormFiller->OnRButtonUp(pPageView, pAnnot->Get(), nFlags, point);
}

bool CFFL_InteractiveFormFiller::OnKeyDown(CPDFSDK_Annot* pAnnot,
                                           uint32_t nKeyCode,
                                           uint32_t nFlags) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);

  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller && pFormFiller->OnKeyDown(pAnnot, nKeyCode, nFlags);
}

bool CFFL_InteractiveFormFiller::OnChar(CPDFSDK_Annot* pAnnot,
                                        uint32_t nChar,
                                        uint32_t nFlags) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  if (nChar == FWL_VKEY_Tab)
    return true;

  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller && pFormFiller->OnChar(pAnnot, nChar, nFlags);
}

bool CFFL_InteractiveFormFiller::OnSetFocus(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                            uint32_t nFlag) {
  if (!(*pAnnot))
    return false;

  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  if (!m_bNotifying) {
    CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
    if (pWidget->GetAAction(CPDF_AAction::GetFocus).GetDict()) {
      m_bNotifying = true;

      uint32_t nValueAge = pWidget->GetValueAge();
      pWidget->ClearAppModified();

      CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, true);
      if (!pFormFiller)
        return false;

      CPDFSDK_PageView* pPageView = (*pAnnot)->GetPageView();
      ASSERT(pPageView);

      CPDFSDK_FieldAction fa;
      fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
      fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
      pFormFiller->GetActionData(pPageView, CPDF_AAction::GetFocus, fa);
      pWidget->OnAAction(CPDF_AAction::GetFocus, &fa, pPageView);
      m_bNotifying = false;
      if (!(*pAnnot))
        return false;

      if (pWidget->IsAppModified()) {
        if (CFFL_FormFiller* pFiller = GetFormFiller(pWidget, false)) {
          pFiller->ResetPDFWindow(pPageView,
                                  nValueAge == pWidget->GetValueAge());
        }
      }
    }
  }

  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), true))
    pFormFiller->SetFocusForAnnot(pAnnot->Get(), nFlag);

  return true;
}

bool CFFL_InteractiveFormFiller::OnKillFocus(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                             uint32_t nFlag) {
  if (!(*pAnnot))
    return false;

  ASSERT((*pAnnot)->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot->Get(), false);
  if (!pFormFiller)
    return true;

  pFormFiller->KillFocusForAnnot(pAnnot->Get(), nFlag);
  if (!(*pAnnot))
    return false;

  if (m_bNotifying)
    return true;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->GetAAction(CPDF_AAction::LoseFocus).GetDict())
    return true;

  m_bNotifying = true;
  pWidget->ClearAppModified();

  CPDFSDK_PageView* pPageView = pWidget->GetPageView();
  ASSERT(pPageView);

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
  pFormFiller->GetActionData(pPageView, CPDF_AAction::LoseFocus, fa);
  pWidget->OnAAction(CPDF_AAction::LoseFocus, &fa, pPageView);
  m_bNotifying = false;
  return !!(*pAnnot);
}

bool CFFL_InteractiveFormFiller::IsVisible(CPDFSDK_Widget* pWidget) {
  return pWidget->IsVisible();
}

bool CFFL_InteractiveFormFiller::IsReadOnly(CPDFSDK_Widget* pWidget) {
  int nFieldFlags = pWidget->GetFieldFlags();
  return (nFieldFlags & FIELDFLAG_READONLY) == FIELDFLAG_READONLY;
}

bool CFFL_InteractiveFormFiller::IsFillingAllowed(CPDFSDK_Widget* pWidget) {
  if (pWidget->GetFieldType() == FormFieldType::kPushButton)
    return false;

  CPDF_Page* pPage = pWidget->GetPDFPage();
  uint32_t dwPermissions = pPage->GetDocument()->GetUserPermissions();
  return (dwPermissions & FPDFPERM_FILL_FORM) ||
         (dwPermissions & FPDFPERM_ANNOT_FORM) ||
         (dwPermissions & FPDFPERM_MODIFY);
}

CFFL_FormFiller* CFFL_InteractiveFormFiller::GetFormFiller(
    CPDFSDK_Annot* pAnnot,
    bool bRegister) {
  auto it = m_Maps.find(pAnnot);
  if (it != m_Maps.end())
    return it->second.get();

  if (!bRegister)
    return nullptr;

  // TODO(thestig): How do we know |pAnnot| is a CPDFSDK_Widget?
  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot);
  FormFieldType fieldType = pWidget->GetFieldType();
  CFFL_FormFiller* pFormFiller;
  switch (fieldType) {
    case FormFieldType::kPushButton:
      pFormFiller = new CFFL_PushButton(m_pFormFillEnv.Get(), pWidget);
      break;
    case FormFieldType::kCheckBox:
      pFormFiller = new CFFL_CheckBox(m_pFormFillEnv.Get(), pWidget);
      break;
    case FormFieldType::kRadioButton:
      pFormFiller = new CFFL_RadioButton(m_pFormFillEnv.Get(), pWidget);
      break;
    case FormFieldType::kTextField:
      pFormFiller = new CFFL_TextField(m_pFormFillEnv.Get(), pWidget);
      break;
    case FormFieldType::kListBox:
      pFormFiller = new CFFL_ListBox(m_pFormFillEnv.Get(), pWidget);
      break;
    case FormFieldType::kComboBox:
      pFormFiller = new CFFL_ComboBox(m_pFormFillEnv.Get(), pWidget);
      break;
    case FormFieldType::kUnknown:
    default:
      pFormFiller = nullptr;
      break;
  }

  if (!pFormFiller)
    return nullptr;

  m_Maps[pAnnot].reset(pFormFiller);
  return pFormFiller;
}

WideString CFFL_InteractiveFormFiller::GetText(CPDFSDK_Annot* pAnnot) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller ? pFormFiller->GetText(pAnnot) : WideString();
}

WideString CFFL_InteractiveFormFiller::GetSelectedText(CPDFSDK_Annot* pAnnot) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller ? pFormFiller->GetSelectedText(pAnnot) : WideString();
}

void CFFL_InteractiveFormFiller::ReplaceSelection(CPDFSDK_Annot* pAnnot,
                                                  const WideString& text) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  if (!pFormFiller)
    return;

  pFormFiller->ReplaceSelection(pAnnot, text);
}

bool CFFL_InteractiveFormFiller::CanUndo(CPDFSDK_Annot* pAnnot) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller && pFormFiller->CanUndo(pAnnot);
}

bool CFFL_InteractiveFormFiller::CanRedo(CPDFSDK_Annot* pAnnot) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller && pFormFiller->CanRedo(pAnnot);
}

bool CFFL_InteractiveFormFiller::Undo(CPDFSDK_Annot* pAnnot) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller && pFormFiller->Undo(pAnnot);
}

bool CFFL_InteractiveFormFiller::Redo(CPDFSDK_Annot* pAnnot) {
  ASSERT(pAnnot->GetPDFAnnot()->GetSubtype() == CPDF_Annot::Subtype::WIDGET);
  CFFL_FormFiller* pFormFiller = GetFormFiller(pAnnot, false);
  return pFormFiller && pFormFiller->Redo(pAnnot);
}

void CFFL_InteractiveFormFiller::UnRegisterFormFiller(CPDFSDK_Annot* pAnnot) {
  auto it = m_Maps.find(pAnnot);
  if (it == m_Maps.end())
    return;

  m_Maps.erase(it);
}

void CFFL_InteractiveFormFiller::QueryWherePopup(
    CPWL_Wnd::PrivateData* pAttached,
    float fPopupMin,
    float fPopupMax,
    bool* bBottom,
    float* fPopupRet) {
  auto* pData = static_cast<CFFL_PrivateData*>(pAttached);
  CPDFSDK_Widget* pWidget = pData->pWidget;
  CPDF_Page* pPage = pWidget->GetPDFPage();

  CFX_FloatRect rcPageView(0, pPage->GetPageHeight(), pPage->GetPageWidth(), 0);
  rcPageView.Normalize();

  CFX_FloatRect rcAnnot = pWidget->GetRect();
  float fTop = 0.0f;
  float fBottom = 0.0f;
  switch (pWidget->GetRotate() / 90) {
    default:
    case 0:
      fTop = rcPageView.top - rcAnnot.top;
      fBottom = rcAnnot.bottom - rcPageView.bottom;
      break;
    case 1:
      fTop = rcAnnot.left - rcPageView.left;
      fBottom = rcPageView.right - rcAnnot.right;
      break;
    case 2:
      fTop = rcAnnot.bottom - rcPageView.bottom;
      fBottom = rcPageView.top - rcAnnot.top;
      break;
    case 3:
      fTop = rcPageView.right - rcAnnot.right;
      fBottom = rcAnnot.left - rcPageView.left;
      break;
  }

  constexpr float kMaxListBoxHeight = 140;
  const float fMaxListBoxHeight =
      pdfium::clamp(kMaxListBoxHeight, fPopupMin, fPopupMax);

  if (fBottom > fMaxListBoxHeight) {
    *fPopupRet = fMaxListBoxHeight;
    *bBottom = true;
    return;
  }

  if (fTop > fMaxListBoxHeight) {
    *fPopupRet = fMaxListBoxHeight;
    *bBottom = false;
    return;
  }

  if (fTop > fBottom) {
    *fPopupRet = fTop;
    *bBottom = false;
  } else {
    *fPopupRet = fBottom;
    *bBottom = true;
  }
}

bool CFFL_InteractiveFormFiller::OnKeyStrokeCommit(
    CPDFSDK_Annot::ObservedPtr* pAnnot,
    CPDFSDK_PageView* pPageView,
    uint32_t nFlag) {
  if (m_bNotifying)
    return true;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->GetAAction(CPDF_AAction::KeyStroke).GetDict())
    return true;

  ASSERT(pPageView);
  m_bNotifying = true;
  pWidget->ClearAppModified();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
  fa.bWillCommit = true;
  fa.bKeyDown = true;
  fa.bRC = true;

  CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false);
  pFormFiller->GetActionData(pPageView, CPDF_AAction::KeyStroke, fa);
  pFormFiller->SaveState(pPageView);
  pWidget->OnAAction(CPDF_AAction::KeyStroke, &fa, pPageView);
  if (!(*pAnnot))
    return true;

  m_bNotifying = false;
  return fa.bRC;
}

bool CFFL_InteractiveFormFiller::OnValidate(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                            CPDFSDK_PageView* pPageView,
                                            uint32_t nFlag) {
  if (m_bNotifying)
    return true;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->GetAAction(CPDF_AAction::Validate).GetDict())
    return true;

  ASSERT(pPageView);
  m_bNotifying = true;
  pWidget->ClearAppModified();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
  fa.bKeyDown = true;
  fa.bRC = true;

  CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false);
  pFormFiller->GetActionData(pPageView, CPDF_AAction::Validate, fa);
  pFormFiller->SaveState(pPageView);
  pWidget->OnAAction(CPDF_AAction::Validate, &fa, pPageView);
  if (!(*pAnnot))
    return true;

  m_bNotifying = false;
  return fa.bRC;
}

void CFFL_InteractiveFormFiller::OnCalculate(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                             CPDFSDK_PageView* pPageView,
                                             uint32_t nFlag) {
  if (m_bNotifying)
    return;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (pWidget) {
    CPDFSDK_InterForm* pInterForm = pPageView->GetFormFillEnv()->GetInterForm();
    pInterForm->OnCalculate(pWidget->GetFormField());
  }
  m_bNotifying = false;
}

void CFFL_InteractiveFormFiller::OnFormat(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                          CPDFSDK_PageView* pPageView,
                                          uint32_t nFlag) {
  if (m_bNotifying)
    return;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  ASSERT(pWidget);
  CPDFSDK_InterForm* pInterForm = pPageView->GetFormFillEnv()->GetInterForm();

  bool bFormatted = false;
  WideString sValue = pInterForm->OnFormat(pWidget->GetFormField(), bFormatted);
  if (!(*pAnnot))
    return;

  if (bFormatted) {
    pInterForm->ResetFieldAppearance(pWidget->GetFormField(), &sValue, true);
    pInterForm->UpdateField(pWidget->GetFormField());
  }

  m_bNotifying = false;
}

#ifdef PDF_ENABLE_XFA
bool CFFL_InteractiveFormFiller::OnClick(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                         CPDFSDK_PageView* pPageView,
                                         uint32_t nFlag) {
  if (m_bNotifying)
    return false;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->HasXFAAAction(PDFSDK_XFA_Click))
    return false;

  m_bNotifying = true;
  uint32_t nAge = pWidget->GetAppearanceAge();
  uint32_t nValueAge = pWidget->GetValueAge();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);

  pWidget->OnXFAAAction(PDFSDK_XFA_Click, &fa, pPageView);
  m_bNotifying = false;
  if (!(*pAnnot) || !IsValidAnnot(pPageView, pWidget))
    return true;
  if (nAge == pWidget->GetAppearanceAge())
    return false;

  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false))
    pFormFiller->ResetPDFWindow(pPageView, nValueAge == pWidget->GetValueAge());
  return false;
}

bool CFFL_InteractiveFormFiller::OnFull(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                        CPDFSDK_PageView* pPageView,
                                        uint32_t nFlag) {
  if (m_bNotifying)
    return false;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->HasXFAAAction(PDFSDK_XFA_Full))
    return false;

  m_bNotifying = true;
  uint32_t nAge = pWidget->GetAppearanceAge();
  uint32_t nValueAge = pWidget->GetValueAge();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);

  pWidget->OnXFAAAction(PDFSDK_XFA_Full, &fa, pPageView);
  m_bNotifying = false;
  if (!(*pAnnot) || !IsValidAnnot(pPageView, pWidget))
    return true;
  if (nAge == pWidget->GetAppearanceAge())
    return false;

  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false))
    pFormFiller->ResetPDFWindow(pPageView, nValueAge == pWidget->GetValueAge());

  return true;
}

bool CFFL_InteractiveFormFiller::OnPopupPreOpen(
    CPWL_Wnd::PrivateData* pAttached,
    uint32_t nFlag) {
  auto* pData = static_cast<CFFL_PrivateData*>(pAttached);
  ASSERT(pData);
  ASSERT(pData->pWidget);

  CPDFSDK_Annot::ObservedPtr pObserved(pData->pWidget);
  return OnPreOpen(&pObserved, pData->pPageView, nFlag) || !pObserved;
}

bool CFFL_InteractiveFormFiller::OnPopupPostOpen(
    CPWL_Wnd::PrivateData* pAttached,
    uint32_t nFlag) {
  auto* pData = static_cast<CFFL_PrivateData*>(pAttached);
  ASSERT(pData);
  ASSERT(pData->pWidget);

  CPDFSDK_Annot::ObservedPtr pObserved(pData->pWidget);
  return OnPostOpen(&pObserved, pData->pPageView, nFlag) || !pObserved;
}

bool CFFL_InteractiveFormFiller::OnPreOpen(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                           CPDFSDK_PageView* pPageView,
                                           uint32_t nFlag) {
  if (m_bNotifying)
    return false;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->HasXFAAAction(PDFSDK_XFA_PreOpen))
    return false;

  m_bNotifying = true;
  uint32_t nAge = pWidget->GetAppearanceAge();
  uint32_t nValueAge = pWidget->GetValueAge();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);

  pWidget->OnXFAAAction(PDFSDK_XFA_PreOpen, &fa, pPageView);
  m_bNotifying = false;
  if (!(*pAnnot) || !IsValidAnnot(pPageView, pWidget))
    return true;
  if (nAge == pWidget->GetAppearanceAge())
    return false;

  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false))
    pFormFiller->ResetPDFWindow(pPageView, nValueAge == pWidget->GetValueAge());

  return true;
}

bool CFFL_InteractiveFormFiller::OnPostOpen(CPDFSDK_Annot::ObservedPtr* pAnnot,
                                            CPDFSDK_PageView* pPageView,
                                            uint32_t nFlag) {
  if (m_bNotifying)
    return false;

  CPDFSDK_Widget* pWidget = static_cast<CPDFSDK_Widget*>(pAnnot->Get());
  if (!pWidget->HasXFAAAction(PDFSDK_XFA_PostOpen))
    return false;

  m_bNotifying = true;
  uint32_t nAge = pWidget->GetAppearanceAge();
  uint32_t nValueAge = pWidget->GetValueAge();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);

  pWidget->OnXFAAAction(PDFSDK_XFA_PostOpen, &fa, pPageView);
  m_bNotifying = false;
  if (!(*pAnnot) || !IsValidAnnot(pPageView, pWidget))
    return true;
  if (nAge == pWidget->GetAppearanceAge())
    return false;

  if (CFFL_FormFiller* pFormFiller = GetFormFiller(pWidget, false))
    pFormFiller->ResetPDFWindow(pPageView, nValueAge == pWidget->GetValueAge());

  return true;
}
#endif  // PDF_ENABLE_XFA

bool CFFL_InteractiveFormFiller::IsValidAnnot(CPDFSDK_PageView* pPageView,
                                              CPDFSDK_Annot* pAnnot) {
  return pPageView && pPageView->IsValidAnnot(pAnnot->GetPDFAnnot());
}

std::pair<bool, bool> CFFL_InteractiveFormFiller::OnBeforeKeyStroke(
    CPWL_Wnd::PrivateData* pAttached,
    WideString& strChange,
    const WideString& strChangeEx,
    int nSelStart,
    int nSelEnd,
    bool bKeyDown,
    uint32_t nFlag) {
  // Copy the private data since the window owning it may not survive.
  CFFL_PrivateData privateData = *static_cast<CFFL_PrivateData*>(pAttached);
  ASSERT(privateData.pWidget);

  CFFL_FormFiller* pFormFiller = GetFormFiller(privateData.pWidget, false);

#ifdef PDF_ENABLE_XFA
  if (pFormFiller->IsFieldFull(privateData.pPageView)) {
    CPDFSDK_Annot::ObservedPtr pObserved(privateData.pWidget);
    if (OnFull(&pObserved, privateData.pPageView, nFlag) || !pObserved)
      return {true, true};
  }
#endif  // PDF_ENABLE_XFA

  if (m_bNotifying ||
      !privateData.pWidget->GetAAction(CPDF_AAction::KeyStroke).GetDict()) {
    return {true, false};
  }

  AutoRestorer<bool> restorer(&m_bNotifying);
  m_bNotifying = true;

  uint32_t nAge = privateData.pWidget->GetAppearanceAge();
  uint32_t nValueAge = privateData.pWidget->GetValueAge();
  CPDFSDK_FormFillEnvironment* pFormFillEnv =
      privateData.pPageView->GetFormFillEnv();

  CPDFSDK_FieldAction fa;
  fa.bModifier = CPDFSDK_FormFillEnvironment::IsCTRLKeyDown(nFlag);
  fa.bShift = CPDFSDK_FormFillEnvironment::IsSHIFTKeyDown(nFlag);
  fa.sChange = strChange;
  fa.sChangeEx = strChangeEx;
  fa.bKeyDown = bKeyDown;
  fa.bWillCommit = false;
  fa.bRC = true;
  fa.nSelStart = nSelStart;
  fa.nSelEnd = nSelEnd;
  pFormFiller->GetActionData(privateData.pPageView, CPDF_AAction::KeyStroke,
                             fa);
  pFormFiller->SaveState(privateData.pPageView);

  CPDFSDK_Annot::ObservedPtr pObserved(privateData.pWidget);
  bool action_status = privateData.pWidget->OnAAction(
      CPDF_AAction::KeyStroke, &fa, privateData.pPageView);

  if (!pObserved || !IsValidAnnot(privateData.pPageView, privateData.pWidget))
    return {true, true};

  if (!action_status)
    return {true, false};

  bool bExit = false;
  if (nAge != privateData.pWidget->GetAppearanceAge()) {
    CPWL_Wnd* pWnd = pFormFiller->ResetPDFWindow(
        privateData.pPageView, nValueAge == privateData.pWidget->GetValueAge());
    if (!pWnd)
      return {true, true};
    privateData = *static_cast<CFFL_PrivateData*>(pWnd->GetAttachedData());
    bExit = true;
  }
  if (fa.bRC) {
    pFormFiller->SetActionData(privateData.pPageView, CPDF_AAction::KeyStroke,
                               fa);
  } else {
    pFormFiller->RestoreState(privateData.pPageView);
  }
  if (pFormFillEnv->GetFocusAnnot() == privateData.pWidget)
    return {false, bExit};

  pFormFiller->CommitData(privateData.pPageView, nFlag);
  return {false, true};
}
