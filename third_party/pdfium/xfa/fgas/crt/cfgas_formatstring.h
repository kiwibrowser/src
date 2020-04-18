// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_CRT_CFGAS_FORMATSTRING_H_
#define XFA_FGAS_CRT_CFGAS_FORMATSTRING_H_

#include <vector>

#include "core/fxcrt/locale_iface.h"
#include "xfa/fxfa/parser/cxfa_localemgr.h"

bool FX_DateFromCanonical(const WideString& wsDate, CFX_DateTime* datetime);
bool FX_TimeFromCanonical(const WideStringView& wsTime,
                          CFX_DateTime* datetime,
                          LocaleIface* pLocale);

class CFGAS_FormatString {
 public:
  explicit CFGAS_FormatString(CXFA_LocaleMgr* pLocaleMgr);
  ~CFGAS_FormatString();

  void SplitFormatString(const WideString& wsFormatString,
                         std::vector<WideString>* wsPatterns);
  FX_LOCALECATEGORY GetCategory(const WideString& wsPattern);

  bool ParseText(const WideString& wsSrcText,
                 const WideString& wsPattern,
                 WideString* wsValue);
  bool ParseNum(const WideString& wsSrcNum,
                const WideString& wsPattern,
                WideString* wsValue);
  bool ParseDateTime(const WideString& wsSrcDateTime,
                     const WideString& wsPattern,
                     FX_DATETIMETYPE eDateTimeType,
                     CFX_DateTime* dtValue);
  bool ParseZero(const WideString& wsSrcText, const WideString& wsPattern);
  bool ParseNull(const WideString& wsSrcText, const WideString& wsPattern);

  bool FormatText(const WideString& wsSrcText,
                  const WideString& wsPattern,
                  WideString* wsOutput);
  bool FormatNum(const WideString& wsSrcNum,
                 const WideString& wsPattern,
                 WideString* wsOutput);
  bool FormatDateTime(const WideString& wsSrcDateTime,
                      const WideString& wsPattern,
                      FX_DATETIMETYPE eDateTimeType,
                      WideString* wsOutput);
  bool FormatZero(const WideString& wsPattern, WideString* wsOutput);
  bool FormatNull(const WideString& wsPattern, WideString* wsOutput);

 private:
  WideString GetTextFormat(const WideString& wsPattern,
                           const WideStringView& wsCategory);
  LocaleIface* GetNumericFormat(const WideString& wsPattern,
                                int32_t* iDotIndex,
                                uint32_t* dwStyle,
                                WideString* wsPurgePattern);
  bool FormatStrNum(const WideStringView& wsInputNum,
                    const WideString& wsPattern,
                    WideString* wsOutput);
  FX_DATETIMETYPE GetDateTimeFormat(const WideString& wsPattern,
                                    LocaleIface** pLocale,
                                    WideString* wsDatePattern,
                                    WideString* wsTimePattern);

  CXFA_LocaleMgr* m_pLocaleMgr;
};

#endif  // XFA_FGAS_CRT_CFGAS_FORMATSTRING_H_
