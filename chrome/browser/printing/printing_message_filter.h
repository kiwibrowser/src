// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_
#define CHROME_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service_shutdown_notifier.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "printing/buildflags/buildflags.h"

struct PrintHostMsg_ScriptedPrint_Params;
class Profile;

namespace base {
class DictionaryValue;
#if defined(OS_ANDROID)
struct FileDescriptor;
#endif
}

namespace printing {

class PrintQueriesQueue;
class PrinterQuery;

// This class filters out incoming printing related IPC messages for the
// renderer process on the IPC thread.
class PrintingMessageFilter : public content::BrowserMessageFilter {
 public:
  PrintingMessageFilter(int render_process_id, Profile* profile);

  // content::BrowserMessageFilter methods.
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;

  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  friend class base::DeleteHelper<PrintingMessageFilter>;
  friend class content::BrowserThread;

  ~PrintingMessageFilter() override;

  void OnDestruct() const override;

  void ShutdownOnUIThread();

#if defined(OS_ANDROID)
  // Used to ask the browser allocate a temporary file for the renderer
  // to fill in resulting PDF in renderer.
  void OnAllocateTempFileForPrinting(int render_frame_id,
                                     base::FileDescriptor* temp_file_fd,
                                     int* sequence_number);
  void OnTempFileForPrintingWritten(int render_frame_id,
                                    int sequence_number,
                                    int page_count);

  // Updates the file descriptor for the PrintViewManagerBasic of a given
  // |render_frame_id|.
  void UpdateFileDescriptor(int render_frame_id, int fd);
#endif

  // Get the default print setting.
  void OnGetDefaultPrintSettings(IPC::Message* reply_msg);
  void OnGetDefaultPrintSettingsReply(scoped_refptr<PrinterQuery> printer_query,
                                      IPC::Message* reply_msg);

  // The renderer host have to show to the user the print dialog and returns
  // the selected print settings. The task is handled by the print worker
  // thread and the UI thread. The reply occurs on the IO thread.
  void OnScriptedPrint(const PrintHostMsg_ScriptedPrint_Params& params,
                       IPC::Message* reply_msg);
  void OnScriptedPrintReply(scoped_refptr<PrinterQuery> printer_query,
                            IPC::Message* reply_msg);

  // Modify the current print settings based on |job_settings|. The task is
  // handled by the print worker thread and the UI thread. The reply occurs on
  // the IO thread.
  void OnUpdatePrintSettings(int document_cookie,
                             const base::DictionaryValue& job_settings,
                             IPC::Message* reply_msg);
  void OnUpdatePrintSettingsReply(scoped_refptr<PrinterQuery> printer_query,
                                  IPC::Message* reply_msg);

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // Check to see if print preview has been cancelled.
  void OnCheckForCancel(int32_t preview_ui_id,
                        int preview_request_id,
                        bool* cancel);
#if defined(OS_WIN)
  void NotifySystemDialogCancelled(int routing_id);
#endif
#endif

  std::unique_ptr<KeyedServiceShutdownNotifier::Subscription>
      printing_shutdown_notifier_;

  BooleanPrefMember is_printing_enabled_;

  const int render_process_id_;

  scoped_refptr<PrintQueriesQueue> queue_;

  DISALLOW_COPY_AND_ASSIGN(PrintingMessageFilter);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_
