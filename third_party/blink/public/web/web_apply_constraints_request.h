// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_APPLY_CONSTRAINTS_REQUEST_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_APPLY_CONSTRAINTS_REQUEST_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_private_ptr.h"

namespace blink {

class ApplyConstraintsRequest;
class WebMediaConstraints;
class WebMediaStreamTrack;
class WebString;

class BLINK_EXPORT WebApplyConstraintsRequest {
 public:
  WebApplyConstraintsRequest() = default;
  WebApplyConstraintsRequest(const WebApplyConstraintsRequest& other) {
    Assign(other);
  }
  WebApplyConstraintsRequest& operator=(
      const WebApplyConstraintsRequest& other) {
    Assign(other);
    return *this;
  }
  ~WebApplyConstraintsRequest();

  bool operator==(const WebApplyConstraintsRequest& other) const;

  void Reset();
  bool IsNull() const { return private_.IsNull(); }

  WebMediaStreamTrack Track() const;
  WebMediaConstraints Constraints() const;

  void RequestSucceeded();
  void RequestFailed(const WebString& constraint, const WebString& message);

  // For testing in content/
  static WebApplyConstraintsRequest CreateForTesting(
      const WebMediaStreamTrack&,
      const WebMediaConstraints&);

#if INSIDE_BLINK
  explicit WebApplyConstraintsRequest(ApplyConstraintsRequest*);
#endif

 private:
  void Assign(const WebApplyConstraintsRequest&);

  WebPrivatePtr<ApplyConstraintsRequest> private_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_APPLY_CONSTRAINTS_REQUEST_H_
