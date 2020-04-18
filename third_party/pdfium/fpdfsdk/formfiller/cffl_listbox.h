// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FORMFILLER_CFFL_LISTBOX_H_
#define FPDFSDK_FORMFILLER_CFFL_LISTBOX_H_

#include <set>
#include <vector>

#include "fpdfsdk/formfiller/cffl_textobject.h"

class CBA_FontMap;

class CFFL_ListBox : public CFFL_TextObject {
 public:
  CFFL_ListBox(CPDFSDK_FormFillEnvironment* pApp, CPDFSDK_Widget* pWidget);
  ~CFFL_ListBox() override;

  // CFFL_TextObject:
  CPWL_Wnd::CreateParams GetCreateParam() override;
  CPWL_Wnd* NewPDFWindow(const CPWL_Wnd::CreateParams& cp) override;
  bool OnChar(CPDFSDK_Annot* pAnnot, uint32_t nChar, uint32_t nFlags) override;
  bool IsDataChanged(CPDFSDK_PageView* pPageView) override;
  void SaveData(CPDFSDK_PageView* pPageView) override;
  void GetActionData(CPDFSDK_PageView* pPageView,
                     CPDF_AAction::AActionType type,
                     CPDFSDK_FieldAction& fa) override;
  void SaveState(CPDFSDK_PageView* pPageView) override;
  void RestoreState(CPDFSDK_PageView* pPageView) override;

 private:
  std::set<int> m_OriginSelections;
  std::vector<int> m_State;
};

#endif  // FPDFSDK_FORMFILLER_CFFL_LISTBOX_H_
