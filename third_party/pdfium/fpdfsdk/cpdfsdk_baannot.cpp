// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_baannot.h"

#include <algorithm>
#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "fpdfsdk/cpdfsdk_datetime.h"
#include "fpdfsdk/cpdfsdk_pageview.h"

CPDFSDK_BAAnnot::CPDFSDK_BAAnnot(CPDF_Annot* pAnnot,
                                 CPDFSDK_PageView* pPageView)
    : CPDFSDK_Annot(pPageView), m_pAnnot(pAnnot) {}

CPDFSDK_BAAnnot::~CPDFSDK_BAAnnot() {}

CPDF_Annot* CPDFSDK_BAAnnot::GetPDFAnnot() const {
  return m_pAnnot.Get();
}

CPDF_Annot* CPDFSDK_BAAnnot::GetPDFPopupAnnot() const {
  return m_pAnnot->GetPopupAnnot();
}

CPDF_Dictionary* CPDFSDK_BAAnnot::GetAnnotDict() const {
  return m_pAnnot->GetAnnotDict();
}

CPDF_Dictionary* CPDFSDK_BAAnnot::GetAPDict() const {
  CPDF_Dictionary* pAPDict = m_pAnnot->GetAnnotDict()->GetDictFor("AP");
  if (!pAPDict)
    pAPDict = m_pAnnot->GetAnnotDict()->SetNewFor<CPDF_Dictionary>("AP");
  return pAPDict;
}

void CPDFSDK_BAAnnot::SetRect(const CFX_FloatRect& rect) {
  ASSERT(rect.right - rect.left >= GetMinWidth());
  ASSERT(rect.top - rect.bottom >= GetMinHeight());

  m_pAnnot->GetAnnotDict()->SetRectFor("Rect", rect);
}

CFX_FloatRect CPDFSDK_BAAnnot::GetRect() const {
  return m_pAnnot->GetRect();
}

CPDF_Annot::Subtype CPDFSDK_BAAnnot::GetAnnotSubtype() const {
  return m_pAnnot->GetSubtype();
}

void CPDFSDK_BAAnnot::DrawAppearance(CFX_RenderDevice* pDevice,
                                     const CFX_Matrix& mtUser2Device,
                                     CPDF_Annot::AppearanceMode mode,
                                     const CPDF_RenderOptions* pOptions) {
  m_pAnnot->DrawAppearance(m_pPageView->GetPDFPage(), pDevice, mtUser2Device,
                           mode, pOptions);
}

bool CPDFSDK_BAAnnot::IsAppearanceValid() {
  return !!m_pAnnot->GetAnnotDict()->GetDictFor("AP");
}

bool CPDFSDK_BAAnnot::IsAppearanceValid(CPDF_Annot::AppearanceMode mode) {
  CPDF_Dictionary* pAP = m_pAnnot->GetAnnotDict()->GetDictFor("AP");
  if (!pAP)
    return false;

  // Choose the right sub-ap
  const char* ap_entry = "N";
  if (mode == CPDF_Annot::Down)
    ap_entry = "D";
  else if (mode == CPDF_Annot::Rollover)
    ap_entry = "R";
  if (!pAP->KeyExist(ap_entry))
    ap_entry = "N";

  // Get the AP stream or subdirectory
  CPDF_Object* psub = pAP->GetDirectObjectFor(ap_entry);
  return !!psub;
}

void CPDFSDK_BAAnnot::SetAnnotName(const WideString& sName) {
  if (sName.IsEmpty()) {
    m_pAnnot->GetAnnotDict()->RemoveFor("NM");
  } else {
    m_pAnnot->GetAnnotDict()->SetNewFor<CPDF_String>(
        "NM", PDF_EncodeText(sName), false);
  }
}

WideString CPDFSDK_BAAnnot::GetAnnotName() const {
  return m_pAnnot->GetAnnotDict()->GetUnicodeTextFor("NM");
}

void CPDFSDK_BAAnnot::SetFlags(uint32_t nFlags) {
  m_pAnnot->GetAnnotDict()->SetNewFor<CPDF_Number>("F",
                                                   static_cast<int>(nFlags));
}

uint32_t CPDFSDK_BAAnnot::GetFlags() const {
  return m_pAnnot->GetAnnotDict()->GetIntegerFor("F");
}

void CPDFSDK_BAAnnot::SetAppState(const ByteString& str) {
  if (str.IsEmpty())
    m_pAnnot->GetAnnotDict()->RemoveFor("AS");
  else
    m_pAnnot->GetAnnotDict()->SetNewFor<CPDF_String>("AS", str, false);
}

ByteString CPDFSDK_BAAnnot::GetAppState() const {
  return m_pAnnot->GetAnnotDict()->GetStringFor("AS");
}

