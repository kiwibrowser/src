/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/web/web_user_media_request.h"

#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/renderer/modules/mediastream/user_media_request.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_descriptor.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_source.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

WebUserMediaRequest::WebUserMediaRequest(UserMediaRequest* request)
    : private_(request) {}

WebUserMediaRequest WebUserMediaRequest::CreateForTesting(
    const WebMediaConstraints& audio,
    const WebMediaConstraints& video) {
  UserMediaRequest* request = UserMediaRequest::CreateForTesting(audio, video);
  return WebUserMediaRequest(request);
}

void WebUserMediaRequest::Reset() {
  private_.Reset();
}

bool WebUserMediaRequest::Audio() const {
  DCHECK(!IsNull());
  return private_->Audio();
}

bool WebUserMediaRequest::Video() const {
  DCHECK(!IsNull());
  return private_->Video();
}

WebMediaConstraints WebUserMediaRequest::AudioConstraints() const {
  DCHECK(!IsNull());
  return private_->AudioConstraints();
}

WebMediaConstraints WebUserMediaRequest::VideoConstraints() const {
  DCHECK(!IsNull());
  return private_->VideoConstraints();
}

bool WebUserMediaRequest::ShouldDisableHardwareNoiseSuppression() const {
  DCHECK(!IsNull());
  return private_->ShouldDisableHardwareNoiseSuppression();
}

WebSecurityOrigin WebUserMediaRequest::GetSecurityOrigin() const {
  DCHECK(!IsNull());
  if (!private_->GetExecutionContext())
    return WebSecurityOrigin::CreateFromString("test://test");
  return WebSecurityOrigin(
      private_->GetExecutionContext()->GetSecurityOrigin());
}

WebDocument WebUserMediaRequest::OwnerDocument() const {
  DCHECK(!IsNull());
  return WebDocument(private_->OwnerDocument());
}

void WebUserMediaRequest::RequestSucceeded(
    const WebMediaStream& stream_descriptor) {
  DCHECK(!IsNull());
  DCHECK(!stream_descriptor.IsNull());
  private_->Succeed(stream_descriptor);
}

void WebUserMediaRequest::RequestFailedConstraint(
    const WebString& constraint_name,
    const WebString& description) {
  DCHECK(!IsNull());
  private_->FailConstraint(constraint_name, description);
}

void WebUserMediaRequest::RequestFailed(Error name,
                                        const WebString& description) {
  DCHECK(!IsNull());
  private_->Fail(name, description);
}

bool WebUserMediaRequest::Equals(const WebUserMediaRequest& other) const {
  if (IsNull() || other.IsNull())
    return false;
  return private_.Get() == other.private_.Get();
}

void WebUserMediaRequest::Assign(const WebUserMediaRequest& other) {
  private_ = other.private_;
}

WebUserMediaRequest::operator UserMediaRequest*() const {
  return private_.Get();
}

}  // namespace blink
