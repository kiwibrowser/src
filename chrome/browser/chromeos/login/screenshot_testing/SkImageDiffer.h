// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WARNING! This file is copied from third_party/skia/tools/skpdiff and slightly
// modified to be compilable outside Skia and suit chromium style. Some comments
// can make no sense.
// TODO(elizavetai): remove this file and reuse the original one in Skia

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SKIMAGEDIFFER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SKIMAGEDIFFER_H_

#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"

/**
 * Encapsulates an image difference metric algorithm that can be potentially run
 * asynchronously.
 */
class SkImageDiffer {
 public:
  SkImageDiffer();
  virtual ~SkImageDiffer();

  static const double RESULT_CORRECT;
  static const double RESULT_INCORRECT;

  struct Result {
    double result;
    int poiCount;
    // TODO(djsollen): Figure out a way that the differ can report which of the
    // optional fields it has filled in.  See http://skbug.com/2712 ('allow
    // skpdiff to report different sets of result fields for different
    // comparison algorithms')
    SkBitmap poiAlphaMask;     // optional
    SkBitmap rgbDiffBitmap;    // optional
    SkBitmap whiteDiffBitmap;  // optional
    int maxRedDiff;            // optional
    int maxGreenDiff;          // optional
    int maxBlueDiff;           // optional
    double timeElapsed;        // optional

    Result();
  };

  // A bitfield indicating which bitmap types we want a differ to create.
  //
  // TODO(epoger): Remove whiteDiffBitmap, because alphaMask can provide
  // the same functionality and more.
  // It will be a little bit tricky, because the rebaseline_server client
  // and server side code will both need to change to use the alphaMask.
  struct BitmapsToCreate {
    bool alphaMask;
    bool rgbDiff;
    bool whiteDiff;
  };
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_SKIMAGEDIFFER_H_
