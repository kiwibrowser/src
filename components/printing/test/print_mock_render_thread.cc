// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/test/print_mock_render_thread.h"

#include <stddef.h>

#include <vector>

#include "base/values.h"
#include "build/build_config.h"
#include "components/printing/test/mock_printer.h"
#include "ipc/ipc_sync_message.h"
#include "printing/buildflags/buildflags.h"
#include "printing/page_range.h"
#include "printing/print_job_constants.h"
#include "printing/units.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/common/print_messages.h"
#endif

PrintMockRenderThread::PrintMockRenderThread()
#if BUILDFLAG(ENABLE_PRINTING)
    : printer_(new MockPrinter),
      print_dialog_user_response_(true),
      print_preview_cancel_page_number_(-1),
      print_preview_pages_remaining_(0)
#endif
{
}

PrintMockRenderThread::~PrintMockRenderThread() {
}

scoped_refptr<base::SingleThreadTaskRunner>
PrintMockRenderThread::GetIOTaskRunner() {
  return io_task_runner_;
}

void PrintMockRenderThread::set_io_task_runner(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  io_task_runner_ = task_runner;
}

bool PrintMockRenderThread::OnMessageReceived(const IPC::Message& msg) {
  if (content::MockRenderThread::OnMessageReceived(msg))
    return true;

  // Some messages we do special handling.
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintMockRenderThread, msg)
#if BUILDFLAG(ENABLE_PRINTING)
    IPC_MESSAGE_HANDLER(PrintHostMsg_GetDefaultPrintSettings,
                        OnGetDefaultPrintSettings)
    IPC_MESSAGE_HANDLER(PrintHostMsg_ScriptedPrint, OnScriptedPrint)
    IPC_MESSAGE_HANDLER(PrintHostMsg_UpdatePrintSettings, OnUpdatePrintSettings)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetPrintedPagesCount,
                        OnDidGetPrintedPagesCount)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidPrintDocument, OnDidPrintDocument)
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetPreviewPageCount,
                        OnDidGetPreviewPageCount)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidPreviewPage, OnDidPreviewPage)
    IPC_MESSAGE_HANDLER(PrintHostMsg_CheckForCancel, OnCheckForCancel)
#endif
#endif  // BUILDFLAG(ENABLE_PRINTING)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

#if BUILDFLAG(ENABLE_PRINTING)

void PrintMockRenderThread::OnGetDefaultPrintSettings(
    PrintMsg_Print_Params* params) {
  printer_->GetDefaultPrintSettings(params);
}

void PrintMockRenderThread::OnScriptedPrint(
    const PrintHostMsg_ScriptedPrint_Params& params,
    PrintMsg_PrintPages_Params* settings) {
  if (print_dialog_user_response_) {
    printer_->ScriptedPrint(params.cookie, params.expected_pages_count,
                            params.has_selection, settings);
  }
}

void PrintMockRenderThread::OnDidGetPrintedPagesCount(int cookie,
                                                      int number_pages) {
  printer_->SetPrintedPagesCount(cookie, number_pages);
}

