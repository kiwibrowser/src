// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_TEXT_INPUT_CLIENT_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_TEXT_INPUT_CLIENT_MESSAGE_FILTER_H_

#include <stddef.h>

#include "base/macros.h"
#include "content/common/mac/attributed_string_coder.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"

namespace gfx {
class Point;
class Rect;
}

namespace content {

// This is a browser-side message filter that lives on the IO thread to handle
// replies to messages sent by the TextInputClientMac. See
// content/browser/renderer_host/text_input_client_mac.h for more information.
class CONTENT_EXPORT TextInputClientMessageFilter
    : public BrowserMessageFilter {
 public:
  TextInputClientMessageFilter();

  // BrowserMessageFilter override:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OverrideThreadForMessage(const IPC::Message& message,
                                BrowserThread::ID* thread) override;

 protected:
  ~TextInputClientMessageFilter() override;

 private:
  // IPC Message handlers:
  void OnGotStringAtPoint(
      const mac::AttributedStringCoder::EncodedString& encoded_string,
      const gfx::Point& point);
  void OnGotCharacterIndexForPoint(uint32_t index);
  void OnGotFirstRectForRange(const gfx::Rect& rect);
  void OnGotStringFromRange(
      const mac::AttributedStringCoder::EncodedString& string,
      const gfx::Point& point);

  DISALLOW_COPY_AND_ASSIGN(TextInputClientMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_TEXT_INPUT_CLIENT_MESSAGE_FILTER_H_
