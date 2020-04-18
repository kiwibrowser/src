/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (c) 2000 Daniel Molkentin (molkentin@kde.org)
 *  Copyright (c) 2000 Stefan Schimanski (schimmi@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "third_party/blink/renderer/modules/mediastream/navigator_media_stream.h"

#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_navigator_user_media_error_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_navigator_user_media_success_callback.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/modules/mediastream/media_error_state.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_constraints.h"
#include "third_party/blink/renderer/modules/mediastream/user_media_controller.h"
#include "third_party/blink/renderer/modules/mediastream/user_media_request.h"

namespace blink {

void NavigatorMediaStream::getUserMedia(
    Navigator& navigator,
    const MediaStreamConstraints& options,
    V8NavigatorUserMediaSuccessCallback* success_callback,
    V8NavigatorUserMediaErrorCallback* error_callback,
    ExceptionState& exception_state) {
  DCHECK(success_callback);
  DCHECK(error_callback);

  UserMediaController* user_media =
      UserMediaController::From(navigator.GetFrame());
  if (!user_media) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "No user media controller available; is this a detached window?");
    return;
  }

  MediaErrorState error_state;
  UserMediaRequest* request = UserMediaRequest::Create(
      navigator.GetFrame()->GetDocument(), user_media, options,
      success_callback, error_callback, error_state);
  if (!request) {
    DCHECK(error_state.HadException());
    if (error_state.CanGenerateException()) {
      error_state.RaiseException(exception_state);
    } else {
      error_callback->InvokeAndReportException(nullptr,
                                               error_state.CreateError());
    }
    return;
  }

  String error_message;
  if (!request->IsSecureContextUse(error_message)) {
    request->Fail(WebUserMediaRequest::Error::kSecurityError, error_message);
    return;
  }

  request->Start();
}

}  // namespace blink
