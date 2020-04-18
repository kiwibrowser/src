// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_QRCODE_BC_QRCODER_H_
#define FXBARCODE_QRCODE_BC_QRCODER_H_

#include <memory>

#include "core/fxcrt/unowned_ptr.h"

class CBC_QRCoderErrorCorrectionLevel;
class CBC_QRCoderMode;
class CBC_CommonByteMatrix;

class CBC_QRCoder {
 public:
  static constexpr int32_t kNumMaskPatterns = 8;

  CBC_QRCoder();
  virtual ~CBC_QRCoder();

  static bool IsValidMaskPattern(int32_t maskPattern);

  CBC_QRCoderMode* GetMode() const;
  const CBC_QRCoderErrorCorrectionLevel* GetECLevel() const;
  int32_t GetVersion() const;
  int32_t GetMatrixWidth() const;
  int32_t GetMaskPattern() const;
  int32_t GetNumTotalBytes() const;
  int32_t GetNumDataBytes() const;
  int32_t GetNumECBytes() const;
  int32_t GetNumRSBlocks() const;
  CBC_CommonByteMatrix* GetMatrix() const;

  int32_t At(int32_t x, int32_t y, int32_t& e);
  bool IsValid();

  void SetMode(CBC_QRCoderMode* value);
  void SetECLevel(const CBC_QRCoderErrorCorrectionLevel* ecLevel);
  void SetVersion(int32_t version);
  void SetMatrixWidth(int32_t width);
  void SetMaskPattern(int32_t pattern);
  void SetNumDataBytes(int32_t bytes);
  void SetNumTotalBytes(int32_t value);
  void SetNumECBytes(int32_t value);
  void SetNumRSBlocks(int32_t block);
  void SetMatrix(std::unique_ptr<CBC_CommonByteMatrix> pMatrix);

 private:
  UnownedPtr<CBC_QRCoderMode> m_mode;
  UnownedPtr<const CBC_QRCoderErrorCorrectionLevel> m_ecLevel;
  int32_t m_version;
  int32_t m_matrixWidth;
  int32_t m_maskPattern;
  int32_t m_numTotalBytes;
  int32_t m_numDataBytes;
  int32_t m_numECBytes;
  int32_t m_numRSBlocks;
  std::unique_ptr<CBC_CommonByteMatrix> m_matrix;
};

#endif  // FXBARCODE_QRCODE_BC_QRCODER_H_
