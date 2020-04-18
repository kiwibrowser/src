// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include <setjmp.h>

#include <memory>
#include <utility>

#include "core/fxcodec/codec/ccodec_jpegmodule.h"
#include "core/fxcodec/codec/ccodec_scanlinedecoder.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxge/dib/cfx_dibsource.h"
#include "core/fxge/fx_dib.h"
#include "third_party/base/logging.h"
#include "third_party/base/ptr_util.h"

extern "C" {
#undef FAR
#if defined(USE_SYSTEM_LIBJPEG)
#include <jpeglib.h>
#elif defined(USE_LIBJPEG_TURBO)
#include "third_party/libjpeg_turbo/jpeglib.h"
#else
#include "third_party/libjpeg/jpeglib.h"
#endif
}  // extern "C"

class CJpegContext : public CCodec_JpegModule::Context {
 public:
  CJpegContext();
  ~CJpegContext() override;

  jmp_buf* GetJumpMark() override { return &m_JumpMark; }

  jmp_buf m_JumpMark;
  jpeg_decompress_struct m_Info;
  jpeg_error_mgr m_ErrMgr;
  jpeg_source_mgr m_SrcMgr;
  unsigned int m_SkipSize;
  void* (*m_AllocFunc)(unsigned int);
  void (*m_FreeFunc)(void*);
};

extern "C" {

static void JpegScanSOI(const uint8_t** src_buf, uint32_t* src_size) {
  if (*src_size == 0)
    return;

  uint32_t offset = 0;
  while (offset < *src_size - 1) {
    if ((*src_buf)[offset] == 0xff && (*src_buf)[offset + 1] == 0xd8) {
      *src_buf += offset;
      *src_size -= offset;
      return;
    }
    offset++;
  }
}

static void _src_do_nothing(struct jpeg_decompress_struct* cinfo) {}

static void _error_fatal(j_common_ptr cinfo) {
  longjmp(*(jmp_buf*)cinfo->client_data, -1);
}

static void _src_skip_data(struct jpeg_decompress_struct* cinfo, long num) {
  if (num > (long)cinfo->src->bytes_in_buffer) {
    _error_fatal((j_common_ptr)cinfo);
  }
  cinfo->src->next_input_byte += num;
  cinfo->src->bytes_in_buffer -= num;
}

static boolean _src_fill_buffer(j_decompress_ptr cinfo) {
  return 0;
}

static boolean _src_resync(j_decompress_ptr cinfo, int desired) {
  return 0;
}

static void _error_do_nothing(j_common_ptr cinfo) {}

static void _error_do_nothing1(j_common_ptr cinfo, int) {}

static void _error_do_nothing2(j_common_ptr cinfo, char*) {}

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
static void _dest_do_nothing(j_compress_ptr cinfo) {}

static boolean _dest_empty(j_compress_ptr cinfo) {
  return false;
}
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
}  // extern "C"

#ifdef PDF_ENABLE_XFA
static void JpegLoadAttribute(struct jpeg_decompress_struct* pInfo,
                              CFX_DIBAttribute* pAttribute) {
  if (!pAttribute)
    return;

  pAttribute->m_nXDPI = pInfo->X_density;
  pAttribute->m_nYDPI = pInfo->Y_density;
  pAttribute->m_wDPIUnit = pInfo->density_unit;
}
#endif  // PDF_ENABLE_XFA

static bool JpegLoadInfo(const uint8_t* src_buf,
                         uint32_t src_size,
                         int* width,
                         int* height,
                         int* num_components,
                         int* bits_per_components,
                         bool* color_transform) {
  JpegScanSOI(&src_buf, &src_size);
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  jerr.error_exit = _error_fatal;
  jerr.emit_message = _error_do_nothing1;
  jerr.output_message = _error_do_nothing;
  jerr.format_message = _error_do_nothing2;
  jerr.reset_error_mgr = _error_do_nothing;
  jerr.trace_level = 0;
  cinfo.err = &jerr;
  jmp_buf mark;
  cinfo.client_data = &mark;
  if (setjmp(mark) == -1)
    return false;

  jpeg_create_decompress(&cinfo);
  struct jpeg_source_mgr src;
  src.init_source = _src_do_nothing;
  src.term_source = _src_do_nothing;
  src.skip_input_data = _src_skip_data;
  src.fill_input_buffer = _src_fill_buffer;
  src.resync_to_restart = _src_resync;
  src.bytes_in_buffer = src_size;
  src.next_input_byte = src_buf;
  cinfo.src = &src;
  if (setjmp(mark) == -1) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }
  int ret = jpeg_read_header(&cinfo, true);
  if (ret != JPEG_HEADER_OK) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }
  *width = cinfo.image_width;
  *height = cinfo.image_height;
  *num_components = cinfo.num_components;
  *color_transform =
      cinfo.jpeg_color_space == JCS_YCbCr || cinfo.jpeg_color_space == JCS_YCCK;
  *bits_per_components = cinfo.data_precision;
  jpeg_destroy_decompress(&cinfo);
  return true;
}

