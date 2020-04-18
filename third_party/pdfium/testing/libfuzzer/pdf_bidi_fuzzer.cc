// Copyright 2018 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "core/fxcrt/fx_bidi.h"
#include "core/fxcrt/widestring.h"
#include "core/fxge/cfx_font.h"
#include "third_party/base/span.h"
#include "xfa/fgas/font/cfgas_fontmgr.h"
#include "xfa/fgas/font/cfgas_gefont.h"
#include "xfa/fgas/layout/cfx_rtfbreak.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  auto fontmgr = pdfium::MakeUnique<CFGAS_FontMgr>();

  auto font = pdfium::MakeUnique<CFX_Font>();
  font->LoadSubst("Arial", true, 0, FXFONT_FW_NORMAL, 0, 0, 0);
  assert(font);

  CFX_RTFBreak rtf_break(FX_LAYOUTSTYLE_ExpandTab);
  rtf_break.SetLineBreakTolerance(1);
  rtf_break.SetFont(CFGAS_GEFont::LoadFont(std::move(font), fontmgr.get()));
  rtf_break.SetFontSize(12);

  WideString input =
      WideString::FromUTF16LE(reinterpret_cast<const unsigned short*>(data),
                              size / sizeof(unsigned short));
  for (auto& ch : input)
    rtf_break.AppendChar(ch);

  auto chars = rtf_break.GetCurrentLineForTesting()->m_LineChars;
  FX_BidiLine(&chars, chars.size());
  return 0;
}
