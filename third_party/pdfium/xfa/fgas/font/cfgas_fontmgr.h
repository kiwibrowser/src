// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_FONTMGR_H_
#define XFA_FGAS_FONT_CFGAS_FONTMGR_H_

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/observable.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/fx_freetype.h"
#include "core/fxge/systemfontinfo_iface.h"
#include "xfa/fgas/font/cfgas_pdffontmgr.h"

class CFGAS_GEFont;
class CFX_FontMapper;
class CFX_FontSourceEnum_File;

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
struct FX_FONTMATCHPARAMS {
  const wchar_t* pwsFamily;
  uint32_t dwFontStyles;
  uint32_t dwUSB;
  bool matchParagraphStyle;
  wchar_t wUnicode;
  uint16_t wCodePage;
};

struct FX_FONTSIGNATURE {
  uint32_t fsUsb[4];
  uint32_t fsCsb[2];
};

inline bool operator==(const FX_FONTSIGNATURE& left,
                       const FX_FONTSIGNATURE& right) {
  return left.fsUsb[0] == right.fsUsb[0] && left.fsUsb[1] == right.fsUsb[1] &&
         left.fsUsb[2] == right.fsUsb[2] && left.fsUsb[3] == right.fsUsb[3] &&
         left.fsCsb[0] == right.fsCsb[0] && left.fsCsb[1] == right.fsCsb[1];
}

struct FX_FONTDESCRIPTOR {
  wchar_t wsFontFace[32];
  uint32_t dwFontStyles;
  uint8_t uCharSet;
  FX_FONTSIGNATURE FontSignature;
};

inline bool operator==(const FX_FONTDESCRIPTOR& left,
                       const FX_FONTDESCRIPTOR& right) {
  return left.uCharSet == right.uCharSet &&
         left.dwFontStyles == right.dwFontStyles &&
         left.FontSignature == right.FontSignature &&
         wcscmp(left.wsFontFace, right.wsFontFace) == 0;
}

#else  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_

class CFX_FontDescriptor {
 public:
  CFX_FontDescriptor();
  ~CFX_FontDescriptor();

  int32_t m_nFaceIndex;
  uint32_t m_dwFontStyles;
  WideString m_wsFaceName;
  std::vector<WideString> m_wsFamilyNames;
  uint32_t m_dwUsb[4];
  uint32_t m_dwCsb[2];
};

class CFX_FontDescriptorInfo {
 public:
  CFX_FontDescriptor* pFont;
  int32_t nPenalty;

  bool operator>(const CFX_FontDescriptorInfo& other) const {
    return nPenalty > other.nPenalty;
  }
  bool operator<(const CFX_FontDescriptorInfo& other) const {
    return nPenalty < other.nPenalty;
  }
  bool operator==(const CFX_FontDescriptorInfo& other) const {
    return nPenalty == other.nPenalty;
  }
};

class CFX_FontSourceEnum_File {
 public:
  CFX_FontSourceEnum_File();
  ~CFX_FontSourceEnum_File();

  bool HasStartPosition();
  std::pair<bool, RetainPtr<IFX_SeekableStream>> GetNext();

 private:
  struct HandleParentPath {
    HandleParentPath() = default;
    HandleParentPath(const HandleParentPath& x) {
      pFileHandle = x.pFileHandle;
      bsParentPath = x.bsParentPath;
    }
    FX_FileHandle* pFileHandle;
    ByteString bsParentPath;
  };

  ByteString GetNextFile();

  WideString m_wsNext;
  std::vector<HandleParentPath> m_FolderQueue;
  std::vector<ByteString> m_FolderPaths;
};

#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_

class CFGAS_FontMgr : public Observable<CFGAS_FontMgr> {
 public:
  CFGAS_FontMgr();
  ~CFGAS_FontMgr();

  RetainPtr<CFGAS_GEFont> GetFontByCodePage(uint16_t wCodePage,
                                            uint32_t dwFontStyles,
                                            const wchar_t* pszFontFamily);
  RetainPtr<CFGAS_GEFont> GetFontByUnicode(wchar_t wUnicode,
                                           uint32_t dwFontStyles,
                                           const wchar_t* pszFontFamily);
  RetainPtr<CFGAS_GEFont> LoadFont(const wchar_t* pszFontFamily,
                                   uint32_t dwFontStyles,
                                   uint16_t wCodePage);
  void RemoveFont(const RetainPtr<CFGAS_GEFont>& pFont);

  bool EnumFonts();

 private:
#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
  const FX_FONTDESCRIPTOR* FindFont(const wchar_t* pszFontFamily,
                                    uint32_t dwFontStyles,
                                    bool matchParagraphStyle,
                                    uint16_t wCodePage,
                                    uint32_t dwUSB,
                                    wchar_t wUnicode);

  std::deque<FX_FONTDESCRIPTOR> m_FontFaces;
#else   // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
  bool EnumFontsFromFontMapper();
  bool EnumFontsFromFiles();
  void RegisterFace(FXFT_Face pFace, const WideString* pFaceName);
  void RegisterFaces(const RetainPtr<IFX_SeekableReadStream>& pFontStream,
                     const WideString* pFaceName);
  void GetNames(const uint8_t* name_table, std::vector<WideString>& Names);
  std::vector<uint16_t> GetCharsets(FXFT_Face pFace) const;
  void GetUSBCSB(FXFT_Face pFace, uint32_t* USB, uint32_t* CSB);
  uint32_t GetFlags(FXFT_Face pFace);
  bool VerifyUnicode(CFX_FontDescriptor* pDesc, wchar_t wcUnicode);
  int32_t IsPartName(const WideString& Name1, const WideString& Name2);
  void MatchFonts(std::vector<CFX_FontDescriptorInfo>* MatchedFonts,
                  uint16_t wCodePage,
                  uint32_t dwFontStyles,
                  const WideString& FontName,
                  wchar_t wcUnicode = 0xFFFE);
  int32_t CalcPenalty(CFX_FontDescriptor* pInstalled,
                      uint16_t wCodePage,
                      uint32_t dwFontStyles,
                      const WideString& FontName,
                      wchar_t wcUnicode = 0xFFFE);
  RetainPtr<CFGAS_GEFont> LoadFont(const WideString& wsFaceName,
                                   int32_t iFaceIndex,
                                   int32_t* pFaceCount);
  FXFT_Face LoadFace(const RetainPtr<IFX_SeekableReadStream>& pFontStream,
                     int32_t iFaceIndex);
  RetainPtr<IFX_SeekableReadStream> CreateFontStream(
      CFX_FontMapper* pFontMapper,
      SystemFontInfoIface* pSystemFontInfo,
      uint32_t index);
  RetainPtr<IFX_SeekableReadStream> CreateFontStream(
      const ByteString& bsFaceName);

  std::unique_ptr<CFX_FontSourceEnum_File> m_pFontSource;
  std::vector<std::unique_ptr<CFX_FontDescriptor>> m_InstalledFonts;
  std::map<uint32_t, std::unique_ptr<std::vector<CFX_FontDescriptorInfo>>>
      m_Hash2CandidateList;
  std::map<RetainPtr<CFGAS_GEFont>, RetainPtr<IFX_SeekableReadStream>>
      m_IFXFont2FileRead;
  std::set<wchar_t> m_FailedUnicodesSet;
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_

  bool VerifyUnicode(const RetainPtr<CFGAS_GEFont>& pFont, wchar_t wcUnicode);

  std::map<uint32_t, std::vector<RetainPtr<CFGAS_GEFont>>> m_Hash2Fonts;
};

#endif  // XFA_FGAS_FONT_CFGAS_FONTMGR_H_