class CCodec_JpegDecoder : public CCodec_ScanlineDecoder {
 public:
  CCodec_JpegDecoder();
  ~CCodec_JpegDecoder() override;

  bool Create(const uint8_t* src_buf,
              uint32_t src_size,
              int width,
              int height,
              int nComps,
              bool ColorTransform);

  // CCodec_ScanlineDecoder
  bool v_Rewind() override;
  uint8_t* v_GetNextLine() override;
  uint32_t GetSrcOffset() override;

  bool InitDecode();

  jmp_buf m_JmpBuf;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_source_mgr src;
  const uint8_t* m_SrcBuf;
  uint32_t m_SrcSize;
  uint8_t* m_pScanlineBuf;

  bool m_bInited;
  bool m_bStarted;
  bool m_bJpegTransform;

 protected:
  uint32_t m_nDefaultScaleDenom;
};

CCodec_JpegDecoder::CCodec_JpegDecoder() {
  m_pScanlineBuf = nullptr;
  m_bStarted = false;
  m_bInited = false;
  memset(&cinfo, 0, sizeof(cinfo));
  memset(&jerr, 0, sizeof(jerr));
  memset(&src, 0, sizeof(src));
  m_nDefaultScaleDenom = 1;
}

CCodec_JpegDecoder::~CCodec_JpegDecoder() {
  FX_Free(m_pScanlineBuf);
  if (m_bInited)
    jpeg_destroy_decompress(&cinfo);
}

bool CCodec_JpegDecoder::InitDecode() {
  cinfo.err = &jerr;
  cinfo.client_data = &m_JmpBuf;
  if (setjmp(m_JmpBuf) == -1)
    return false;

  jpeg_create_decompress(&cinfo);
  m_bInited = true;
  cinfo.src = &src;
  src.bytes_in_buffer = m_SrcSize;
  src.next_input_byte = m_SrcBuf;
  if (setjmp(m_JmpBuf) == -1) {
    jpeg_destroy_decompress(&cinfo);
    m_bInited = false;
    return false;
  }
  cinfo.image_width = m_OrigWidth;
  cinfo.image_height = m_OrigHeight;
  int ret = jpeg_read_header(&cinfo, true);
  if (ret != JPEG_HEADER_OK)
    return false;

  if (cinfo.saw_Adobe_marker)
    m_bJpegTransform = true;

  if (cinfo.num_components == 3 && !m_bJpegTransform)
    cinfo.out_color_space = cinfo.jpeg_color_space;

  m_OrigWidth = cinfo.image_width;
  m_OrigHeight = cinfo.image_height;
  m_OutputWidth = m_OrigWidth;
  m_OutputHeight = m_OrigHeight;
  m_nDefaultScaleDenom = cinfo.scale_denom;
  return true;
}

