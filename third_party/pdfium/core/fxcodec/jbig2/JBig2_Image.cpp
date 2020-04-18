// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jbig2/JBig2_Image.h"

#include <limits.h>
#include <string.h>

#include <algorithm>
#include <memory>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_safe_types.h"
#include "third_party/base/ptr_util.h"

namespace {

const int kMaxImagePixels = INT_MAX - 31;
const int kMaxImageBytes = kMaxImagePixels / 8;

}  // namespace

CJBig2_Image::CJBig2_Image(int32_t w, int32_t h)
    : m_pData(nullptr), m_nWidth(0), m_nHeight(0), m_nStride(0) {
  if (w <= 0 || h <= 0 || w > kMaxImagePixels)
    return;

  int32_t stride_pixels = (w + 31) & ~31;
  if (h > kMaxImagePixels / stride_pixels)
    return;

  m_nWidth = w;
  m_nHeight = h;
  m_nStride = stride_pixels / 8;
  m_pData.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
      FX_Alloc2D(uint8_t, m_nStride, m_nHeight)));
}

CJBig2_Image::CJBig2_Image(int32_t w, int32_t h, int32_t stride, uint8_t* pBuf)
    : m_pData(nullptr), m_nWidth(0), m_nHeight(0), m_nStride(0) {
  if (w < 0 || h < 0 || stride < 0 || stride > kMaxImageBytes)
    return;

  int32_t stride_pixels = 8 * stride;
  if (stride_pixels < w || h > kMaxImagePixels / stride_pixels)
    return;

  m_nWidth = w;
  m_nHeight = h;
  m_nStride = stride;
  m_pData.Reset(pBuf);
}

CJBig2_Image::CJBig2_Image(const CJBig2_Image& other)
    : m_pData(nullptr),
      m_nWidth(other.m_nWidth),
      m_nHeight(other.m_nHeight),
      m_nStride(other.m_nStride) {
  if (other.m_pData) {
    m_pData.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
        FX_Alloc2D(uint8_t, m_nStride, m_nHeight)));
    memcpy(data(), other.data(), m_nStride * m_nHeight);
  }
}

CJBig2_Image::~CJBig2_Image() {}

// static
bool CJBig2_Image::IsValidImageSize(int32_t w, int32_t h) {
  return w > 0 && w <= JBIG2_MAX_IMAGE_SIZE && h > 0 &&
         h <= JBIG2_MAX_IMAGE_SIZE;
}

int CJBig2_Image::GetPixel(int32_t x, int32_t y) const {
  if (!m_pData)
    return 0;

  if (x < 0 || x >= m_nWidth)
    return 0;

  if (y < 0 || y >= m_nHeight)
    return 0;

  int32_t m = y * m_nStride + (x >> 3);
  int32_t n = x & 7;
  return ((data()[m] >> (7 - n)) & 1);
}

void CJBig2_Image::SetPixel(int32_t x, int32_t y, int v) {
  if (!m_pData)
    return;

  if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight)
    return;

  int32_t m = y * m_nStride + (x >> 3);
  int32_t n = 1 << (7 - (x & 7));
  if (v)
    data()[m] |= n;
  else
    data()[m] &= ~n;
}

void CJBig2_Image::CopyLine(int32_t hTo, int32_t hFrom) {
  if (!m_pData)
    return;

  if (hFrom < 0 || hFrom >= m_nHeight) {
    memset(data() + hTo * m_nStride, 0, m_nStride);
  } else {
    memcpy(data() + hTo * m_nStride, data() + hFrom * m_nStride, m_nStride);
  }
}
void CJBig2_Image::Fill(bool v) {
  if (!m_pData)
    return;

  memset(data(), v ? 0xff : 0, m_nStride * m_nHeight);
}
bool CJBig2_Image::ComposeTo(CJBig2_Image* pDst,
                             int32_t x,
                             int32_t y,
                             JBig2ComposeOp op) {
  return m_pData ? ComposeToOpt2(pDst, x, y, op) : false;
}
bool CJBig2_Image::ComposeToWithRect(CJBig2_Image* pDst,
                                     int32_t x,
                                     int32_t y,
                                     const FX_RECT& rtSrc,
                                     JBig2ComposeOp op) {
  if (!m_pData)
    return false;

  if (rtSrc == FX_RECT(0, 0, m_nWidth, m_nHeight))
    return ComposeToOpt2(pDst, x, y, op);
  return ComposeToOpt2WithRect(pDst, x, y, op, rtSrc);
}

