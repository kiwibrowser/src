// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_DATAMATRIX_BC_SYMBOLINFO_H_
#define FXBARCODE_DATAMATRIX_BC_SYMBOLINFO_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"

class CBC_SymbolInfo {
 public:
  CBC_SymbolInfo(int32_t dataCapacity,
                 int32_t errorCodewords,
                 int32_t matrixWidth,
                 int32_t matrixHeight,
                 int32_t dataRegions);
  virtual ~CBC_SymbolInfo();

  static void Initialize();
  static void Finalize();
  static void overrideSymbolSet(CBC_SymbolInfo* override);
  static CBC_SymbolInfo* lookup(int32_t dataCodewords,
                                bool allowRectangular,
                                int32_t& e);

  int32_t getSymbolDataWidth() const;
  int32_t getSymbolDataHeight() const;
  int32_t getSymbolWidth() const;
  int32_t getSymbolHeight() const;
  int32_t getCodewordCount() const;
  virtual int32_t getInterleavedBlockCount() const;
  int32_t getDataLengthForInterleavedBlock(int32_t index) const;
  int32_t getErrorLengthForInterleavedBlock(int32_t index) const;

  int32_t dataCapacity() const { return m_dataCapacity; }
  int32_t errorCodewords() const { return m_errorCodewords; }
  int32_t matrixWidth() const { return m_matrixWidth; }
  int32_t matrixHeight() const { return m_matrixHeight; }

 protected:
  CBC_SymbolInfo(int32_t dataCapacity,
                 int32_t errorCodewords,
                 int32_t matrixWidth,
                 int32_t matrixHeight,
                 int32_t dataRegions,
                 int32_t rsBlockData,
                 int32_t rsBlockError);

 private:
  int32_t getHorizontalDataRegions() const;
  int32_t getVerticalDataRegions() const;

  const bool m_rectangular;
  const int32_t m_dataCapacity;
  const int32_t m_errorCodewords;
  const int32_t m_matrixWidth;
  const int32_t m_matrixHeight;
  const int32_t m_dataRegions;
  const int32_t m_rsBlockData;
  const int32_t m_rsBlockError;
};

#endif  // FXBARCODE_DATAMATRIX_BC_SYMBOLINFO_H_