bool CCodec_JpegDecoder::Create(const uint8_t* src_buf,
                                uint32_t src_size,
                                int width,
                                int height,
                                int nComps,
                                bool ColorTransform) {
  JpegScanSOI(&src_buf, &src_size);
  m_SrcBuf = src_buf;
  m_SrcSize = src_size;
  jerr.error_exit = _error_fatal;
  jerr.emit_message = _error_do_nothing1;
  jerr.output_message = _error_do_nothing;
  jerr.format_message = _error_do_nothing2;
  jerr.reset_error_mgr = _error_do_nothing;
  src.init_source = _src_do_nothing;
  src.term_source = _src_do_nothing;
  src.skip_input_data = _src_skip_data;
  src.fill_input_buffer = _src_fill_buffer;
  src.resync_to_restart = _src_resync;
  m_bJpegTransform = ColorTransform;
  if (src_size > 1 && memcmp(src_buf + src_size - 2, "\xFF\xD9", 2) != 0) {
    ((uint8_t*)src_buf)[src_size - 2] = 0xFF;
    ((uint8_t*)src_buf)[src_size - 1] = 0xD9;
  }
  m_OutputWidth = m_OrigWidth = width;
  m_OutputHeight = m_OrigHeight = height;
  if (!InitDecode())
    return false;

  if (cinfo.num_components < nComps)
    return false;

  if ((int)cinfo.image_width < width)
    return false;

  m_Pitch =
      (static_cast<uint32_t>(cinfo.image_width) * cinfo.num_components + 3) /
      4 * 4;
  m_pScanlineBuf = FX_Alloc(uint8_t, m_Pitch);
  m_nComps = cinfo.num_components;
  m_bpc = 8;
  m_bStarted = false;
  return true;
}

bool CCodec_JpegDecoder::v_Rewind() {
  if (m_bStarted) {
    jpeg_destroy_decompress(&cinfo);
    if (!InitDecode()) {
      return false;
    }
  }
  if (setjmp(m_JmpBuf) == -1) {
    return false;
  }
  cinfo.scale_denom = m_nDefaultScaleDenom;
  m_OutputWidth = m_OrigWidth;
  m_OutputHeight = m_OrigHeight;
  if (!jpeg_start_decompress(&cinfo)) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }
  if ((int)cinfo.output_width > m_OrigWidth) {
    NOTREACHED();
    return false;
  }
  m_bStarted = true;
  return true;
}

uint8_t* CCodec_JpegDecoder::v_GetNextLine() {
  if (setjmp(m_JmpBuf) == -1)
    return nullptr;

  int nlines = jpeg_read_scanlines(&cinfo, &m_pScanlineBuf, 1);
  return nlines > 0 ? m_pScanlineBuf : nullptr;
}

uint32_t CCodec_JpegDecoder::GetSrcOffset() {
  return (uint32_t)(m_SrcSize - src.bytes_in_buffer);
}

std::unique_ptr<CCodec_ScanlineDecoder> CCodec_JpegModule::CreateDecoder(
    const uint8_t* src_buf,
    uint32_t src_size,
    int width,
    int height,
    int nComps,
    bool ColorTransform) {
  if (!src_buf || src_size == 0)
    return nullptr;

  auto pDecoder = pdfium::MakeUnique<CCodec_JpegDecoder>();
  if (!pDecoder->Create(src_buf, src_size, width, height, nComps,
                        ColorTransform)) {
    return nullptr;
  }
  return std::move(pDecoder);
}

bool CCodec_JpegModule::LoadInfo(const uint8_t* src_buf,
                                 uint32_t src_size,
                                 int* width,
                                 int* height,
                                 int* num_components,
                                 int* bits_per_components,
                                 bool* color_transform) {
  return JpegLoadInfo(src_buf, src_size, width, height, num_components,
                      bits_per_components, color_transform);
}

extern "C" {

static void _error_fatal1(j_common_ptr cinfo) {
  auto* pContext = reinterpret_cast<CJpegContext*>(cinfo->client_data);
  longjmp(pContext->m_JumpMark, -1);
}

static void _src_skip_data1(struct jpeg_decompress_struct* cinfo, long num) {
  if (cinfo->src->bytes_in_buffer < static_cast<size_t>(num)) {
    auto* pContext = reinterpret_cast<CJpegContext*>(cinfo->client_data);
    pContext->m_SkipSize = (unsigned int)(num - cinfo->src->bytes_in_buffer);
    cinfo->src->bytes_in_buffer = 0;
  } else {
    cinfo->src->next_input_byte += num;
    cinfo->src->bytes_in_buffer -= num;
  }
}

static void* jpeg_alloc_func(unsigned int size) {
  return FX_Alloc(char, size);
}

static void jpeg_free_func(void* p) {
  FX_Free(p);
}

}  // extern "C"

