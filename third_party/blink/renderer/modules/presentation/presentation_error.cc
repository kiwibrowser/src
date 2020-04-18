// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/presentation_error.h"

#include "third_party/blink/renderer/core/dom/exception_code.h"

namespace blink {

DOMException* CreatePresentationError(
    const mojom::blink::PresentationError& error) {
  ExceptionCode code = kUnknownError;
  switch (error.error_type) {
    case mojom::blink::PresentationErrorType::NO_AVAILABLE_SCREENS:
    case mojom::blink::PresentationErrorType::NO_PRESENTATION_FOUND:
      code = kNotFoundError;
      break;
    case mojom::blink::PresentationErrorType::PRESENTATION_REQUEST_CANCELLED:
      code = kNotAllowedError;
      break;
    case mojom::blink::PresentationErrorType::PREVIOUS_START_IN_PROGRESS:
      code = kOperationError;
      break;
    case mojom::blink::PresentationErrorType::UNKNOWN:
      code = kUnknownError;
      break;
  }

  return DOMException::Create(code, error.message);
}

}  // namespace blink
