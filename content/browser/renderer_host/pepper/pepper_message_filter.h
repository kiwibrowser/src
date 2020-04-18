// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_MESSAGE_FILTER_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/browser_message_filter.h"

namespace ppapi {
class PPB_X509Certificate_Fields;
}

namespace content {

// Message filter that handles IPC for PPB_X509Certificate_Private.
class PepperMessageFilter : public BrowserMessageFilter {
 public:
  PepperMessageFilter();

  // BrowserMessageFilter methods.
  bool OnMessageReceived(const IPC::Message& message) override;

 protected:
  ~PepperMessageFilter() override;

 private:
  void OnX509CertificateParseDER(const std::vector<char>& der,
                                 bool* succeeded,
                                 ppapi::PPB_X509Certificate_Fields* result);

  DISALLOW_COPY_AND_ASSIGN(PepperMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_MESSAGE_FILTER_H_