CJpegContext::CJpegContext()
    : m_SkipSize(0), m_AllocFunc(jpeg_alloc_func), m_FreeFunc(jpeg_free_func) {
  memset(&m_Info, 0, sizeof(m_Info));
  m_Info.client_data = this;
  m_Info.err = &m_ErrMgr;

  memset(&m_ErrMgr, 0, sizeof(m_ErrMgr));
  m_ErrMgr.error_exit = _error_fatal1;
  m_ErrMgr.emit_message = _error_do_nothing1;
  m_ErrMgr.output_message = _error_do_nothing;
  m_ErrMgr.format_message = _error_do_nothing2;
  m_ErrMgr.reset_error_mgr = _error_do_nothing;

  memset(&m_SrcMgr, 0, sizeof(m_SrcMgr));
  m_SrcMgr.init_source = _src_do_nothing;
  m_SrcMgr.term_source = _src_do_nothing;
  m_SrcMgr.skip_input_data = _src_skip_data1;
  m_SrcMgr.fill_input_buffer = _src_fill_buffer;
  m_SrcMgr.resync_to_restart = _src_resync;
}

CJpegContext::~CJpegContext() {
  jpeg_destroy_decompress(&m_Info);
}

std::unique_ptr<CCodec_JpegModule::Context> CCodec_JpegModule::Start() {
  // Use ordinary pointer until past the possibility of a longjump.
  auto* pContext = new CJpegContext();
  if (setjmp(pContext->m_JumpMark) == -1) {
    delete pContext;
    return nullptr;
  }

  jpeg_create_decompress(&pContext->m_Info);
  pContext->m_Info.src = &pContext->m_SrcMgr;
  pContext->m_SkipSize = 0;
  return pdfium::WrapUnique(pContext);
}

void CCodec_JpegModule::Input(Context* pContext,
                              const unsigned char* src_buf,
                              uint32_t src_size) {
  auto* ctx = static_cast<CJpegContext*>(pContext);
  if (ctx->m_SkipSize) {
    if (ctx->m_SkipSize > src_size) {
      ctx->m_SrcMgr.bytes_in_buffer = 0;
      ctx->m_SkipSize -= src_size;
      return;
    }
    src_size -= ctx->m_SkipSize;
    src_buf += ctx->m_SkipSize;
    ctx->m_SkipSize = 0;
  }
  ctx->m_SrcMgr.next_input_byte = src_buf;
  ctx->m_SrcMgr.bytes_in_buffer = src_size;
}

