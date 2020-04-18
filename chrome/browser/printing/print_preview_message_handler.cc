// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_preview_message_handler.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/common/print_messages.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "printing/page_size_margins.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings.h"

using content::BrowserThread;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintPreviewMessageHandler);

namespace printing {

namespace {

void StopWorker(int document_cookie) {
  if (document_cookie <= 0)
    return;
  scoped_refptr<PrintQueriesQueue> queue =
      g_browser_process->print_job_manager()->queue();
  scoped_refptr<PrinterQuery> printer_query =
      queue->PopPrinterQuery(document_cookie);
  if (printer_query.get()) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&PrinterQuery::StopWorker, printer_query));
  }
}

scoped_refptr<base::RefCountedMemory> GetDataFromHandle(
    base::SharedMemoryHandle handle,
    uint32_t data_size) {
  auto shared_buf = std::make_unique<base::SharedMemory>(handle, true);
  if (!shared_buf->Map(data_size)) {
    NOTREACHED();
    return nullptr;
  }

  return base::MakeRefCounted<base::RefCountedSharedMemory>(
      std::move(shared_buf), data_size);
}

}  // namespace

PrintPreviewMessageHandler::PrintPreviewMessageHandler(
    WebContents* web_contents)
    : content::WebContentsObserver(web_contents), weak_ptr_factory_(this) {
  DCHECK(web_contents);
}

PrintPreviewMessageHandler::~PrintPreviewMessageHandler() {
}

WebContents* PrintPreviewMessageHandler::GetPrintPreviewDialog() {
  PrintPreviewDialogController* dialog_controller =
      PrintPreviewDialogController::GetInstance();
  if (!dialog_controller)
    return nullptr;
  return dialog_controller->GetPrintPreviewForContents(web_contents());
}

PrintPreviewUI* PrintPreviewMessageHandler::GetPrintPreviewUI() {
  WebContents* dialog = GetPrintPreviewDialog();
  if (!dialog || !dialog->GetWebUI())
    return nullptr;
  return static_cast<PrintPreviewUI*>(dialog->GetWebUI()->GetController());
}

void PrintPreviewMessageHandler::OnRequestPrintPreview(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_RequestPrintPreview_Params& params) {
  if (params.webnode_only) {
    PrintViewManager::FromWebContents(web_contents())->PrintPreviewForWebNode(
        render_frame_host);
  }
  PrintPreviewDialogController::PrintPreview(web_contents());
  PrintPreviewUI::SetInitialParams(GetPrintPreviewDialog(), params);
}

void PrintPreviewMessageHandler::OnDidGetPreviewPageCount(
    const PrintHostMsg_DidGetPreviewPageCount_Params& params) {
  if (params.page_count <= 0) {
    NOTREACHED();
    return;
  }

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;

  print_preview_ui->ClearAllPreviewData();
  print_preview_ui->OnDidGetPreviewPageCount(params);
}

void PrintPreviewMessageHandler::OnDidPreviewPage(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_DidPreviewPage_Params& params) {
  int page_number = params.page_number;
  const PrintHostMsg_DidPrintContent_Params& content = params.content;
  if (page_number < FIRST_PAGE_INDEX || !content.data_size)
    return;

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;

  if (IsOopifEnabled() && print_preview_ui->source_is_modifiable()) {
    auto* client = PrintCompositeClient::FromWebContents(web_contents());
    DCHECK(client);

    // Use utility process to convert skia metafile to pdf.
    client->DoCompositePageToPdf(
        params.document_cookie, render_frame_host, params.page_number,
        content.metafile_data_handle, content.data_size,
        content.subframe_content_info,
        base::BindOnce(&PrintPreviewMessageHandler::OnCompositePdfPageDone,
                       weak_ptr_factory_.GetWeakPtr(), params.page_number,
                       params.preview_request_id));
  } else {
    NotifyUIPreviewPageReady(
        page_number, params.preview_request_id,
        GetDataFromHandle(content.metafile_data_handle, content.data_size));
  }
}

