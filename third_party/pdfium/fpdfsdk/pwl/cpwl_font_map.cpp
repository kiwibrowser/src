// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/pwl/cpwl_font_map.h"

#include <utility>

#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/font/cpdf_fontencoding.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fpdfdoc/ipvt_fontmap.h"
#include "core/fxcrt/fx_codepage.h"
#include "fpdfsdk/pwl/cpwl_wnd.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"

namespace {

const char kDefaultFontName[] = "Helvetica";

const char* const g_sDEStandardFontName[] = {"Courier",
                                             "Courier-Bold",
                                             "Courier-BoldOblique",
                                             "Courier-Oblique",
                                             "Helvetica",
                                             "Helvetica-Bold",
                                             "Helvetica-BoldOblique",
                                             "Helvetica-Oblique",
                                             "Times-Roman",
                                             "Times-Bold",
                                             "Times-Italic",
                                             "Times-BoldItalic",
                                             "Symbol",
                                             "ZapfDingbats"};

}  // namespace

CPWL_FontMap::CPWL_FontMap(CFX_SystemHandler* pSystemHandler)
    : m_pSystemHandler(pSystemHandler) {
  ASSERT(m_pSystemHandler);
}

CPWL_FontMap::~CPWL_FontMap() {
  Empty();
}

CPDF_Document* CPWL_FontMap::GetDocument() {
  if (!m_pPDFDoc) {
    if (CPDF_ModuleMgr::Get()) {
      m_pPDFDoc = pdfium::MakeUnique<CPDF_Document>(nullptr);
      m_pPDFDoc->CreateNewDoc();
    }
  }
  return m_pPDFDoc.get();
}

CPDF_Font* CPWL_FontMap::GetPDFFont(int32_t nFontIndex) {
  if (pdfium::IndexInBounds(m_Data, nFontIndex) && m_Data[nFontIndex])
    return m_Data[nFontIndex]->pFont;

  return nullptr;
}

ByteString CPWL_FontMap::GetPDFFontAlias(int32_t nFontIndex) {
  if (pdfium::IndexInBounds(m_Data, nFontIndex) && m_Data[nFontIndex])
    return m_Data[nFontIndex]->sFontName;

  return ByteString();
}

bool CPWL_FontMap::KnowWord(int32_t nFontIndex, uint16_t word) {
  return pdfium::IndexInBounds(m_Data, nFontIndex) && m_Data[nFontIndex] &&
         CharCodeFromUnicode(nFontIndex, word) >= 0;
}

int32_t CPWL_FontMap::GetWordFontIndex(uint16_t word,
                                       int32_t nCharset,
                                       int32_t nFontIndex) {
  if (nFontIndex > 0) {
    if (KnowWord(nFontIndex, word))
      return nFontIndex;
  } else {
    if (const CPWL_FontMap_Data* pData = GetFontMapData(0)) {
      if (nCharset == FX_CHARSET_Default ||
          pData->nCharset == FX_CHARSET_Symbol || nCharset == pData->nCharset) {
        if (KnowWord(0, word))
          return 0;
      }
    }
  }

  int32_t nNewFontIndex =
      GetFontIndex(GetNativeFontName(nCharset), nCharset, true);
  if (nNewFontIndex >= 0) {
    if (KnowWord(nNewFontIndex, word))
      return nNewFontIndex;
  }
  nNewFontIndex = GetFontIndex("Arial Unicode MS", FX_CHARSET_Default, false);
  if (nNewFontIndex >= 0) {
    if (KnowWord(nNewFontIndex, word))
      return nNewFontIndex;
  }
  return -1;
}

int32_t CPWL_FontMap::CharCodeFromUnicode(int32_t nFontIndex, uint16_t word) {
  if (!pdfium::IndexInBounds(m_Data, nFontIndex))
    return -1;

  CPWL_FontMap_Data* pData = m_Data[nFontIndex].get();
  if (!pData || !pData->pFont)
    return -1;

  if (pData->pFont->IsUnicodeCompatible())
    return pData->pFont->CharCodeFromUnicode(word);

  return word < 0xFF ? word : -1;
}

ByteString CPWL_FontMap::GetNativeFontName(int32_t nCharset) {
  for (const auto& pData : m_NativeFont) {
    if (pData && pData->nCharset == nCharset)
      return pData->sFontName;
  }

  ByteString sNew = GetNativeFont(nCharset);
  if (sNew.IsEmpty())
    return ByteString();

  auto pNewData = pdfium::MakeUnique<CPWL_FontMap_Native>();
  pNewData->nCharset = nCharset;
  pNewData->sFontName = sNew;
  m_NativeFont.push_back(std::move(pNewData));
  return sNew;
}

void CPWL_FontMap::Empty() {
  m_Data.clear();
  m_NativeFont.clear();
}

void CPWL_FontMap::Initialize() {
  GetFontIndex(kDefaultFontName, FX_CHARSET_ANSI, false);
}

bool CPWL_FontMap::IsStandardFont(const ByteString& sFontName) {
  for (const char* name : g_sDEStandardFontName) {
    if (sFontName == name)
      return true;
  }

  return false;
}

