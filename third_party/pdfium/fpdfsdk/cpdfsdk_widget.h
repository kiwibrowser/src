// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_CPDFSDK_WIDGET_H_
#define FPDFSDK_CPDFSDK_WIDGET_H_

#include <set>

#include "core/fpdfdoc/cpdf_aaction.h"
#include "core/fpdfdoc/cpdf_action.h"
#include "core/fpdfdoc/cpdf_annot.h"
#include "core/fpdfdoc/cpdf_formfield.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_color.h"
#include "fpdfsdk/cpdfsdk_baannot.h"
#include "fpdfsdk/cpdfsdk_fieldaction.h"

class CFX_RenderDevice;
class CPDF_Annot;
class CPDF_Dictionary;
class CPDF_FormControl;
class CPDF_FormField;
class CPDF_RenderOptions;
class CPDF_Stream;
class CPDFSDK_InterForm;
class CPDFSDK_PageView;

#ifdef PDF_ENABLE_XFA
class CXFA_FFWidget;
class CXFA_FFWidgetHandler;
#endif  // PDF_ENABLE_XFA

class CPDFSDK_Widget : public CPDFSDK_BAAnnot {
 public:
#ifdef PDF_ENABLE_XFA
  CXFA_FFWidget* GetMixXFAWidget() const;
  CXFA_FFWidgetHandler* GetXFAWidgetHandler() const;

  bool HasXFAAAction(PDFSDK_XFAAActionType eXFAAAT);
  bool OnXFAAAction(PDFSDK_XFAAActionType eXFAAAT,
                    CPDFSDK_FieldAction* data,
                    CPDFSDK_PageView* pPageView);

  void Synchronize(bool bSynchronizeElse);
#endif  // PDF_ENABLE_XFA

  CPDFSDK_Widget(CPDF_Annot* pAnnot,
                 CPDFSDK_PageView* pPageView,
                 CPDFSDK_InterForm* pInterForm);
  ~CPDFSDK_Widget() override;

  bool IsSignatureWidget() const override;
  CPDF_Action GetAAction(CPDF_AAction::AActionType eAAT) override;
  bool IsAppearanceValid() override;

  int GetLayoutOrder() const override;

  FormFieldType GetFieldType() const;
  int GetFieldFlags() const;
  int GetRotate() const;

  bool GetFillColor(FX_COLORREF& color) const;
  bool GetBorderColor(FX_COLORREF& color) const;
  bool GetTextColor(FX_COLORREF& color) const;
  float GetFontSize() const;

  int GetSelectedIndex(int nIndex) const;
#ifndef PDF_ENABLE_XFA
  WideString GetValue() const;
#else
  WideString GetValue(bool bDisplay = true) const;
#endif  // PDF_ENABLE_XFA
  WideString GetDefaultValue() const;
  WideString GetOptionLabel(int nIndex) const;
  int CountOptions() const;
  bool IsOptionSelected(int nIndex) const;
  int GetTopVisibleIndex() const;
  bool IsChecked() const;
  int GetAlignment() const;
  int GetMaxLen() const;
  WideString GetAlternateName() const;

  void SetCheck(bool bChecked, bool bNotify);
  void SetValue(const WideString& sValue, bool bNotify);
  void SetOptionSelection(int index, bool bSelected, bool bNotify);
  void ClearSelection(bool bNotify);
  void SetTopVisibleIndex(int index);

#ifdef PDF_ENABLE_XFA
  void ResetAppearance(bool bValueChanged);
#endif  // PDF_ENABLE_XFA
  void ResetAppearance(const WideString* sValue, bool bValueChanged);
  void ResetFieldAppearance(bool bValueChanged);
  void UpdateField();
  WideString OnFormat(bool& bFormatted);

  bool OnAAction(CPDF_AAction::AActionType type,
                 CPDFSDK_FieldAction* data,
                 CPDFSDK_PageView* pPageView);

  CPDFSDK_InterForm* GetInterForm() const { return m_pInterForm.Get(); }
  CPDF_FormField* GetFormField() const;
  CPDF_FormControl* GetFormControl() const;
  static CPDF_FormControl* GetFormControl(CPDF_InterForm* pInterForm,
                                          const CPDF_Dictionary* pAnnotDict);

  void DrawShadow(CFX_RenderDevice* pDevice, CPDFSDK_PageView* pPageView);

  void SetAppModified();
  void ClearAppModified();
  bool IsAppModified() const;

  uint32_t GetAppearanceAge() const { return m_nAppearanceAge; }
  uint32_t GetValueAge() const { return m_nValueAge; }

  bool IsWidgetAppearanceValid(CPDF_Annot::AppearanceMode mode);
  void DrawAppearance(CFX_RenderDevice* pDevice,
                      const CFX_Matrix& mtUser2Device,
                      CPDF_Annot::AppearanceMode mode,
                      const CPDF_RenderOptions* pOptions) override;

  CFX_Matrix GetMatrix() const;
  CFX_FloatRect GetClientRect() const;
  CFX_FloatRect GetRotatedRect() const;
  CFX_Color GetTextPWLColor() const;
  CFX_Color GetBorderPWLColor() const;
  CFX_Color GetFillPWLColor() const;

 private:
#ifdef PDF_ENABLE_XFA
  CXFA_FFWidget* GetGroupMixXFAWidget();
  WideString GetName() const;
#endif  // PDF_ENABLE_XFA

  UnownedPtr<CPDFSDK_InterForm> const m_pInterForm;
  bool m_bAppModified;
  uint32_t m_nAppearanceAge;
  uint32_t m_nValueAge;

#ifdef PDF_ENABLE_XFA
  mutable UnownedPtr<CXFA_FFWidget> m_hMixXFAWidget;
  mutable UnownedPtr<CXFA_FFWidgetHandler> m_pWidgetHandler;
#endif  // PDF_ENABLE_XFA
};

#endif  // FPDFSDK_CPDFSDK_WIDGET_H_
