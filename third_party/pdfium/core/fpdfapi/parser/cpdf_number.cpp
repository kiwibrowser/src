// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fxcrt/fx_stream.h"
#include "third_party/base/ptr_util.h"

CPDF_Number::CPDF_Number() : m_bInteger(true), m_Integer(0) {}

CPDF_Number::CPDF_Number(int value) : m_bInteger(true), m_Integer(value) {}

CPDF_Number::CPDF_Number(float value) : m_bInteger(false), m_Float(value) {}

CPDF_Number::CPDF_Number(const ByteStringView& str)
    : m_bInteger(FX_atonum(str, &m_Integer)) {}

CPDF_Number::~CPDF_Number() {}

CPDF_Object::Type CPDF_Number::GetType() const {
  return NUMBER;
}

std::unique_ptr<CPDF_Object> CPDF_Number::Clone() const {
  return m_bInteger ? pdfium::MakeUnique<CPDF_Number>(m_Integer)
                    : pdfium::MakeUnique<CPDF_Number>(m_Float);
}

float CPDF_Number::GetNumber() const {
  return m_bInteger ? static_cast<float>(m_Integer) : m_Float;
}

int CPDF_Number::GetInteger() const {
  return m_bInteger ? m_Integer : static_cast<int>(m_Float);
}

bool CPDF_Number::IsNumber() const {
  return true;
}

CPDF_Number* CPDF_Number::AsNumber() {
  return this;
}

const CPDF_Number* CPDF_Number::AsNumber() const {
  return this;
}

void CPDF_Number::SetString(const ByteString& str) {
  m_bInteger = FX_atonum(str.AsStringView(), &m_Integer);
}

ByteString CPDF_Number::GetString() const {
  return m_bInteger ? ByteString::FormatInteger(m_Integer)
                    : ByteString::FormatFloat(m_Float);
}

bool CPDF_Number::WriteTo(IFX_ArchiveStream* archive) const {
  return archive->WriteString(" ") &&
         archive->WriteString(GetString().AsStringView());
}