int32_t CPWL_FontMap::FindFont(const ByteString& sFontName, int32_t nCharset) {
  int32_t i = 0;
  for (const auto& pData : m_Data) {
    if (pData &&
        (nCharset == FX_CHARSET_Default || nCharset == pData->nCharset) &&
        (sFontName.IsEmpty() || pData->sFontName == sFontName)) {
      return i;
    }
    ++i;
  }
  return -1;
}

int32_t CPWL_FontMap::GetFontIndex(const ByteString& sFontName,
                                   int32_t nCharset,
                                   bool bFind) {
  int32_t nFontIndex = FindFont(EncodeFontAlias(sFontName, nCharset), nCharset);
  if (nFontIndex >= 0)
    return nFontIndex;

  ByteString sAlias;
  CPDF_Font* pFont = bFind ? FindFontSameCharset(&sAlias, nCharset) : nullptr;
  if (!pFont) {
    ByteString sTemp = sFontName;
    pFont = AddFontToDocument(GetDocument(), sTemp, nCharset);
    sAlias = EncodeFontAlias(sTemp, nCharset);
  }
  AddedFont(pFont, sAlias);
  return AddFontData(pFont, sAlias, nCharset);
}

CPDF_Font* CPWL_FontMap::FindFontSameCharset(ByteString* sFontAlias,
                                             int32_t nCharset) {
  return nullptr;
}

int32_t CPWL_FontMap::AddFontData(CPDF_Font* pFont,
                                  const ByteString& sFontAlias,
                                  int32_t nCharset) {
  auto pNewData = pdfium::MakeUnique<CPWL_FontMap_Data>();
  pNewData->pFont = pFont;
  pNewData->sFontName = sFontAlias;
  pNewData->nCharset = nCharset;
  m_Data.push_back(std::move(pNewData));
  return pdfium::CollectionSize<int32_t>(m_Data) - 1;
}

void CPWL_FontMap::AddedFont(CPDF_Font* pFont, const ByteString& sFontAlias) {}

ByteString CPWL_FontMap::GetNativeFont(int32_t nCharset) {
  if (nCharset == FX_CHARSET_Default)
    nCharset = GetNativeCharset();

  ByteString sFontName = GetDefaultFontByCharset(nCharset);
  if (!m_pSystemHandler->FindNativeTrueTypeFont(sFontName))
    return ByteString();

  return sFontName;
}

CPDF_Font* CPWL_FontMap::AddFontToDocument(CPDF_Document* pDoc,
                                           ByteString& sFontName,
                                           uint8_t nCharset) {
  if (IsStandardFont(sFontName))
    return AddStandardFont(pDoc, sFontName);

  return AddSystemFont(pDoc, sFontName, nCharset);
}

CPDF_Font* CPWL_FontMap::AddStandardFont(CPDF_Document* pDoc,
                                         ByteString& sFontName) {
  if (!pDoc)
    return nullptr;

  CPDF_Font* pFont = nullptr;

  if (sFontName == "ZapfDingbats") {
    pFont = pDoc->AddStandardFont(sFontName.c_str(), nullptr);
  } else {
    CPDF_FontEncoding fe(PDFFONT_ENCODING_WINANSI);
    pFont = pDoc->AddStandardFont(sFontName.c_str(), &fe);
  }

  return pFont;
}

CPDF_Font* CPWL_FontMap::AddSystemFont(CPDF_Document* pDoc,
                                       ByteString& sFontName,
                                       uint8_t nCharset) {
  if (!pDoc)
    return nullptr;

  if (sFontName.IsEmpty())
    sFontName = GetNativeFont(nCharset);
  if (nCharset == FX_CHARSET_Default)
    nCharset = GetNativeCharset();

  return m_pSystemHandler->AddNativeTrueTypeFontToPDF(pDoc, sFontName,
                                                      nCharset);
}

ByteString CPWL_FontMap::EncodeFontAlias(const ByteString& sFontName,
                                         int32_t nCharset) {
  return EncodeFontAlias(sFontName) + ByteString::Format("_%02X", nCharset);
}

ByteString CPWL_FontMap::EncodeFontAlias(const ByteString& sFontName) {
  ByteString sRet = sFontName;
  sRet.Remove(' ');
  return sRet;
}

const CPWL_FontMap_Data* CPWL_FontMap::GetFontMapData(int32_t nIndex) const {
  return pdfium::IndexInBounds(m_Data, nIndex) ? m_Data[nIndex].get() : nullptr;
}