bool CJBig2_Image::ComposeFrom(int32_t x,
                               int32_t y,
                               CJBig2_Image* pSrc,
                               JBig2ComposeOp op) {
  return m_pData ? pSrc->ComposeTo(this, x, y, op) : false;
}
bool CJBig2_Image::ComposeFromWithRect(int32_t x,
                                       int32_t y,
                                       CJBig2_Image* pSrc,
                                       const FX_RECT& rtSrc,
                                       JBig2ComposeOp op) {
  return m_pData ? pSrc->ComposeToWithRect(this, x, y, rtSrc, op) : false;
}

#define JBIG2_GETDWORD(buf) \
  ((uint32_t)(((buf)[0] << 24) | ((buf)[1] << 16) | ((buf)[2] << 8) | (buf)[3]))

std::unique_ptr<CJBig2_Image> CJBig2_Image::SubImage(int32_t x,
                                                     int32_t y,
                                                     int32_t w,
                                                     int32_t h) {
  int32_t m;
  int32_t n;
  int32_t j;
  uint8_t* pLineSrc;
  uint8_t* pLineDst;
  uint32_t wTmp;
  uint8_t* pSrc;
  uint8_t* pSrcEnd;
  uint8_t* pDst;
  uint8_t* pDstEnd;
  if (w == 0 || h == 0)
    return nullptr;

  auto pImage = pdfium::MakeUnique<CJBig2_Image>(w, h);
  if (!m_pData) {
    pImage->Fill(0);
    return pImage;
  }
  if (!pImage->m_pData)
    return pImage;

  pLineSrc = data() + m_nStride * y;
  pLineDst = pImage->data();
  m = (x >> 5) << 2;
  n = x & 31;
  if (n == 0) {
    for (j = 0; j < h; j++) {
      pSrc = pLineSrc + m;
      pSrcEnd = pLineSrc + m_nStride;
      pDst = pLineDst;
      pDstEnd = pLineDst + pImage->m_nStride;
      for (; pDst < pDstEnd; pSrc += 4, pDst += 4) {
        *((uint32_t*)pDst) = *((uint32_t*)pSrc);
      }
      pLineSrc += m_nStride;
      pLineDst += pImage->m_nStride;
    }
  } else {
    for (j = 0; j < h; j++) {
      pSrc = pLineSrc + m;
      pSrcEnd = pLineSrc + m_nStride;
      pDst = pLineDst;
      pDstEnd = pLineDst + pImage->m_nStride;
      for (; pDst < pDstEnd; pSrc += 4, pDst += 4) {
        if (pSrc + 4 < pSrcEnd) {
          wTmp = (JBIG2_GETDWORD(pSrc) << n) |
                 (JBIG2_GETDWORD(pSrc + 4) >> (32 - n));
        } else {
          wTmp = JBIG2_GETDWORD(pSrc) << n;
        }
        pDst[0] = (uint8_t)(wTmp >> 24);
        pDst[1] = (uint8_t)(wTmp >> 16);
        pDst[2] = (uint8_t)(wTmp >> 8);
        pDst[3] = (uint8_t)wTmp;
      }
      pLineSrc += m_nStride;
      pLineDst += pImage->m_nStride;
    }
  }
  return pImage;
}