void CPDFSDK_BAAnnot::SetBorderWidth(int nWidth) {
  CPDF_Array* pBorder = m_pAnnot->GetAnnotDict()->GetArrayFor("Border");
  if (pBorder) {
    pBorder->SetNewAt<CPDF_Number>(2, nWidth);
  } else {
    CPDF_Dictionary* pBSDict = m_pAnnot->GetAnnotDict()->GetDictFor("BS");
    if (!pBSDict)
      pBSDict = m_pAnnot->GetAnnotDict()->SetNewFor<CPDF_Dictionary>("BS");

    pBSDict->SetNewFor<CPDF_Number>("W", nWidth);
  }
}

int CPDFSDK_BAAnnot::GetBorderWidth() const {
  if (CPDF_Array* pBorder = m_pAnnot->GetAnnotDict()->GetArrayFor("Border"))
    return pBorder->GetIntegerAt(2);

  if (CPDF_Dictionary* pBSDict = m_pAnnot->GetAnnotDict()->GetDictFor("BS"))
    return pBSDict->GetIntegerFor("W", 1);

  return 1;
}

void CPDFSDK_BAAnnot::SetBorderStyle(BorderStyle nStyle) {
  CPDF_Dictionary* pBSDict = m_pAnnot->GetAnnotDict()->GetDictFor("BS");
  if (!pBSDict)
    pBSDict = m_pAnnot->GetAnnotDict()->SetNewFor<CPDF_Dictionary>("BS");

  switch (nStyle) {
    case BorderStyle::SOLID:
      pBSDict->SetNewFor<CPDF_Name>("S", "S");
      break;
    case BorderStyle::DASH:
      pBSDict->SetNewFor<CPDF_Name>("S", "D");
      break;
    case BorderStyle::BEVELED:
      pBSDict->SetNewFor<CPDF_Name>("S", "B");
      break;
    case BorderStyle::INSET:
      pBSDict->SetNewFor<CPDF_Name>("S", "I");
      break;
    case BorderStyle::UNDERLINE:
      pBSDict->SetNewFor<CPDF_Name>("S", "U");
      break;
    default:
      break;
  }
}

BorderStyle CPDFSDK_BAAnnot::GetBorderStyle() const {
  CPDF_Dictionary* pBSDict = m_pAnnot->GetAnnotDict()->GetDictFor("BS");
  if (pBSDict) {
    ByteString sBorderStyle = pBSDict->GetStringFor("S", "S");
    if (sBorderStyle == "S")
      return BorderStyle::SOLID;
    if (sBorderStyle == "D")
      return BorderStyle::DASH;
    if (sBorderStyle == "B")
      return BorderStyle::BEVELED;
    if (sBorderStyle == "I")
      return BorderStyle::INSET;
    if (sBorderStyle == "U")
      return BorderStyle::UNDERLINE;
  }

  CPDF_Array* pBorder = m_pAnnot->GetAnnotDict()->GetArrayFor("Border");
  if (pBorder) {
    if (pBorder->GetCount() >= 4) {
      CPDF_Array* pDP = pBorder->GetArrayAt(3);
      if (pDP && pDP->GetCount() > 0)
        return BorderStyle::DASH;
    }
  }

  return BorderStyle::SOLID;
}

bool CPDFSDK_BAAnnot::IsVisible() const {
  uint32_t nFlags = GetFlags();
  return !((nFlags & ANNOTFLAG_INVISIBLE) || (nFlags & ANNOTFLAG_HIDDEN) ||
           (nFlags & ANNOTFLAG_NOVIEW));
}

CPDF_Action CPDFSDK_BAAnnot::GetAction() const {
  return CPDF_Action(m_pAnnot->GetAnnotDict()->GetDictFor("A"));
}

CPDF_AAction CPDFSDK_BAAnnot::GetAAction() const {
  return CPDF_AAction(m_pAnnot->GetAnnotDict()->GetDictFor("AA"));
}

CPDF_Action CPDFSDK_BAAnnot::GetAAction(CPDF_AAction::AActionType eAAT) {
  CPDF_AAction AAction = GetAAction();
  if (AAction.ActionExist(eAAT))
    return AAction.GetAction(eAAT);

  if (eAAT == CPDF_AAction::ButtonUp)
    return GetAction();

  return CPDF_Action(nullptr);
}

void CPDFSDK_BAAnnot::SetOpenState(bool bOpenState) {
  if (CPDF_Annot* pAnnot = m_pAnnot->GetPopupAnnot())
    pAnnot->SetOpenState(bOpenState);
}

int CPDFSDK_BAAnnot::GetLayoutOrder() const {
  if (m_pAnnot->GetSubtype() == CPDF_Annot::Subtype::POPUP)
    return 1;

  return CPDFSDK_Annot::GetLayoutOrder();
}
