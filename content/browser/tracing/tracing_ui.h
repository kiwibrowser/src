// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_TRACING_UI_H_
#define CONTENT_BROWSER_TRACING_TRACING_UI_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/trace_uploader.h"
#include "content/public/browser/web_ui_controller.h"

namespace base {
namespace trace_event {
class TraceConfig;
}  // namespace trace_event
}  // namespace base

namespace content {

class TracingDelegate;

// The C++ back-end for the chrome://tracing webui page.
class CONTENT_EXPORT TracingUI : public WebUIController {
 public:
  explicit TracingUI(WebUI* web_ui);
  ~TracingUI() override;

  // Public for testing.
  static bool GetTracingOptions(const std::string& data64,
                                base::trace_event::TraceConfig* trace_config);

  void OnTraceUploadProgress(int64_t current, int64_t total);
  void OnTraceUploadComplete(bool success, const std::string& feedback);

 private:
  void DoUploadInternal(const std::string& file_contents,
                        TraceUploader::UploadMode upload_mode);
  void DoUpload(const base::ListValue* args);
  void DoUploadBase64Encoded(const base::ListValue* args);

  std::unique_ptr<TracingDelegate> delegate_;
  std::unique_ptr<TraceUploader> trace_uploader_;
  base::WeakPtrFactory<TracingUI> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TracingUI);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_TRACING_UI_H_
