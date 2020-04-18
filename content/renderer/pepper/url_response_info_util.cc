// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/url_response_info_util.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/renderer/pepper/pepper_file_ref_renderer_host.h"
#include "content/renderer/pepper/renderer_ppapi_host_impl.h"
#include "ipc/ipc_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/url_response_info_data.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/public/platform/web_http_header_visitor.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_response.h"

using blink::WebHTTPHeaderVisitor;
using blink::WebString;
using blink::WebURLResponse;

namespace content {

namespace {

class HeadersToString : public WebHTTPHeaderVisitor {
 public:
  HeadersToString() {}
  ~HeadersToString() override {}

  const std::string& buffer() const { return buffer_; }

  void VisitHeader(const WebString& name, const WebString& value) override {
    if (!buffer_.empty())
      buffer_.append("\n");
    buffer_.append(name.Utf8());
    buffer_.append(": ");
    buffer_.append(value.Utf8());
  }

 private:
  std::string buffer_;
};

bool IsRedirect(int32_t status) { return status >= 300 && status <= 399; }

void DidCreateResourceHosts(const ppapi::URLResponseInfoData& in_data,
                            const base::FilePath& external_path,
                            int renderer_pending_host_id,
                            const DataFromWebURLResponseCallback& callback,
                            const std::vector<int>& browser_pending_host_ids) {
  DCHECK_EQ(1U, browser_pending_host_ids.size());
  int browser_pending_host_id = 0;

  if (browser_pending_host_ids.size() == 1)
    browser_pending_host_id = browser_pending_host_ids[0];

  ppapi::URLResponseInfoData data = in_data;

  data.body_as_file_ref =
      ppapi::MakeExternalFileRefCreateInfo(external_path,
                                           std::string(),
                                           browser_pending_host_id,
                                           renderer_pending_host_id);
  callback.Run(data);
}

}  // namespace

void DataFromWebURLResponse(RendererPpapiHostImpl* host_impl,
                            PP_Instance pp_instance,
                            const WebURLResponse& response,
                            const DataFromWebURLResponseCallback& callback) {
  ppapi::URLResponseInfoData data;
  data.url = response.Url().GetString().Utf8();
  data.status_code = response.HttpStatusCode();
  data.status_text = response.HttpStatusText().Utf8();
  if (IsRedirect(data.status_code)) {
    data.redirect_url =
        response.HttpHeaderField(WebString::FromUTF8("Location")).Utf8();
  }

  HeadersToString headers_to_string;
  response.VisitHTTPHeaderFields(&headers_to_string);
  data.headers = headers_to_string.buffer();

  WebString file_path = response.DownloadFilePath();
  if (!file_path.IsEmpty()) {
    base::FilePath external_path = blink::WebStringToFilePath(file_path);
    // TODO(teravest): Write a utility function to create resource hosts in the
    // renderer and browser.
    PepperFileRefRendererHost* renderer_host =
        new PepperFileRefRendererHost(host_impl, pp_instance, 0, external_path);
    int renderer_pending_host_id =
        host_impl->GetPpapiHost()->AddPendingResourceHost(
            std::unique_ptr<ppapi::host::ResourceHost>(renderer_host));

    std::vector<IPC::Message> create_msgs;
    create_msgs.push_back(PpapiHostMsg_FileRef_CreateForRawFS(external_path));
    host_impl->CreateBrowserResourceHosts(pp_instance,
                                          create_msgs,
                                          base::Bind(&DidCreateResourceHosts,
                                                     data,
                                                     external_path,
                                                     renderer_pending_host_id,
                                                     callback));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback, data));
  }
}

}  // namespace content
