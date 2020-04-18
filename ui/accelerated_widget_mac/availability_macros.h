// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// AVSampleBufferDisplayLayer has incorrectly been marked as available from
// macOS 10.10, whereas it's been available since at least macOS 10.9. This
// macro allows us to use AVSampleBufferDisplayLayer109 in its place.
// Since we don't deploy to lower than 10.9, just disabling the warning is fine.
#define AVSampleBufferDisplayLayer109                                  \
  _Pragma("clang diagnostic push")                                     \
      _Pragma("clang diagnostic ignored \"-Wunguarded-availability\"") \
          AVSampleBufferDisplayLayer _Pragma("clang diagnostic pop")
