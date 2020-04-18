// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_DEFAULTFONTMANAGER_H_
#define XFA_FGAS_FONT_CFGAS_DEFAULTFONTMANAGER_H_

#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/retain_ptr.h"
#include "xfa/fgas/font/cfgas_gefont.h"

class CFGAS_DefaultFontManager {
 public:
  CFGAS_DefaultFontManager();
  ~CFGAS_DefaultFontManager();

  RetainPtr<CFGAS_GEFont> GetFont(CFGAS_FontMgr* pFontMgr,
                                  const WideStringView& wsFontFamily,
                                  uint32_t dwFontStyles);
  RetainPtr<CFGAS_GEFont> GetDefaultFont(CFGAS_FontMgr* pFontMgr,
                                         const WideStringView& wsFontFamily,
                                         uint32_t dwFontStyles);

 private:
  std::vector<RetainPtr<CFGAS_GEFont>> m_CacheFonts;
};

#endif  // XFA_FGAS_FONT_CFGAS_DEFAULTFONTMANAGER_H_
