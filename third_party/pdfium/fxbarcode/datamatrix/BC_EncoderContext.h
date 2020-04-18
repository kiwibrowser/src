// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_ENCODERCONTEXT_H_
#define FXBARCODE_DATAMATRIX_BC_ENCODERCONTEXT_H_

#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/widestring.h"

class CBC_SymbolInfo;

class CBC_EncoderContext {
 public:
  CBC_EncoderContext(const WideString& msg,
                     const WideString& ecLevel,
                     int32_t& e);
  ~CBC_EncoderContext();

  void setAllowRectangular(bool allow);
  void setSkipAtEnd(int32_t count);
  wchar_t getCurrentChar();
  wchar_t getCurrent();
  void writeCodewords(const WideString& codewords);
  void writeCodeword(wchar_t codeword);
  size_t getCodewordCount();
  void signalEncoderChange(int32_t encoding);
  void resetEncoderSignal();
  bool hasMoreCharacters();
  size_t getRemainingCharacters();
  void updateSymbolInfo(int32_t& e);
  void updateSymbolInfo(int32_t len, int32_t& e);
  void resetSymbolInfo();

  WideString m_msg;
  WideString m_codewords;
  size_t m_pos;
  int32_t m_newEncoding;
  UnownedPtr<CBC_SymbolInfo> m_symbolInfo;

 private:
  size_t getTotalMessageCharCount();

  bool m_allowRectangular;  // Force square when false.
  size_t m_skipAtEnd;
};

#endif  // FXBARCODE_DATAMATRIX_BC_ENCODERCONTEXT_H_
