// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/renderer/print_render_frame_helper.h"

#include <stddef.h>

#include "base/logging.h"
#include "build/build_config.h"
#include "components/printing/common/print_messages.h"
#include "printing/buildflags/buildflags.h"
#include "printing/metafile_skia_wrapper.h"

#if defined(OS_ANDROID)
#include "base/file_descriptor_posix.h"
#else
#include "base/process/process_handle.h"
#endif  // defined(OS_ANDROID)

namespace {

#if defined(OS_ANDROID)
bool SaveToFD(const printing::Metafile& metafile,
              const base::FileDescriptor& fd) {
  DCHECK_GT(metafile.GetDataSize(), 0U);

  if (fd.fd < 0) {
    DLOG(ERROR) << "Invalid file descriptor!";
    return false;
  }
  base::File file(fd.fd);
  bool result = metafile.SaveTo(&file);
  DLOG_IF(ERROR, !result) << "Failed to save file with fd " << fd.fd;

  if (!fd.auto_close)
    file.TakePlatformFile();
  return result;
}
#endif  // defined(OS_ANDROID)

}  // namespace

namespace printing {

bool PrintRenderFrameHelper::PrintPagesNative(blink::WebLocalFrame* frame,
                                              int page_count,
                                              bool is_pdf) {
  const PrintMsg_PrintPages_Params& params = *print_pages_params_;
  const PrintMsg_Print_Params& print_params = params.params;

  std::vector<int> printed_pages = GetPrintedPages(params, page_count);
  if (printed_pages.empty())
    return false;

  PdfMetafileSkia metafile(print_params.printed_doc_type,
                           print_params.document_cookie);
  CHECK(metafile.Init());

  for (int page_number : printed_pages) {
    PrintPageInternal(print_params, page_number, page_count,
                      GetScaleFactor(print_params.scale_factor, is_pdf), frame,
                      &metafile, nullptr, nullptr);
  }

  // blink::printEnd() for PDF should be called before metafile is closed.
  FinishFramePrinting();

  metafile.FinishDocument();

#if defined(OS_ANDROID)
  int sequence_number = -1;
  base::FileDescriptor fd;

  // Ask the browser to open a file for us.
  Send(new PrintHostMsg_AllocateTempFileForPrinting(routing_id(), &fd,
                                                    &sequence_number));
  if (!SaveToFD(metafile, fd))
    return false;

  // Tell the browser we've finished writing the file.
  Send(new PrintHostMsg_TempFileForPrintingWritten(
      routing_id(), sequence_number, printed_pages.size()));
  return true;
#else
  PrintHostMsg_DidPrintDocument_Params page_params;
  if (!CopyMetafileDataToReadOnlySharedMem(metafile, &page_params.content)) {
    return false;
  }

  page_params.document_cookie = print_params.document_cookie;
  Send(new PrintHostMsg_DidPrintDocument(routing_id(), page_params));
  return true;
#endif  // defined(OS_ANDROID)
}

}  // namespace printing
