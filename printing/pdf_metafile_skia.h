// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_PDF_METAFILE_SKIA_H_
#define PRINTING_PDF_METAFILE_SKIA_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "build/build_config.h"
#include "cc/paint/paint_canvas.h"
#include "printing/common/pdf_metafile_utils.h"
#include "printing/metafile.h"
#include "skia/ext/platform_canvas.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace printing {

struct PdfMetafileSkiaData;

// This class uses Skia graphics library to generate a PDF or MSKP document.
// TODO(thestig): Rename to MetafileSkia.
class PRINTING_EXPORT PdfMetafileSkia : public Metafile {
 public:
  // Default constructor, for SkiaDocumentType::PDF type only.
  // TODO(weili): we should split up this use case into a different class, see
  //              comments before InitFromData()'s implementation.
  PdfMetafileSkia();
  PdfMetafileSkia(SkiaDocumentType type, int document_cookie);
  ~PdfMetafileSkia() override;

  // Metafile methods.
  bool Init() override;
  bool InitFromData(const void* src_buffer, size_t src_buffer_size) override;

  void StartPage(const gfx::Size& page_size,
                 const gfx::Rect& content_area,
                 const float& scale_factor) override;
  bool FinishPage() override;
  bool FinishDocument() override;

  uint32_t GetDataSize() const override;
  bool GetData(void* dst_buffer, uint32_t dst_buffer_size) const override;

  gfx::Rect GetPageBounds(unsigned int page_number) const override;
  unsigned int GetPageCount() const override;

  printing::NativeDrawingContext context() const override;

#if defined(OS_WIN)
  bool Playback(printing::NativeDrawingContext hdc,
                const RECT* rect) const override;
  bool SafePlayback(printing::NativeDrawingContext hdc) const override;
#elif defined(OS_MACOSX)
  bool RenderPage(unsigned int page_number,
                  printing::NativeDrawingContext context,
                  const CGRect rect,
                  const MacRenderPageParams& params) const override;
#endif

  bool SaveTo(base::File* file) const override;

  // Unlike FinishPage() or FinishDocument(), this is for out-of-process
  // subframe printing. It will just serialize the content into SkPicture
  // format and store it as final data.
  void FinishFrameContent();

  // Return a new metafile containing just the current page in draft mode.
  std::unique_ptr<PdfMetafileSkia> GetMetafileForCurrentPage(
      SkiaDocumentType type);

  // This method calls StartPage and then returns an appropriate
  // PlatformCanvas implementation bound to the context created by
  // StartPage or NULL on error.  The PaintCanvas pointer that
  // is returned is owned by this PdfMetafileSkia object and does not
  // need to be ref()ed or unref()ed.  The canvas will remain valid
  // until FinishPage() or FinishDocument() is called.
  cc::PaintCanvas* GetVectorCanvasForNewPage(const gfx::Size& page_size,
                                             const gfx::Rect& content_area,
                                             const float& scale_factor);

  // This is used for painting content of out-of-process subframes.
  // For such a subframe, since the content is in another process, we create a
  // place holder picture now, and replace it with actual content by pdf
  // compositor service later.
  uint32_t CreateContentForRemoteFrame(const gfx::Rect& rect,
                                       int render_proxy_id);

  int GetDocumentCookie() const;
  const ContentToProxyIdMap& GetSubframeContentInfo() const;

 private:
  // Callback function used during page content drawing to replace a custom
  // data holder with corresponding place holder SkPicture.
  void CustomDataToSkPictureCallback(SkCanvas* canvas, uint32_t content_id);

  std::unique_ptr<PdfMetafileSkiaData> data_;

  DISALLOW_COPY_AND_ASSIGN(PdfMetafileSkia);
};

}  // namespace printing

#endif  // PRINTING_PDF_METAFILE_SKIA_H_
