// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_error_util.h"

#include "third_party/blink/renderer/core/dom/dom_exception.h"

namespace blink {

DOMException* CreateDOMExceptionFromRTCError(const webrtc::RTCError& error) {
  switch (error.type()) {
    case webrtc::RTCErrorType::NONE:
      // This should never happen.
      NOTREACHED();
      break;
    case webrtc::RTCErrorType::SYNTAX_ERROR:
      return DOMException::Create(kSyntaxError, error.message());
    case webrtc::RTCErrorType::INVALID_MODIFICATION:
      return DOMException::Create(kInvalidModificationError, error.message());
    case webrtc::RTCErrorType::NETWORK_ERROR:
      return DOMException::Create(kNetworkError, error.message());
    case webrtc::RTCErrorType::UNSUPPORTED_PARAMETER:
    case webrtc::RTCErrorType::UNSUPPORTED_OPERATION:
    case webrtc::RTCErrorType::RESOURCE_EXHAUSTED:
    case webrtc::RTCErrorType::INTERNAL_ERROR:
      return DOMException::Create(kOperationError, error.message());
    case webrtc::RTCErrorType::INVALID_STATE:
      return DOMException::Create(kInvalidStateError, error.message());
    case webrtc::RTCErrorType::INVALID_PARAMETER:
      // One use of this value is to signal invalid SDP syntax.
      // According to spec, this should return an RTCError with name
      // "RTCError" and detail "sdp-syntax-error", with
      // "sdpLineNumber" set to indicate the line where the error
      // occured.
      // TODO(https://crbug.com/821806): Implement the RTCError object.
      return DOMException::Create(kInvalidAccessError, error.message());
    case webrtc::RTCErrorType::INVALID_RANGE:
      return DOMException::Create(kV8RangeError, error.message());
    default:
      LOG(ERROR) << "Got unhandled RTC error "
                 << static_cast<int>(error.type());
      // No DOM equivalent. Needs per-error evaluation.
      NOTREACHED();
      break;
  }
  NOTREACHED();
  return nullptr;
}
}  // namespace blink
