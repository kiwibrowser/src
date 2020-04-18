// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include <limits>
#include <vector>

#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_string.h"

namespace {

class CFX_UTF8Encoder {
 public:
  CFX_UTF8Encoder() {}
  ~CFX_UTF8Encoder() {}

  void Input(wchar_t unicodeAsWchar) {
    uint32_t unicode = static_cast<uint32_t>(unicodeAsWchar);
    if (unicode < 0x80) {
      m_Buffer.push_back(unicode);
    } else {
      if (unicode >= 0x80000000)
        return;

      int nbytes = 0;
      if (unicode < 0x800)
        nbytes = 2;
      else if (unicode < 0x10000)
        nbytes = 3;
      else if (unicode < 0x200000)
        nbytes = 4;
      else if (unicode < 0x4000000)
        nbytes = 5;
      else
        nbytes = 6;

      static const uint8_t prefix[] = {0xc0, 0xe0, 0xf0, 0xf8, 0xfc};
      int order = 1 << ((nbytes - 1) * 6);
      int code = unicodeAsWchar;
      m_Buffer.push_back(prefix[nbytes - 2] | (code / order));
      for (int i = 0; i < nbytes - 1; i++) {
        code = code % order;
        order >>= 6;
        m_Buffer.push_back(0x80 | (code / order));
      }
    }
  }

  // The data returned by GetResult() is invalidated when this is modified by
  // appending any data.
  ByteStringView GetResult() const {
    return ByteStringView(m_Buffer.data(), m_Buffer.size());
  }

 private:
  std::vector<uint8_t> m_Buffer;
};

}  // namespace

ByteString FX_UTF8Encode(const WideStringView& wsStr) {
  size_t len = wsStr.GetLength();
  const wchar_t* pStr = wsStr.unterminated_c_str();
  CFX_UTF8Encoder encoder;
  while (len-- > 0)
    encoder.Input(*pStr++);

  return ByteString(encoder.GetResult());
}

namespace {

const float fraction_scales[] = {0.1f,          0.01f,         0.001f,
                                 0.0001f,       0.00001f,      0.000001f,
                                 0.0000001f,    0.00000001f,   0.000000001f,
                                 0.0000000001f, 0.00000000001f};

float FractionalScale(size_t scale_factor, int value) {
  return fraction_scales[scale_factor] * value;
}

}  // namespace

bool FX_atonum(const ByteStringView& strc, void* pData) {
  if (strc.Contains('.')) {
    float* pFloat = static_cast<float*>(pData);
    *pFloat = FX_atof(strc);
    return false;
  }

  // Note, numbers in PDF are typically of the form 123, -123, etc. But,
  // for things like the Permissions on the encryption hash the number is
  // actually an unsigned value. We use a uint32_t so we can deal with the
  // unsigned and then check for overflow if the user actually signed the value.
  // The Permissions flag is listed in Table 3.20 PDF 1.7 spec.
  pdfium::base::CheckedNumeric<uint32_t> integer = 0;
  bool bNegative = false;
  bool bSigned = false;
  size_t cc = 0;
  if (strc[0] == '+') {
    cc++;
    bSigned = true;
  } else if (strc[0] == '-') {
    bNegative = true;
    bSigned = true;
    cc++;
  }

  while (cc < strc.GetLength() && std::isdigit(strc[cc])) {
    integer = integer * 10 + FXSYS_DecimalCharToInt(strc.CharAt(cc));
    if (!integer.IsValid())
      break;
    cc++;
  }

  // We have a sign, and the value was greater then a regular integer
  // we've overflowed, reset to the default value.
  if (bSigned) {
    if (bNegative) {
      if (integer.ValueOrDefault(0) >
          static_cast<uint32_t>(std::numeric_limits<int>::max()) + 1) {
        integer = 0;
      }
    } else if (integer.ValueOrDefault(0) >
               static_cast<uint32_t>(std::numeric_limits<int>::max())) {
      integer = 0;
    }
  }

  // Switch back to the int space so we can flip to a negative if we need.
  uint32_t uValue = integer.ValueOrDefault(0);
  int32_t value = static_cast<int>(uValue);
  if (bNegative)
    value = -value;

  int* pInt = static_cast<int*>(pData);
  *pInt = value;
  return true;
}

float FX_atof(const ByteStringView& strc) {
  if (strc.IsEmpty())
    return 0.0;

  int cc = 0;
  bool bNegative = false;
  int len = strc.GetLength();
  if (strc[0] == '+') {
    cc++;
  } else if (strc[0] == '-') {
    bNegative = true;
    cc++;
  }
  while (cc < len) {
    if (strc[cc] != '+' && strc[cc] != '-')
      break;
    cc++;
  }
  float value = 0;
  while (cc < len) {
    if (strc[cc] == '.')
      break;
    value = value * 10 + FXSYS_DecimalCharToInt(strc.CharAt(cc));
    cc++;
  }
  int scale = 0;
  if (cc < len && strc[cc] == '.') {
    cc++;
    while (cc < len) {
      value += FractionalScale(scale, FXSYS_DecimalCharToInt(strc.CharAt(cc)));
      scale++;
      if (scale == FX_ArraySize(fraction_scales))
        break;
      cc++;
    }
  }
  return bNegative ? -value : value;
}

float FX_atof(const WideStringView& wsStr) {
  return FX_atof(FX_UTF8Encode(wsStr).c_str());
}

size_t FX_ftoa(float d, char* buf) {
  buf[0] = '0';
  buf[1] = '\0';
  if (d == 0.0f) {
    return 1;
  }
  bool bNegative = false;
  if (d < 0) {
    bNegative = true;
    d = -d;
  }
  int scale = 1;
  int scaled = FXSYS_round(d);
  while (scaled < 100000) {
    if (scale == 1000000) {
      break;
    }
    scale *= 10;
    scaled = FXSYS_round(d * scale);
  }
  if (scaled == 0) {
    return 1;
  }
  char buf2[32];
  size_t buf_size = 0;
  if (bNegative) {
    buf[buf_size++] = '-';
  }
  int i = scaled / scale;
  FXSYS_itoa(i, buf2, 10);
  size_t len = strlen(buf2);
  memcpy(buf + buf_size, buf2, len);
  buf_size += len;
  int fraction = scaled % scale;
  if (fraction == 0) {
    return buf_size;
  }
  buf[buf_size++] = '.';
  scale /= 10;
  while (fraction) {
    buf[buf_size++] = '0' + fraction / scale;
    fraction %= scale;
    scale /= 10;
  }
  return buf_size;
}
