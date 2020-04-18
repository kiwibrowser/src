// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_LAYOUT_CFX_BREAK_H_
#define XFA_FGAS_LAYOUT_CFX_BREAK_H_

#include <stdint.h>

#include "core/fxcrt/retain_ptr.h"
#include "xfa/fgas/layout/cfx_breakline.h"

class CFGAS_GEFont;

struct FX_TPO {
  bool operator<(const FX_TPO& that) const { return pos < that.pos; }

  int32_t index;
  int32_t pos;
};

enum FX_LAYOUTSTYLE {
  FX_LAYOUTSTYLE_None = 0,
  FX_LAYOUTSTYLE_Pagination = 0x01,
  FX_LAYOUTSTYLE_ExpandTab = 0x10,
  FX_LAYOUTSTYLE_SingleLine = 0x200,
  FX_LAYOUTSTYLE_CombText = 0x400
};

class CFX_Break {
 public:
  virtual ~CFX_Break();

  void Reset();

  void SetLayoutStyles(uint32_t dwLayoutStyles);
  uint32_t GetLayoutStyles() const { return m_dwLayoutStyles; }

  void SetFont(const RetainPtr<CFGAS_GEFont>& pFont);
  void SetFontSize(float fFontSize);
  void SetTabWidth(float fTabWidth);
  int32_t GetTabWidth() const { return m_iTabWidth; }

  void SetHorizontalScale(int32_t iScale);
  void SetVerticalScale(int32_t iScale);
  void SetLineBreakTolerance(float fTolerance);
  void SetLineBoundary(float fLineStart, float fLineEnd);

  void SetCharSpace(float fCharSpace);
  void SetParagraphBreakChar(wchar_t wch);
  void SetDefaultChar(wchar_t wch);

  int32_t CountBreakPieces() const;
  const CFX_BreakPiece* GetBreakPieceUnstable(int32_t index) const;
  void ClearBreakPieces();

  CFX_Char* GetLastChar(int32_t index, bool bOmitChar, bool bRichText) const;
  const CFX_BreakLine* GetCurrentLineForTesting() const { return m_pCurLine; }

 protected:
  explicit CFX_Break(uint32_t dwLayoutStyles);

  void SetBreakStatus();
  bool HasLine() const { return m_iReadyLineIndex >= 0; }
  FX_CHARTYPE GetUnifiedCharType(FX_CHARTYPE dwType) const;

  FX_CHARTYPE m_eCharType;
  bool m_bSingleLine;
  bool m_bCombText;
  uint32_t m_dwIdentity;
  uint32_t m_dwLayoutStyles;
  int32_t m_iLineStart;
  int32_t m_iLineWidth;
  wchar_t m_wParagraphBreakChar;
  int32_t m_iFontSize;
  int32_t m_iTabWidth;
  int32_t m_iHorizontalScale;
  int32_t m_iVerticalScale;
  int32_t m_iTolerance;
  int32_t m_iCharSpace;
  int32_t m_iDefChar;
  wchar_t m_wDefChar;
  RetainPtr<CFGAS_GEFont> m_pFont;
  CFX_BreakLine* m_pCurLine;
  int8_t m_iReadyLineIndex;
  CFX_BreakLine m_Line[2];

 private:
  void FontChanged();
};

#endif  // XFA_FGAS_LAYOUT_CFX_BREAK_H_
