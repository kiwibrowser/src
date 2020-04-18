/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_USER_MEDIA_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_USER_MEDIA_CLIENT_H_

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/modules/mediastream/user_media_client.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class ApplyConstraintsRequest;
class LocalFrame;
class MediaStreamComponent;
class UserMediaRequest;
class WebUserMediaClient;

class MODULES_EXPORT UserMediaClient {
 public:
  static std::unique_ptr<UserMediaClient> Create(WebUserMediaClient* client) {
    return base::WrapUnique(new UserMediaClient(client));
  }

  void RequestUserMedia(UserMediaRequest*);
  void CancelUserMediaRequest(UserMediaRequest*);
  void ApplyConstraints(ApplyConstraintsRequest*);
  void StopTrack(MediaStreamComponent*);

 private:
  explicit UserMediaClient(WebUserMediaClient*);

  WebUserMediaClient* client_;
};

MODULES_EXPORT void ProvideUserMediaTo(LocalFrame&,
                                       std::unique_ptr<UserMediaClient>);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_USER_MEDIA_CLIENT_H_