void PrintPreviewMessageHandler::OnMetafileReadyForPrinting(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_DidPreviewDocument_Params& params) {
  // Always try to stop the worker.
  StopWorker(params.document_cookie);

  if (params.expected_pages_count <= 0) {
    NOTREACHED();
    return;
  }

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;

  const PrintHostMsg_DidPrintContent_Params& content = params.content;
  if (IsOopifEnabled() && print_preview_ui->source_is_modifiable()) {
    auto* client = PrintCompositeClient::FromWebContents(web_contents());
    DCHECK(client);

    client->DoCompositeDocumentToPdf(
        params.document_cookie, render_frame_host, content.metafile_data_handle,
        content.data_size, content.subframe_content_info,
        base::BindOnce(&PrintPreviewMessageHandler::OnCompositePdfDocumentDone,
                       weak_ptr_factory_.GetWeakPtr(),
                       params.expected_pages_count, params.preview_request_id));
  } else {
    NotifyUIPreviewDocumentReady(
        params.expected_pages_count, params.preview_request_id,
        GetDataFromHandle(content.metafile_data_handle, content.data_size));
  }
}

void PrintPreviewMessageHandler::OnPrintPreviewFailed(int document_cookie) {
  StopWorker(document_cookie);

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnPrintPreviewFailed();
}

void PrintPreviewMessageHandler::OnDidGetDefaultPageLayout(
    const PageSizeMargins& page_layout_in_points,
    const gfx::Rect& printable_area_in_points,
    bool has_custom_page_size_style) {
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnDidGetDefaultPageLayout(page_layout_in_points,
                                              printable_area_in_points,
                                              has_custom_page_size_style);
}

void PrintPreviewMessageHandler::OnPrintPreviewCancelled(int document_cookie) {
  // Always need to stop the worker.
  StopWorker(document_cookie);

  // Notify UI
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnPrintPreviewCancelled();
}

void PrintPreviewMessageHandler::OnInvalidPrinterSettings(int document_cookie) {
  StopWorker(document_cookie);
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnInvalidPrinterSettings();
}

void PrintPreviewMessageHandler::OnSetOptionsFromDocument(
    const PrintHostMsg_SetOptionsFromDocument_Params& params) {
  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->OnSetOptionsFromDocument(params);
}

void PrintPreviewMessageHandler::NotifyUIPreviewPageReady(
    int page_number,
    int request_id,
    scoped_refptr<base::RefCountedMemory> data_bytes) {
  DCHECK(data_bytes);

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->SetPrintPreviewDataForIndex(page_number,
                                                std::move(data_bytes));
  print_preview_ui->OnDidPreviewPage(page_number, request_id);
}

void PrintPreviewMessageHandler::NotifyUIPreviewDocumentReady(
    int page_count,
    int request_id,
    scoped_refptr<base::RefCountedMemory> data_bytes) {
  if (!data_bytes || !data_bytes->size())
    return;

  PrintPreviewUI* print_preview_ui = GetPrintPreviewUI();
  if (!print_preview_ui)
    return;
  print_preview_ui->SetPrintPreviewDataForIndex(COMPLETE_PREVIEW_DOCUMENT_INDEX,
                                                std::move(data_bytes));
  print_preview_ui->OnPreviewDataIsAvailable(page_count, request_id);
}

void PrintPreviewMessageHandler::OnCompositePdfPageDone(
    int page_number,
    int request_id,
    mojom::PdfCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (status != mojom::PdfCompositor::Status::SUCCESS) {
    DLOG(ERROR) << "Compositing pdf failed with error " << status;
    return;
  }
  NotifyUIPreviewPageReady(
      page_number, request_id,
      base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
}

void PrintPreviewMessageHandler::OnCompositePdfDocumentDone(
    int page_count,
    int request_id,
    mojom::PdfCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (status != mojom::PdfCompositor::Status::SUCCESS) {
    DLOG(ERROR) << "Compositing pdf failed with error " << status;
    return;
  }
  NotifyUIPreviewDocumentReady(
      page_count, request_id,
      base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
}

bool PrintPreviewMessageHandler::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PrintPreviewMessageHandler, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(PrintHostMsg_RequestPrintPreview,
                        OnRequestPrintPreview)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidPreviewPage, OnDidPreviewPage)
    IPC_MESSAGE_HANDLER(PrintHostMsg_MetafileReadyForPrinting,
                        OnMetafileReadyForPrinting)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  if (handled)
    return true;

  handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintPreviewMessageHandler, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetPreviewPageCount,
                        OnDidGetPreviewPageCount)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewFailed,
                        OnPrintPreviewFailed)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DidGetDefaultPageLayout,
                        OnDidGetDefaultPageLayout)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewCancelled,
                        OnPrintPreviewCancelled)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintPreviewInvalidPrinterSettings,
                        OnInvalidPrinterSettings)
    IPC_MESSAGE_HANDLER(PrintHostMsg_SetOptionsFromDocument,
                        OnSetOptionsFromDocument)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

}  // namespace printing
