// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <memory>

#include "core/fxcodec/codec/ccodec_faxmodule.h"
#include "core/fxcodec/codec/ccodec_scanlinedecoder.h"

static int GetInteger(const uint8_t* data) {
  return data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  const int kParameterSize = 21;
  if (size < kParameterSize)
    return 0;

  int width = GetInteger(data);
  int height = GetInteger(data + 4);
  int K = GetInteger(data + 8);
  int Columns = GetInteger(data + 12);
  int Rows = GetInteger(data + 16);
  bool EndOfLine = !(data[20] & 0x01);
  bool ByteAlign = !(data[20] & 0x02);
  bool BlackIs1 = !(data[20] & 0x04);
  data += kParameterSize;
  size -= kParameterSize;

  CCodec_FaxModule fax_module;
  std::unique_ptr<CCodec_ScanlineDecoder> decoder(
      fax_module.CreateDecoder(data, size, width, height, K, EndOfLine,
                               ByteAlign, BlackIs1, Columns, Rows));

  if (decoder) {
    int line = 0;
    while (decoder->GetScanline(line))
      line++;
  }

  return 0;
}
