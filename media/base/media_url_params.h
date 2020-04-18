// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_MEDIA_URL_PARAMS_H_
#define MEDIA_BASE_MEDIA_URL_PARAMS_H_

#include "url/gurl.h"

namespace media {

// Encapsulates the necessary information in order to play media in URL based
// playback (as opposed to stream based).
// See MediaUrlDemuxer and MediaPlayerRenderer.
struct MEDIA_EXPORT MediaUrlParams {
  // URL of the media to be played.
  GURL media_url;

  // Used to play media in authenticated scenarios.
  // NOTE: This URL is not the first party cookies, but the first party URL
  // returned by blink::WebDocument::firstPartyForCookies().
  // In the MediaPlayerRenderer case, it will ultimately be used in
  // MediaResourceGetterTask::CheckPolicyForCookies, to limit the scope of the
  // cookies that the MediaPlayerRenderer has access to.
  GURL site_for_cookies;
};

}  // namespace media

#endif  // MEDIA_BASE_MEDIA_URL_PARAMS_H_
