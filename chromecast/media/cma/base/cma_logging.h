// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BASE_CMA_LOGGING_H_
#define CHROMECAST_MEDIA_CMA_BASE_CMA_LOGGING_H_

#include "base/logging.h"

namespace chromecast {
namespace media {

#define CMALOG(loglevel) VLOG(loglevel)

enum {
  kLogControl = 2,
  kLogFrame = 3
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BASE_CMA_LOGGING_H_
