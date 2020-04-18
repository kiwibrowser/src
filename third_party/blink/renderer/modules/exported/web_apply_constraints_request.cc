// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/web_apply_constraints_request.h"

#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/modules/mediastream/apply_constraints_request.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

WebApplyConstraintsRequest::WebApplyConstraintsRequest(
    ApplyConstraintsRequest* request)
    : private_(request) {}

WebApplyConstraintsRequest::~WebApplyConstraintsRequest() {
  Reset();
}

WebApplyConstraintsRequest WebApplyConstraintsRequest::CreateForTesting(
    const WebMediaStreamTrack& track,
    const WebMediaConstraints& constraints) {
  return WebApplyConstraintsRequest(
      ApplyConstraintsRequest::CreateForTesting(track, constraints));
}

void WebApplyConstraintsRequest::Reset() {
  private_.Reset();
}

WebMediaStreamTrack WebApplyConstraintsRequest::Track() const {
  DCHECK(!IsNull());
  return private_->Track();
}

WebMediaConstraints WebApplyConstraintsRequest::Constraints() const {
  DCHECK(!IsNull());
  return private_->Constraints();
}

void WebApplyConstraintsRequest::RequestSucceeded() {
  DCHECK(!IsNull());
  private_->RequestSucceeded();
}

void WebApplyConstraintsRequest::RequestFailed(const WebString& constraint,
                                               const WebString& message) {
  DCHECK(!IsNull());
  private_->RequestFailed(constraint, message);
}

bool WebApplyConstraintsRequest::operator==(
    const WebApplyConstraintsRequest& other) const {
  return private_.Get() == other.private_.Get();
}

void WebApplyConstraintsRequest::Assign(
    const WebApplyConstraintsRequest& other) {
  private_ = other.private_;
}

}  // namespace blink
