// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PARSER_CPDF_STREAM_ACC_H_
#define CORE_FPDFAPI_PARSER_CPDF_STREAM_ACC_H_

#include <memory>

#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "third_party/base/span.h"

class CPDF_StreamAcc : public Retainable {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  CPDF_StreamAcc(const CPDF_StreamAcc&) = delete;
  CPDF_StreamAcc& operator=(const CPDF_StreamAcc&) = delete;

  void LoadAllData(bool bRawAccess, uint32_t estimated_size, bool bImageAcc);
  void LoadAllDataFiltered();
  void LoadAllDataRaw();

  const CPDF_Stream* GetStream() const { return m_pStream.Get(); }
  const CPDF_Dictionary* GetDict() const;

  uint8_t* GetData() const;
  uint32_t GetSize() const;
  pdfium::span<uint8_t> GetSpan() const {
    return pdfium::make_span(GetData(), GetSize());
  }
  const ByteString& GetImageDecoder() const { return m_ImageDecoder; }
  const CPDF_Dictionary* GetImageParam() const { return m_pImageParam; }
  std::unique_ptr<uint8_t, FxFreeDeleter> DetachData();

 protected:
  explicit CPDF_StreamAcc(const CPDF_Stream* pStream);
  ~CPDF_StreamAcc() override;

 private:
  uint8_t* m_pData = nullptr;
  uint32_t m_dwSize = 0;
  bool m_bNewBuf = false;
  ByteString m_ImageDecoder;
  CPDF_Dictionary* m_pImageParam = nullptr;
  UnownedPtr<const CPDF_Stream> const m_pStream;
  uint8_t* m_pSrcData = nullptr;
};

#endif  // CORE_FPDFAPI_PARSER_CPDF_STREAM_ACC_H_
