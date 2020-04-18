// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_media_log.h"

#include "base/logging.h"

namespace media {

MojoMediaLog::MojoMediaLog(
    scoped_refptr<mojom::ThreadSafeMediaLogAssociatedPtr> remote_media_log)
    : remote_media_log_(std::move(remote_media_log)) {
  DVLOG(1) << __func__;
}

MojoMediaLog::~MojoMediaLog() {
  DVLOG(1) << __func__;
}

void MojoMediaLog::AddEvent(std::unique_ptr<MediaLogEvent> event) {
  DVLOG(1) << __func__;
  DCHECK(event);
  (**remote_media_log_).AddEvent(*event);
}

}  // namespace media
