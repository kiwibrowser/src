/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_DATA_CHANNEL_HANDLER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_DATA_CHANNEL_HANDLER_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"
#include "third_party/blink/public/platform/web_rtc_data_channel_handler_client.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class WebRTCDataChannelHandler {
 public:
  virtual ~WebRTCDataChannelHandler() = default;

  virtual void SetClient(WebRTCDataChannelHandlerClient*) = 0;

  virtual WebString Label() = 0;

  // DEPRECATED
  virtual bool IsReliable() { return true; }

  virtual bool Ordered() const = 0;
  virtual unsigned short MaxRetransmitTime() const = 0;
  virtual unsigned short MaxRetransmits() const = 0;
  virtual WebString Protocol() const = 0;
  virtual bool Negotiated() const = 0;
  virtual unsigned short Id() const = 0;

  virtual WebRTCDataChannelHandlerClient::ReadyState GetState() const = 0;
  virtual unsigned long BufferedAmount() = 0;
  virtual bool SendStringData(const WebString&) = 0;
  virtual bool SendRawData(const char*, size_t) = 0;
  virtual void Close() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_DATA_CHANNEL_HANDLER_H_