int32_t CPWL_FontMap::GetNativeCharset() {
  uint8_t nCharset = FX_CHARSET_ANSI;
  int32_t iCodePage = FXSYS_GetACP();
  switch (iCodePage) {
    case FX_CODEPAGE_ShiftJIS:
      nCharset = FX_CHARSET_ShiftJIS;
      break;
    case FX_CODEPAGE_ChineseSimplified:
      nCharset = FX_CHARSET_ChineseSimplified;
      break;
    case FX_CODEPAGE_ChineseTraditional:
      nCharset = FX_CHARSET_ChineseTraditional;
      break;
    case FX_CODEPAGE_MSWin_WesternEuropean:
      nCharset = FX_CHARSET_ANSI;
      break;
    case FX_CODEPAGE_MSDOS_Thai:
      nCharset = FX_CHARSET_Thai;
      break;
    case FX_CODEPAGE_Hangul:
      nCharset = FX_CHARSET_Hangul;
      break;
    case FX_CODEPAGE_UTF16LE:
      nCharset = FX_CHARSET_ANSI;
      break;
    case FX_CODEPAGE_MSWin_EasternEuropean:
      nCharset = FX_CHARSET_MSWin_EasternEuropean;
      break;
    case FX_CODEPAGE_MSWin_Cyrillic:
      nCharset = FX_CHARSET_MSWin_Cyrillic;
      break;
    case FX_CODEPAGE_MSWin_Greek:
      nCharset = FX_CHARSET_MSWin_Greek;
      break;
    case FX_CODEPAGE_MSWin_Turkish:
      nCharset = FX_CHARSET_MSWin_Turkish;
      break;
    case FX_CODEPAGE_MSWin_Hebrew:
      nCharset = FX_CHARSET_MSWin_Hebrew;
      break;
    case FX_CODEPAGE_MSWin_Arabic:
      nCharset = FX_CHARSET_MSWin_Arabic;
      break;
    case FX_CODEPAGE_MSWin_Baltic:
      nCharset = FX_CHARSET_MSWin_Baltic;
      break;
    case FX_CODEPAGE_MSWin_Vietnamese:
      nCharset = FX_CHARSET_MSWin_Vietnamese;
      break;
    case FX_CODEPAGE_Johab:
      nCharset = FX_CHARSET_Johab;
      break;
  }
  return nCharset;
}

const FPDF_CharsetFontMap CPWL_FontMap::defaultTTFMap[] = {
    {FX_CHARSET_ANSI, "Helvetica"},
    {FX_CHARSET_ChineseSimplified, "SimSun"},
    {FX_CHARSET_ChineseTraditional, "MingLiU"},
    {FX_CHARSET_ShiftJIS, "MS Gothic"},
    {FX_CHARSET_Hangul, "Batang"},
    {FX_CHARSET_MSWin_Cyrillic, "Arial"},
#if _FX_PLATFORM_ == _FX_PLATFORM_LINUX_ || _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    {FX_CHARSET_MSWin_EasternEuropean, "Arial"},
#else
    {FX_CHARSET_MSWin_EasternEuropean, "Tahoma"},
#endif
    {FX_CHARSET_MSWin_Arabic, "Arial"},
    {-1, nullptr}};

ByteString CPWL_FontMap::GetDefaultFontByCharset(int32_t nCharset) {
  int i = 0;
  while (defaultTTFMap[i].charset != -1) {
    if (nCharset == defaultTTFMap[i].charset)
      return defaultTTFMap[i].fontname;
    ++i;
  }
  return "";
}

int32_t CPWL_FontMap::CharSetFromUnicode(uint16_t word, int32_t nOldCharset) {
  // to avoid CJK Font to show ASCII
  if (word < 0x7F)
    return FX_CHARSET_ANSI;
  // follow the old charset
  if (nOldCharset != FX_CHARSET_Default)
    return nOldCharset;

  // find new charset
  if ((word >= 0x4E00 && word <= 0x9FA5) ||
      (word >= 0xE7C7 && word <= 0xE7F3) ||
      (word >= 0x3000 && word <= 0x303F) ||
      (word >= 0x2000 && word <= 0x206F)) {
    return FX_CHARSET_ChineseSimplified;
  }

  if (((word >= 0x3040) && (word <= 0x309F)) ||
      ((word >= 0x30A0) && (word <= 0x30FF)) ||
      ((word >= 0x31F0) && (word <= 0x31FF)) ||
      ((word >= 0xFF00) && (word <= 0xFFEF))) {
    return FX_CHARSET_ShiftJIS;
  }

  if (((word >= 0xAC00) && (word <= 0xD7AF)) ||
      ((word >= 0x1100) && (word <= 0x11FF)) ||
      ((word >= 0x3130) && (word <= 0x318F))) {
    return FX_CHARSET_Hangul;
  }

  if (word >= 0x0E00 && word <= 0x0E7F)
    return FX_CHARSET_Thai;

  if ((word >= 0x0370 && word <= 0x03FF) || (word >= 0x1F00 && word <= 0x1FFF))
    return FX_CHARSET_MSWin_Greek;

  if ((word >= 0x0600 && word <= 0x06FF) || (word >= 0xFB50 && word <= 0xFEFC))
    return FX_CHARSET_MSWin_Arabic;

  if (word >= 0x0590 && word <= 0x05FF)
    return FX_CHARSET_MSWin_Hebrew;

  if (word >= 0x0400 && word <= 0x04FF)
    return FX_CHARSET_MSWin_Cyrillic;

  if (word >= 0x0100 && word <= 0x024F)
    return FX_CHARSET_MSWin_EasternEuropean;

  if (word >= 0x1E00 && word <= 0x1EFF)
    return FX_CHARSET_MSWin_Vietnamese;

  return FX_CHARSET_ANSI;
}
