/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef PC_TEST_FAKEPERIODICVIDEOTRACKSOURCE_H_
#define PC_TEST_FAKEPERIODICVIDEOTRACKSOURCE_H_

#include "pc/videotracksource.h"
#include "pc/test/fakeperiodicvideosource.h"

namespace webrtc {

// A VideoTrackSource generating frames with configured size and frame interval.
class FakePeriodicVideoTrackSource : public VideoTrackSource {
 public:
  explicit FakePeriodicVideoTrackSource(bool remote)
      : FakePeriodicVideoTrackSource(FakePeriodicVideoSource::Config(),
                                     remote) {}

  FakePeriodicVideoTrackSource(FakePeriodicVideoSource::Config config,
                               bool remote)
      // Note that VideoTrack constructor gets a pointer to an
      // uninitialized source object; that works because it only
      // stores the pointer for later use.
      : VideoTrackSource(&source_, remote), source_(config) {}

  ~FakePeriodicVideoTrackSource() = default;

 private:
  FakePeriodicVideoSource source_;
};

}  // namespace webrtc

#endif  // PC_TEST_FAKEPERIODICVIDEOTRACKSOURCE_H_
