// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXBARCODE_QRCODE_BC_QRCODERMATRIXUTIL_H_
#define FXBARCODE_QRCODE_BC_QRCODERMATRIXUTIL_H_

class CBC_CommonByteMatrix;
class CBC_QRCoderErrorCorrectionLevel;
class CBC_QRCoderBitVector;

class CBC_QRCoderMatrixUtil {
 public:
  CBC_QRCoderMatrixUtil();
  virtual ~CBC_QRCoderMatrixUtil();
  static void ClearMatrix(CBC_CommonByteMatrix* matrix, int32_t& e);
  static void BuildMatrix(CBC_QRCoderBitVector* dataBits,
                          const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                          int32_t version,
                          int32_t maskPattern,
                          CBC_CommonByteMatrix* matrix,
                          int32_t& e);
  static void EmbedBasicPatterns(int32_t version,
                                 CBC_CommonByteMatrix* matrix,
                                 int32_t& e);
  static void EmbedTypeInfo(const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                            int32_t maskPattern,
                            CBC_CommonByteMatrix* matrix,
                            int32_t& e);
  static void EmbedDataBits(CBC_QRCoderBitVector* dataBits,
                            int32_t maskPattern,
                            CBC_CommonByteMatrix* matrix,
                            int32_t& e);
  static void MaybeEmbedVersionInfo(int32_t version,
                                    CBC_CommonByteMatrix* matrix,
                                    int32_t& e);
  static int32_t FindMSBSet(int32_t value);
  static int32_t CalculateBCHCode(int32_t code, int32_t poly);
  static void MakeTypeInfoBits(const CBC_QRCoderErrorCorrectionLevel* ecLevel,
                               int32_t maskPattern,
                               CBC_QRCoderBitVector* bits,
                               int32_t& e);
  static void MakeVersionInfoBits(int32_t version,
                                  CBC_QRCoderBitVector* bits,
                                  int32_t& e);
  static bool IsEmpty(int32_t value);
  static bool IsValidValue(int32_t value);
  static void EmbedTimingPatterns(CBC_CommonByteMatrix* matrix, int32_t& e);
  static void EmbedDarkDotAtLeftBottomCorner(CBC_CommonByteMatrix* matrix,
                                             int32_t& e);
  static void EmbedHorizontalSeparationPattern(int32_t xStart,
                                               int32_t yStart,
                                               CBC_CommonByteMatrix* matrix,
                                               int32_t& e);
  static void EmbedVerticalSeparationPattern(int32_t xStart,
                                             int32_t yStart,
                                             CBC_CommonByteMatrix* matrix,
                                             int32_t& e);
  static void EmbedPositionAdjustmentPattern(int32_t xStart,
                                             int32_t yStart,
                                             CBC_CommonByteMatrix* matrix,
                                             int32_t& e);
  static void EmbedPositionDetectionPattern(int32_t xStart,
                                            int32_t yStart,
                                            CBC_CommonByteMatrix* matrix,
                                            int32_t& e);
  static void EmbedPositionDetectionPatternsAndSeparators(
      CBC_CommonByteMatrix* matrix,
      int32_t& e);
  static void MaybeEmbedPositionAdjustmentPatterns(int32_t version,
                                                   CBC_CommonByteMatrix* matrix,
                                                   int32_t& e);
};

#endif  // FXBARCODE_QRCODE_BC_QRCODERMATRIXUTIL_H_
