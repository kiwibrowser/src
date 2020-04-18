// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/font/cfgas_defaultfontmanager.h"

#include "xfa/fgas/font/fgas_fontutils.h"

CFGAS_DefaultFontManager::CFGAS_DefaultFontManager() {}

CFGAS_DefaultFontManager::~CFGAS_DefaultFontManager() {}

RetainPtr<CFGAS_GEFont> CFGAS_DefaultFontManager::GetFont(
    CFGAS_FontMgr* pFontMgr,
    const WideStringView& wsFontFamily,
    uint32_t dwFontStyles) {
  WideString wsFontName(wsFontFamily);
  RetainPtr<CFGAS_GEFont> pFont =
      pFontMgr->LoadFont(wsFontName.c_str(), dwFontStyles, 0xFFFF);
  if (!pFont) {
    const FGAS_FontInfo* pCurFont =
        FGAS_FontInfoByFontName(wsFontName.AsStringView());
    if (pCurFont && pCurFont->pReplaceFont) {
      uint32_t dwStyle = 0;
      // TODO(dsinclair): Why doesn't this check the other flags?
      if (FontStyleIsBold(dwFontStyles))
        dwStyle |= FXFONT_BOLD;
      if (FontStyleIsItalic(dwFontStyles))
        dwStyle |= FXFONT_ITALIC;

      const wchar_t* pReplace = pCurFont->pReplaceFont;
      int32_t iLength = wcslen(pReplace);
      while (iLength > 0) {
        const wchar_t* pNameText = pReplace;
        while (*pNameText != L',' && iLength > 0) {
          pNameText++;
          iLength--;
        }
        WideString wsReplace = WideString(pReplace, pNameText - pReplace);
        pFont = pFontMgr->LoadFont(wsReplace.c_str(), dwStyle, 0xFFFF);
        if (pFont)
          break;

        iLength--;
        pNameText++;
        pReplace = pNameText;
      }
    }
  }
  if (pFont)
    m_CacheFonts.push_back(pFont);
  return pFont;
}

RetainPtr<CFGAS_GEFont> CFGAS_DefaultFontManager::GetDefaultFont(
    CFGAS_FontMgr* pFontMgr,
    const WideStringView& wsFontFamily,
    uint32_t dwFontStyles) {
  RetainPtr<CFGAS_GEFont> pFont =
      pFontMgr->LoadFont(L"Arial Narrow", dwFontStyles, 0xFFFF);
  if (!pFont) {
    pFont = pFontMgr->LoadFont(static_cast<const wchar_t*>(nullptr),
                               dwFontStyles, 0xFFFF);
  }
  if (pFont)
    m_CacheFonts.push_back(pFont);
  return pFont;
}
