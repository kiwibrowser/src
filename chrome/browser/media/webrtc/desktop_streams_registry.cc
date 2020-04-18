// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/desktop_streams_registry.h"

#include "base/base64.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/random.h"

namespace {

const int kStreamIdLengthBytes = 16;

const int kApprovedStreamTimeToLiveSeconds = 10;

std::string GenerateRandomStreamId() {
  char buffer[kStreamIdLengthBytes];
  crypto::RandBytes(buffer, arraysize(buffer));
  std::string result;
  base::Base64Encode(base::StringPiece(buffer, arraysize(buffer)),
                     &result);
  return result;
}

}  // namespace

DesktopStreamsRegistry::DesktopStreamsRegistry() {}
DesktopStreamsRegistry::~DesktopStreamsRegistry() {}

std::string DesktopStreamsRegistry::RegisterStream(
    int render_process_id,
    int render_frame_id,
    const GURL& origin,
    const content::DesktopMediaID& source,
    const std::string& extension_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string id = GenerateRandomStreamId();
  DCHECK(approved_streams_.find(id) == approved_streams_.end());
  ApprovedDesktopMediaStream& stream = approved_streams_[id];
  stream.render_process_id = render_process_id;
  stream.render_frame_id = render_frame_id;
  stream.origin = origin;
  stream.source = source;
  stream.extension_name = extension_name;

  content::BrowserThread::PostDelayedTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&DesktopStreamsRegistry::CleanupStream,
                     base::Unretained(this), id),
      base::TimeDelta::FromSeconds(kApprovedStreamTimeToLiveSeconds));

  return id;
}

content::DesktopMediaID DesktopStreamsRegistry::RequestMediaForStreamId(
    const std::string& id,
    int render_process_id,
    int render_frame_id,
    const GURL& origin,
    std::string* extension_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  StreamsMap::iterator it = approved_streams_.find(id);

  // Verify that if there is a request with the specified ID it was created for
  // the same origin and the same renderer.
  if (it == approved_streams_.end() ||
      render_process_id != it->second.render_process_id ||
      render_frame_id != it->second.render_frame_id ||
      origin != it->second.origin) {
    return content::DesktopMediaID();
  }

  content::DesktopMediaID result = it->second.source;
  *extension_name = it->second.extension_name;
  approved_streams_.erase(it);
  return result;
}

void DesktopStreamsRegistry::CleanupStream(const std::string& id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  approved_streams_.erase(id);
}

DesktopStreamsRegistry::ApprovedDesktopMediaStream::ApprovedDesktopMediaStream()
    : render_process_id(-1), render_frame_id(-1) {}
