// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_message_filter.h"

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/pepper_socket_utils.h"
#include "content/public/common/content_client.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

PepperMessageFilter::PepperMessageFilter()
    : BrowserMessageFilter(PpapiMsgStart) {}

PepperMessageFilter::~PepperMessageFilter() {}

bool PepperMessageFilter::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PepperMessageFilter, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBX509Certificate_ParseDER,
                        OnX509CertificateParseDER)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PepperMessageFilter::OnX509CertificateParseDER(
    const std::vector<char>& der,
    bool* succeeded,
    ppapi::PPB_X509Certificate_Fields* result) {
  *succeeded = (der.size() != 0 && pepper_socket_utils::GetCertificateFields(
                                       &der[0], der.size(), result));
}

}  // namespace content