void CJBig2_Image::Expand(int32_t h, bool v) {
  if (!m_pData || h <= m_nHeight || h > kMaxImageBytes / m_nStride)
    return;

  if (m_pData.IsOwned()) {
    m_pData.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(FX_Realloc(
        uint8_t, m_pData.ReleaseAndClear().release(), h * m_nStride)));
  } else {
    uint8_t* pExternalBuffer = data();
    m_pData.Reset(std::unique_ptr<uint8_t, FxFreeDeleter>(
        FX_Alloc(uint8_t, h * m_nStride)));
    memcpy(data(), pExternalBuffer, m_nHeight * m_nStride);
  }
  memset(data() + m_nHeight * m_nStride, v ? 0xff : 0,
         (h - m_nHeight) * m_nStride);
  m_nHeight = h;
}

bool CJBig2_Image::ComposeToOpt2(CJBig2_Image* pDst,
                                 int32_t x,
                                 int32_t y,
                                 JBig2ComposeOp op) {
  ASSERT(m_pData);

  if (x < -1048576 || x > 1048576 || y < -1048576 || y > 1048576)
    return false;

  int32_t xs0 = x < 0 ? -x : 0;
  int32_t xs1;
  FX_SAFE_INT32 iChecked = pDst->m_nWidth;
  iChecked -= x;
  if (iChecked.IsValid() && m_nWidth > iChecked.ValueOrDie())
    xs1 = iChecked.ValueOrDie();
  else
    xs1 = m_nWidth;

  int32_t ys0 = y < 0 ? -y : 0;
  int32_t ys1;
  iChecked = pDst->m_nHeight;
  iChecked -= y;
  if (iChecked.IsValid() && m_nHeight > iChecked.ValueOrDie())
    ys1 = pDst->m_nHeight - y;
  else
    ys1 = m_nHeight;

  if (ys0 >= ys1 || xs0 >= xs1)
    return false;

  int32_t xd0 = std::max(x, 0);
  int32_t yd0 = std::max(y, 0);
  int32_t w = xs1 - xs0;
  int32_t h = ys1 - ys0;
  int32_t xd1 = xd0 + w;
  int32_t yd1 = yd0 + h;
  uint32_t d1 = xd0 & 31;
  uint32_t d2 = xd1 & 31;
  uint32_t s1 = xs0 & 31;
  uint32_t maskL = 0xffffffff >> d1;
  uint32_t maskR = 0xffffffff << ((32 - (xd1 & 31)) % 32);
  uint32_t maskM = maskL & maskR;
  uint8_t* lineSrc = data() + ys0 * m_nStride + ((xs0 >> 5) << 2);
  int32_t lineLeft = m_nStride - ((xs0 >> 5) << 2);
  uint8_t* lineDst = pDst->data() + yd0 * pDst->m_nStride + ((xd0 >> 5) << 2);
  if ((xd0 & ~31) == ((xd1 - 1) & ~31)) {
    if ((xs0 & ~31) == ((xs1 - 1) & ~31)) {
      if (s1 > d1) {
        uint32_t shift = s1 - d1;
        for (int32_t yy = yd0; yy < yd1; yy++) {
          uint32_t tmp1 = JBIG2_GETDWORD(lineSrc) << shift;
          uint32_t tmp2 = JBIG2_GETDWORD(lineDst);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskM) | ((tmp1 | tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskM) | ((tmp1 & tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskM) | ((tmp1 ^ tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskM) | ((~(tmp1 ^ tmp2)) & maskM);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskM) | (tmp1 & maskM);
              break;
          }
          lineDst[0] = (uint8_t)(tmp >> 24);
          lineDst[1] = (uint8_t)(tmp >> 16);
          lineDst[2] = (uint8_t)(tmp >> 8);
          lineDst[3] = (uint8_t)tmp;
          lineSrc += m_nStride;
          lineDst += pDst->m_nStride;
        }
      } else {
        uint32_t shift = d1 - s1;
        for (int32_t yy = yd0; yy < yd1; yy++) {
          uint32_t tmp1 = JBIG2_GETDWORD(lineSrc) >> shift;
          uint32_t tmp2 = JBIG2_GETDWORD(lineDst);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskM) | ((tmp1 | tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskM) | ((tmp1 & tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskM) | ((tmp1 ^ tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskM) | ((~(tmp1 ^ tmp2)) & maskM);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskM) | (tmp1 & maskM);
              break;
          }
          lineDst[0] = (uint8_t)(tmp >> 24);
          lineDst[1] = (uint8_t)(tmp >> 16);
          lineDst[2] = (uint8_t)(tmp >> 8);
          lineDst[3] = (uint8_t)tmp;
          lineSrc += m_nStride;
          lineDst += pDst->m_nStride;
        }
      }
    } else {
      uint32_t shift1 = s1 - d1;
      uint32_t shift2 = 32 - shift1;
      for (int32_t yy = yd0; yy < yd1; yy++) {
        uint32_t tmp1 = (JBIG2_GETDWORD(lineSrc) << shift1) |
                        (JBIG2_GETDWORD(lineSrc + 4) >> shift2);
        uint32_t tmp2 = JBIG2_GETDWORD(lineDst);
        uint32_t tmp = 0;
        switch (op) {
          case JBIG2_COMPOSE_OR:
            tmp = (tmp2 & ~maskM) | ((tmp1 | tmp2) & maskM);
            break;
          case JBIG2_COMPOSE_AND:
            tmp = (tmp2 & ~maskM) | ((tmp1 & tmp2) & maskM);
            break;
          case JBIG2_COMPOSE_XOR:
            tmp = (tmp2 & ~maskM) | ((tmp1 ^ tmp2) & maskM);
            break;
          case JBIG2_COMPOSE_XNOR:
            tmp = (tmp2 & ~maskM) | ((~(tmp1 ^ tmp2)) & maskM);
            break;
          case JBIG2_COMPOSE_REPLACE:
            tmp = (tmp2 & ~maskM) | (tmp1 & maskM);
            break;
        }
        lineDst[0] = (uint8_t)(tmp >> 24);
        lineDst[1] = (uint8_t)(tmp >> 16);
        lineDst[2] = (uint8_t)(tmp >> 8);
        lineDst[3] = (uint8_t)tmp;
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    }
  } else {
    uint8_t* sp = nullptr;
    uint8_t* dp = nullptr;

    if (s1 > d1) {
      uint32_t shift1 = s1 - d1;
      uint32_t shift2 = 32 - shift1;
      int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
      for (int32_t yy = yd0; yy < yd1; yy++) {
        sp = lineSrc;
        dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift1) |
                          (JBIG2_GETDWORD(sp + 4) >> shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskL) | ((tmp1 | tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskL) | ((tmp1 & tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskL) | ((tmp1 ^ tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskL) | ((~(tmp1 ^ tmp2)) & maskL);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskL) | (tmp1 & maskL);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift1) |
                          (JBIG2_GETDWORD(sp + 4) >> shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = tmp1 | tmp2;
              break;
            case JBIG2_COMPOSE_AND:
              tmp = tmp1 & tmp2;
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = tmp1 ^ tmp2;
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = ~(tmp1 ^ tmp2);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = tmp1;
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 =
              (JBIG2_GETDWORD(sp) << shift1) |
              (((sp + 4) < lineSrc + lineLeft ? JBIG2_GETDWORD(sp + 4) : 0) >>
               shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskR) | ((tmp1 | tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskR) | ((tmp1 & tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskR) | ((tmp1 ^ tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskR) | ((~(tmp1 ^ tmp2)) & maskR);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskR) | (tmp1 & maskR);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
        }
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    } else if (s1 == d1) {
      int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
      for (int32_t yy = yd0; yy < yd1; yy++) {
        sp = lineSrc;
        dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskL) | ((tmp1 | tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskL) | ((tmp1 & tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskL) | ((tmp1 ^ tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskL) | ((~(tmp1 ^ tmp2)) & maskL);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskL) | (tmp1 & maskL);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = tmp1 | tmp2;
              break;
            case JBIG2_COMPOSE_AND:
              tmp = tmp1 & tmp2;
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = tmp1 ^ tmp2;
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = ~(tmp1 ^ tmp2);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = tmp1;
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskR) | ((tmp1 | tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskR) | ((tmp1 & tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskR) | ((tmp1 ^ tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskR) | ((~(tmp1 ^ tmp2)) & maskR);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskR) | (tmp1 & maskR);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
        }
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    } else {
      uint32_t shift1 = d1 - s1;
      uint32_t shift2 = 32 - shift1;
      int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
      for (int32_t yy = yd0; yy < yd1; yy++) {
        sp = lineSrc;
        dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp) >> shift1;
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskL) | ((tmp1 | tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskL) | ((tmp1 & tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskL) | ((tmp1 ^ tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskL) | ((~(tmp1 ^ tmp2)) & maskL);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskL) | (tmp1 & maskL);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift2) |
                          ((JBIG2_GETDWORD(sp + 4)) >> shift1);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = tmp1 | tmp2;
              break;
            case JBIG2_COMPOSE_AND:
              tmp = tmp1 & tmp2;
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = tmp1 ^ tmp2;
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = ~(tmp1 ^ tmp2);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = tmp1;
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 =
              (JBIG2_GETDWORD(sp) << shift2) |
              (((sp + 4) < lineSrc + lineLeft ? JBIG2_GETDWORD(sp + 4) : 0) >>
               shift1);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskR) | ((tmp1 | tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskR) | ((tmp1 & tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskR) | ((tmp1 ^ tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskR) | ((~(tmp1 ^ tmp2)) & maskR);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskR) | (tmp1 & maskR);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
        }
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    }
  }
  return true;
}

bool CJBig2_Image::ComposeToOpt2WithRect(CJBig2_Image* pDst,
                                         int32_t x,
                                         int32_t y,
                                         JBig2ComposeOp op,
                                         const FX_RECT& rtSrc) {
  ASSERT(m_pData);

  // TODO(weili): Check whether the range check is correct. Should x>=1048576?
  if (x < -1048576 || x > 1048576 || y < -1048576 || y > 1048576) {
    return false;
  }
  int32_t sw = rtSrc.Width();
  int32_t sh = rtSrc.Height();
  int32_t ys0 = y < 0 ? -y : 0;
  int32_t ys1 = y + sh > pDst->m_nHeight ? pDst->m_nHeight - y : sh;
  int32_t xs0 = x < 0 ? -x : 0;
  int32_t xs1 = x + sw > pDst->m_nWidth ? pDst->m_nWidth - x : sw;
  if ((ys0 >= ys1) || (xs0 >= xs1)) {
    return 0;
  }
  int32_t w = xs1 - xs0;
  int32_t h = ys1 - ys0;
  int32_t yd0 = y < 0 ? 0 : y;
  int32_t xd0 = x < 0 ? 0 : x;
  int32_t xd1 = xd0 + w;
  int32_t yd1 = yd0 + h;
  int32_t d1 = xd0 & 31;
  int32_t d2 = xd1 & 31;
  int32_t s1 = xs0 & 31;
  int32_t maskL = 0xffffffff >> d1;
  int32_t maskR = 0xffffffff << ((32 - (xd1 & 31)) % 32);
  int32_t maskM = maskL & maskR;
  uint8_t* lineSrc =
      data() + (rtSrc.top + ys0) * m_nStride + (((xs0 + rtSrc.left) >> 5) << 2);
  int32_t lineLeft = m_nStride - ((xs0 >> 5) << 2);
  uint8_t* lineDst = pDst->data() + yd0 * pDst->m_nStride + ((xd0 >> 5) << 2);
  if ((xd0 & ~31) == ((xd1 - 1) & ~31)) {
    if ((xs0 & ~31) == ((xs1 - 1) & ~31)) {
      if (s1 > d1) {
        uint32_t shift = s1 - d1;
        for (int32_t yy = yd0; yy < yd1; yy++) {
          uint32_t tmp1 = JBIG2_GETDWORD(lineSrc) << shift;
          uint32_t tmp2 = JBIG2_GETDWORD(lineDst);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskM) | ((tmp1 | tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskM) | ((tmp1 & tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskM) | ((tmp1 ^ tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskM) | ((~(tmp1 ^ tmp2)) & maskM);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskM) | (tmp1 & maskM);
              break;
          }
          lineDst[0] = (uint8_t)(tmp >> 24);
          lineDst[1] = (uint8_t)(tmp >> 16);
          lineDst[2] = (uint8_t)(tmp >> 8);
          lineDst[3] = (uint8_t)tmp;
          lineSrc += m_nStride;
          lineDst += pDst->m_nStride;
        }
      } else {
        uint32_t shift = d1 - s1;
        for (int32_t yy = yd0; yy < yd1; yy++) {
          uint32_t tmp1 = JBIG2_GETDWORD(lineSrc) >> shift;
          uint32_t tmp2 = JBIG2_GETDWORD(lineDst);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskM) | ((tmp1 | tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskM) | ((tmp1 & tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskM) | ((tmp1 ^ tmp2) & maskM);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskM) | ((~(tmp1 ^ tmp2)) & maskM);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskM) | (tmp1 & maskM);
              break;
          }
          lineDst[0] = (uint8_t)(tmp >> 24);
          lineDst[1] = (uint8_t)(tmp >> 16);
          lineDst[2] = (uint8_t)(tmp >> 8);
          lineDst[3] = (uint8_t)tmp;
          lineSrc += m_nStride;
          lineDst += pDst->m_nStride;
        }
      }
    } else {
      uint32_t shift1 = s1 - d1;
      uint32_t shift2 = 32 - shift1;
      for (int32_t yy = yd0; yy < yd1; yy++) {
        uint32_t tmp1 = (JBIG2_GETDWORD(lineSrc) << shift1) |
                        (JBIG2_GETDWORD(lineSrc + 4) >> shift2);
        uint32_t tmp2 = JBIG2_GETDWORD(lineDst);
        uint32_t tmp = 0;
        switch (op) {
          case JBIG2_COMPOSE_OR:
            tmp = (tmp2 & ~maskM) | ((tmp1 | tmp2) & maskM);
            break;
          case JBIG2_COMPOSE_AND:
            tmp = (tmp2 & ~maskM) | ((tmp1 & tmp2) & maskM);
            break;
          case JBIG2_COMPOSE_XOR:
            tmp = (tmp2 & ~maskM) | ((tmp1 ^ tmp2) & maskM);
            break;
          case JBIG2_COMPOSE_XNOR:
            tmp = (tmp2 & ~maskM) | ((~(tmp1 ^ tmp2)) & maskM);
            break;
          case JBIG2_COMPOSE_REPLACE:
            tmp = (tmp2 & ~maskM) | (tmp1 & maskM);
            break;
        }
        lineDst[0] = (uint8_t)(tmp >> 24);
        lineDst[1] = (uint8_t)(tmp >> 16);
        lineDst[2] = (uint8_t)(tmp >> 8);
        lineDst[3] = (uint8_t)tmp;
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    }
  } else {
    if (s1 > d1) {
      uint32_t shift1 = s1 - d1;
      uint32_t shift2 = 32 - shift1;
      int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
      for (int32_t yy = yd0; yy < yd1; yy++) {
        uint8_t* sp = lineSrc;
        uint8_t* dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift1) |
                          (JBIG2_GETDWORD(sp + 4) >> shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskL) | ((tmp1 | tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskL) | ((tmp1 & tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskL) | ((tmp1 ^ tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskL) | ((~(tmp1 ^ tmp2)) & maskL);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskL) | (tmp1 & maskL);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift1) |
                          (JBIG2_GETDWORD(sp + 4) >> shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = tmp1 | tmp2;
              break;
            case JBIG2_COMPOSE_AND:
              tmp = tmp1 & tmp2;
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = tmp1 ^ tmp2;
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = ~(tmp1 ^ tmp2);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = tmp1;
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 =
              (JBIG2_GETDWORD(sp) << shift1) |
              (((sp + 4) < lineSrc + lineLeft ? JBIG2_GETDWORD(sp + 4) : 0) >>
               shift2);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskR) | ((tmp1 | tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskR) | ((tmp1 & tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskR) | ((tmp1 ^ tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskR) | ((~(tmp1 ^ tmp2)) & maskR);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskR) | (tmp1 & maskR);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
        }
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    } else if (s1 == d1) {
      int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
      for (int32_t yy = yd0; yy < yd1; yy++) {
        uint8_t* sp = lineSrc;
        uint8_t* dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskL) | ((tmp1 | tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskL) | ((tmp1 & tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskL) | ((tmp1 ^ tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskL) | ((~(tmp1 ^ tmp2)) & maskL);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskL) | (tmp1 & maskL);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = tmp1 | tmp2;
              break;
            case JBIG2_COMPOSE_AND:
              tmp = tmp1 & tmp2;
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = tmp1 ^ tmp2;
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = ~(tmp1 ^ tmp2);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = tmp1;
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskR) | ((tmp1 | tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskR) | ((tmp1 & tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskR) | ((tmp1 ^ tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskR) | ((~(tmp1 ^ tmp2)) & maskR);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskR) | (tmp1 & maskR);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
        }
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    } else {
      uint32_t shift1 = d1 - s1;
      uint32_t shift2 = 32 - shift1;
      int32_t middleDwords = (xd1 >> 5) - ((xd0 + 31) >> 5);
      for (int32_t yy = yd0; yy < yd1; yy++) {
        uint8_t* sp = lineSrc;
        uint8_t* dp = lineDst;
        if (d1 != 0) {
          uint32_t tmp1 = JBIG2_GETDWORD(sp) >> shift1;
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskL) | ((tmp1 | tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskL) | ((tmp1 & tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskL) | ((tmp1 ^ tmp2) & maskL);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskL) | ((~(tmp1 ^ tmp2)) & maskL);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskL) | (tmp1 & maskL);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          dp += 4;
        }
        for (int32_t xx = 0; xx < middleDwords; xx++) {
          uint32_t tmp1 = (JBIG2_GETDWORD(sp) << shift2) |
                          ((JBIG2_GETDWORD(sp + 4)) >> shift1);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = tmp1 | tmp2;
              break;
            case JBIG2_COMPOSE_AND:
              tmp = tmp1 & tmp2;
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = tmp1 ^ tmp2;
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = ~(tmp1 ^ tmp2);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = tmp1;
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
          sp += 4;
          dp += 4;
        }
        if (d2 != 0) {
          uint32_t tmp1 =
              (JBIG2_GETDWORD(sp) << shift2) |
              (((sp + 4) < lineSrc + lineLeft ? JBIG2_GETDWORD(sp + 4) : 0) >>
               shift1);
          uint32_t tmp2 = JBIG2_GETDWORD(dp);
          uint32_t tmp = 0;
          switch (op) {
            case JBIG2_COMPOSE_OR:
              tmp = (tmp2 & ~maskR) | ((tmp1 | tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_AND:
              tmp = (tmp2 & ~maskR) | ((tmp1 & tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XOR:
              tmp = (tmp2 & ~maskR) | ((tmp1 ^ tmp2) & maskR);
              break;
            case JBIG2_COMPOSE_XNOR:
              tmp = (tmp2 & ~maskR) | ((~(tmp1 ^ tmp2)) & maskR);
              break;
            case JBIG2_COMPOSE_REPLACE:
              tmp = (tmp2 & ~maskR) | (tmp1 & maskR);
              break;
          }
          dp[0] = (uint8_t)(tmp >> 24);
          dp[1] = (uint8_t)(tmp >> 16);
          dp[2] = (uint8_t)(tmp >> 8);
          dp[3] = (uint8_t)tmp;
        }
        lineSrc += m_nStride;
        lineDst += pDst->m_nStride;
      }
    }
  }
  return 1;
}
