// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PAGE_CAPTURE_PAGE_CAPTURE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_PAGE_CAPTURE_PAGE_CAPTURE_API_H_

#include <stdint.h>

#include <string>

#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/common/extensions/api/page_capture.h"
#include "storage/browser/blob/shareable_file_reference.h"

namespace base {
class FilePath;
}

namespace content {
class WebContents;
}

namespace extensions {

class PageCaptureSaveAsMHTMLFunction : public ChromeAsyncExtensionFunction {
 public:
  PageCaptureSaveAsMHTMLFunction();

  // Test specific delegate used to test that the temporary file gets deleted.
  class TestDelegate {
   public:
    // Called on the UI thread when the temporary file that contains the
    // generated data has been created.
    virtual void OnTemporaryFileCreated(const base::FilePath& temp_file) = 0;
  };
  static void SetTestDelegate(TestDelegate* delegate);

 private:
  ~PageCaptureSaveAsMHTMLFunction() override;
  bool RunAsync() override;
  bool OnMessageReceived(const IPC::Message& message) override;

#if defined(OS_CHROMEOS)
  // Resolves the API permission request in Public Sessions.
  void ResolvePermissionRequest(const PermissionIDSet& allowed_permissions);
#endif

  // Called on the file thread.
  void CreateTemporaryFile();

  void TemporaryFileCreatedOnIO(bool success);
  void TemporaryFileCreatedOnUI(bool success);

  // Called on the UI thread.
  void ReturnFailure(const std::string& error);
  void ReturnSuccess(int64_t file_size);

  // Callback called once the MHTML generation is done.
  void MHTMLGenerated(int64_t mhtml_file_size);

  // Returns the WebContents we are associated with, NULL if it's been closed.
  content::WebContents* GetWebContents();

  std::unique_ptr<extensions::api::page_capture::SaveAsMHTML::Params> params_;

  // The path to the temporary file containing the MHTML data.
  base::FilePath mhtml_path_;

  // The file containing the MHTML.
  scoped_refptr<storage::ShareableFileReference> mhtml_file_;

  DECLARE_EXTENSION_FUNCTION("pageCapture.saveAsMHTML", PAGECAPTURE_SAVEASMHTML)
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_PAGE_CAPTURE_PAGE_CAPTURE_API_H_