#ifdef PDF_ENABLE_XFA
int CCodec_JpegModule::ReadHeader(Context* pContext,
                                  int* width,
                                  int* height,
                                  int* nComps,
                                  CFX_DIBAttribute* pAttribute) {
#else   // PDF_ENABLE_XFA
int CCodec_JpegModule::ReadHeader(Context* pContext,
                                  int* width,
                                  int* height,
                                  int* nComps) {
#endif  // PDF_ENABLE_XFA
  auto* ctx = static_cast<CJpegContext*>(pContext);
  int ret = jpeg_read_header(&ctx->m_Info, true);
  if (ret == JPEG_SUSPENDED)
    return 2;
  if (ret != JPEG_HEADER_OK)
    return 1;

  *width = ctx->m_Info.image_width;
  *height = ctx->m_Info.image_height;
  *nComps = ctx->m_Info.num_components;
#ifdef PDF_ENABLE_XFA
  JpegLoadAttribute(&ctx->m_Info, pAttribute);
#endif
  return 0;
}

bool CCodec_JpegModule::StartScanline(Context* pContext, int down_scale) {
  auto* ctx = static_cast<CJpegContext*>(pContext);
  ctx->m_Info.scale_denom = static_cast<unsigned int>(down_scale);
  return !!jpeg_start_decompress(&ctx->m_Info);
}

bool CCodec_JpegModule::ReadScanline(Context* pContext,
                                     unsigned char* dest_buf) {
  auto* ctx = static_cast<CJpegContext*>(pContext);
  unsigned int nlines = jpeg_read_scanlines(&ctx->m_Info, &dest_buf, 1);
  return nlines == 1;
}

uint32_t CCodec_JpegModule::GetAvailInput(Context* pContext,
                                          uint8_t** avail_buf_ptr) {
  auto* ctx = static_cast<CJpegContext*>(pContext);
  if (avail_buf_ptr) {
    *avail_buf_ptr = nullptr;
    if (ctx->m_SrcMgr.bytes_in_buffer > 0) {
      *avail_buf_ptr = (uint8_t*)ctx->m_SrcMgr.next_input_byte;
    }
  }
  return (uint32_t)ctx->m_SrcMgr.bytes_in_buffer;
}

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
#define JPEG_BLOCK_SIZE 1048576
bool CCodec_JpegModule::JpegEncode(const RetainPtr<CFX_DIBSource>& pSource,
                                   uint8_t** dest_buf,
                                   size_t* dest_size) {
  struct jpeg_error_mgr jerr;
  jerr.error_exit = _error_do_nothing;
  jerr.emit_message = _error_do_nothing1;
  jerr.output_message = _error_do_nothing;
  jerr.format_message = _error_do_nothing2;
  jerr.reset_error_mgr = _error_do_nothing;

  struct jpeg_compress_struct cinfo;
  memset(&cinfo, 0, sizeof(cinfo));
  cinfo.err = &jerr;
  jpeg_create_compress(&cinfo);
  int Bpp = pSource->GetBPP() / 8;
  uint32_t nComponents = Bpp >= 3 ? (pSource->IsCmykImage() ? 4 : 3) : 1;
  uint32_t pitch = pSource->GetPitch();
  uint32_t width = pdfium::base::checked_cast<uint32_t>(pSource->GetWidth());
  uint32_t height = pdfium::base::checked_cast<uint32_t>(pSource->GetHeight());
  FX_SAFE_UINT32 safe_buf_len = width;
  safe_buf_len *= height;
  safe_buf_len *= nComponents;
  safe_buf_len += 1024;
  if (!safe_buf_len.IsValid())
    return false;

  uint32_t dest_buf_length = safe_buf_len.ValueOrDie();
  *dest_buf = FX_TryAlloc(uint8_t, dest_buf_length);
  const int MIN_TRY_BUF_LEN = 1024;
  while (!(*dest_buf) && dest_buf_length > MIN_TRY_BUF_LEN) {
    dest_buf_length >>= 1;
    *dest_buf = FX_TryAlloc(uint8_t, dest_buf_length);
  }
  if (!(*dest_buf))
    return false;

  struct jpeg_destination_mgr dest;
  dest.init_destination = _dest_do_nothing;
  dest.term_destination = _dest_do_nothing;
  dest.empty_output_buffer = _dest_empty;
  dest.next_output_byte = *dest_buf;
  dest.free_in_buffer = dest_buf_length;
  cinfo.dest = &dest;
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = nComponents;
  if (nComponents == 1) {
    cinfo.in_color_space = JCS_GRAYSCALE;
  } else if (nComponents == 3) {
    cinfo.in_color_space = JCS_RGB;
  } else {
    cinfo.in_color_space = JCS_CMYK;
  }
  uint8_t* line_buf = nullptr;
  if (nComponents > 1)
    line_buf = FX_Alloc2D(uint8_t, width, nComponents);

  jpeg_set_defaults(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);
  JSAMPROW row_pointer[1];
  JDIMENSION row;
  while (cinfo.next_scanline < cinfo.image_height) {
    const uint8_t* src_scan = pSource->GetScanline(cinfo.next_scanline);
    if (nComponents > 1) {
      uint8_t* dest_scan = line_buf;
      if (nComponents == 3) {
        for (uint32_t i = 0; i < width; i++) {
          dest_scan[0] = src_scan[2];
          dest_scan[1] = src_scan[1];
          dest_scan[2] = src_scan[0];
          dest_scan += 3;
          src_scan += Bpp;
        }
      } else {
        for (uint32_t i = 0; i < pitch; i++) {
          *dest_scan++ = ~*src_scan++;
        }
      }
      row_pointer[0] = line_buf;
    } else {
      row_pointer[0] = (uint8_t*)src_scan;
    }
    row = cinfo.next_scanline;
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
    if (cinfo.next_scanline == row) {
      *dest_buf =
          FX_Realloc(uint8_t, *dest_buf, dest_buf_length + JPEG_BLOCK_SIZE);
      dest.next_output_byte = *dest_buf + dest_buf_length - dest.free_in_buffer;
      dest_buf_length += JPEG_BLOCK_SIZE;
      dest.free_in_buffer += JPEG_BLOCK_SIZE;
    }
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  FX_Free(line_buf);
  *dest_size = dest_buf_length - static_cast<size_t>(dest.free_in_buffer);

  return true;
}
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