void PrintMockRenderThread::OnDidPrintDocument(
    const PrintHostMsg_DidPrintDocument_Params& params) {
  printer_->PrintPage(params);
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void PrintMockRenderThread::OnDidGetPreviewPageCount(
    const PrintHostMsg_DidGetPreviewPageCount_Params& params) {
  print_preview_pages_remaining_ = params.page_count;
}

void PrintMockRenderThread::OnDidPreviewPage(
    const PrintHostMsg_DidPreviewPage_Params& params) {
  DCHECK_GE(params.page_number, printing::FIRST_PAGE_INDEX);
  print_preview_pages_remaining_--;
  print_preview_pages_.emplace_back(params.page_number,
                                    params.content.data_size);
}

void PrintMockRenderThread::OnCheckForCancel(int32_t preview_ui_id,
                                             int preview_request_id,
                                             bool* cancel) {
  *cancel =
      (print_preview_pages_remaining_ == print_preview_cancel_page_number_);
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

void PrintMockRenderThread::OnUpdatePrintSettings(
    int document_cookie,
    const base::DictionaryValue& job_settings,
    PrintMsg_PrintPages_Params* params,
    bool* canceled) {
  if (canceled)
    *canceled = false;
  // Check and make sure the required settings are all there.
  // We don't actually care about the values.
  std::string dummy_string;
  int margins_type = 0;
  if (!job_settings.GetBoolean(printing::kSettingLandscape, nullptr) ||
      !job_settings.GetBoolean(printing::kSettingCollate, nullptr) ||
      !job_settings.GetInteger(printing::kSettingColor, nullptr) ||
      !job_settings.GetBoolean(printing::kSettingPrintToPDF, nullptr) ||
      !job_settings.GetBoolean(printing::kIsFirstRequest, nullptr) ||
      !job_settings.GetString(printing::kSettingDeviceName, &dummy_string) ||
      !job_settings.GetInteger(printing::kSettingDuplexMode, nullptr) ||
      !job_settings.GetInteger(printing::kSettingCopies, nullptr) ||
      !job_settings.GetInteger(printing::kPreviewUIID, nullptr) ||
      !job_settings.GetInteger(printing::kPreviewRequestID, nullptr) ||
      !job_settings.GetInteger(printing::kSettingMarginsType, &margins_type)) {
    return;
  }

  // Just return the default settings.
  const base::ListValue* page_range_array;
  printing::PageRanges new_ranges;
  if (job_settings.GetList(printing::kSettingPageRange, &page_range_array)) {
    for (size_t index = 0; index < page_range_array->GetSize(); ++index) {
      const base::DictionaryValue* dict;
      if (!page_range_array->GetDictionary(index, &dict))
        continue;
      printing::PageRange range;
      if (!dict->GetInteger(printing::kSettingPageRangeFrom, &range.from) ||
          !dict->GetInteger(printing::kSettingPageRangeTo, &range.to)) {
        continue;
      }
      // Page numbers are 1-based in the dictionary.
      // Page numbers are 0-based for the printing context.
      range.from--;
      range.to--;
      new_ranges.push_back(range);
    }
  }

  // Get media size
  const base::DictionaryValue* media_size_value = nullptr;
  gfx::Size page_size;
  if (job_settings.GetDictionary(printing::kSettingMediaSize,
                                 &media_size_value)) {
    int width_microns = 0;
    int height_microns = 0;
    if (media_size_value->GetInteger(printing::kSettingMediaSizeWidthMicrons,
                                     &width_microns) &&
        media_size_value->GetInteger(printing::kSettingMediaSizeHeightMicrons,
                                     &height_microns)) {
      float device_microns_per_unit =
          (printing::kHundrethsMMPerInch * 10.0f) / printing::kDefaultPdfDpi;
      page_size = gfx::Size(width_microns / device_microns_per_unit,
                            height_microns / device_microns_per_unit);
    }
  }

  // Get scaling
  int scale_factor = 100;
  job_settings.GetInteger(printing::kSettingScaleFactor, &scale_factor);

  std::vector<int> pages(printing::PageRange::GetPages(new_ranges));
  printer_->UpdateSettings(document_cookie, params, pages, margins_type,
                           page_size, scale_factor);
  job_settings.GetBoolean(printing::kSettingShouldPrintSelectionOnly,
                          &params->params.selection_only);
  job_settings.GetBoolean(printing::kSettingShouldPrintBackgrounds,
                          &params->params.should_print_backgrounds);
}

MockPrinter* PrintMockRenderThread::printer() {
  return printer_.get();
}

void PrintMockRenderThread::set_print_dialog_user_response(bool response) {
  print_dialog_user_response_ = response;
}

void PrintMockRenderThread::set_print_preview_cancel_page_number(int page) {
  print_preview_cancel_page_number_ = page;
}

int PrintMockRenderThread::print_preview_pages_remaining() const {
  return print_preview_pages_remaining_;
}

const std::vector<std::pair<int, uint32_t>>&
PrintMockRenderThread::print_preview_pages() const {
  return print_preview_pages_;
}
#endif  // BUILDFLAG(ENABLE_PRINTING)
