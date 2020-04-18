// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_ONED_BC_ONEDIMWRITER_H_
#define FXBARCODE_ONED_BC_ONEDIMWRITER_H_

#include <memory>
#include <vector>

#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_renderdevice.h"
#include "fxbarcode/BC_Library.h"
#include "fxbarcode/BC_Writer.h"

class CFX_Font;
class CFX_PathData;
class CFX_RenderDevice;

class CBC_OneDimWriter : public CBC_Writer {
 public:
  CBC_OneDimWriter();
  ~CBC_OneDimWriter() override;

  virtual bool RenderResult(const WideStringView& contents,
                            uint8_t* code,
                            int32_t codeLength);
  virtual bool CheckContentValidity(const WideStringView& contents) = 0;
  virtual WideString FilterContents(const WideStringView& contents) = 0;
  virtual WideString RenderTextContents(const WideStringView& contents);
  virtual void SetPrintChecksum(bool checksum);
  virtual void SetDataLength(int32_t length);
  virtual void SetCalcChecksum(bool state);
  virtual void SetFontSize(float size);
  virtual void SetFontStyle(int32_t style);
  virtual void SetFontColor(FX_ARGB color);

  uint8_t* Encode(const ByteString& contents,
                  BCFORMAT format,
                  int32_t& outWidth,
                  int32_t& outHeight);
  bool RenderDeviceResult(CFX_RenderDevice* device,
                          const CFX_Matrix* matrix,
                          const WideStringView& contents);
  bool SetFont(CFX_Font* cFont);

 protected:
  virtual uint8_t* EncodeWithHint(const ByteString& contents,
                                  BCFORMAT format,
                                  int32_t& outWidth,
                                  int32_t& outHeight,
                                  int32_t hints);
  virtual uint8_t* EncodeImpl(const ByteString& contents,
                              int32_t& outLength) = 0;
  virtual void CalcTextInfo(const ByteString& text,
                            FXTEXT_CHARPOS* charPos,
                            CFX_Font* cFont,
                            float geWidth,
                            int32_t fontSize,
                            float& charsLen);
  virtual bool ShowChars(const WideStringView& contents,
                         CFX_RenderDevice* device,
                         const CFX_Matrix* matrix,
                         int32_t barWidth,
                         int32_t multiple);
  virtual void ShowDeviceChars(CFX_RenderDevice* device,
                               const CFX_Matrix* matrix,
                               const ByteString str,
                               float geWidth,
                               FXTEXT_CHARPOS* pCharPos,
                               float locX,
                               float locY,
                               int32_t barWidth);
  virtual int32_t AppendPattern(uint8_t* target,
                                int32_t pos,
                                const int8_t* pattern,
                                int32_t patternLength,
                                int32_t startColor,
                                int32_t& e);

  wchar_t Upper(wchar_t ch);
  void RenderVerticalBars(int32_t outputX, int32_t width, int32_t height);

  bool m_bPrintChecksum;
  int32_t m_iDataLenth;
  bool m_bCalcChecksum;
  UnownedPtr<CFX_Font> m_pFont;
  float m_fFontSize;
  int32_t m_iFontStyle;
  uint32_t m_fontColor;
  BC_TEXT_LOC m_locTextLoc;
  size_t m_iContentLen;
  bool m_bLeftPadding;
  bool m_bRightPadding;
  std::vector<CFX_PathData> m_output;
  int32_t m_barWidth;
  int32_t m_multiple;
  float m_outputHScale;
};

#endif  // FXBARCODE_ONED_BC_ONEDIMWRITER_H_
