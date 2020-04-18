// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/edit/cpdf_encryptor.h"
#include "core/fpdfapi/parser/cpdf_crypto_handler.h"

CPDF_Encryptor::CPDF_Encryptor(CPDF_CryptoHandler* pHandler,
                               int objnum,
                               pdfium::span<const uint8_t> src_data) {
  if (src_data.empty())
    return;

  if (!pHandler) {
    m_Span = src_data;
    return;
  }

  uint32_t buf_size = pHandler->EncryptGetSize(src_data);
  m_NewBuf.resize(buf_size);
  pHandler->EncryptContent(objnum, 0, src_data, m_NewBuf.data(),
                           buf_size);  // Updates |buf_size| with actual.
  m_NewBuf.resize(buf_size);
  m_Span = m_NewBuf;
}

CPDF_Encryptor::~CPDF_Encryptor() {}
