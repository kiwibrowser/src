/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import org.webrtc.VideoCapturer;

public class JavaVideoSourceTestHelper {
  @CalledByNative
  public static void startCapture(VideoCapturer.CapturerObserver observer, boolean success) {
    observer.onCapturerStarted(success);
  }

  @CalledByNative
  public static void stopCapture(VideoCapturer.CapturerObserver observer) {
    observer.onCapturerStopped();
  }

  @CalledByNative
  public static void deliverFrame(VideoCapturer.CapturerObserver observer) {
    final int FRAME_WIDTH = 2;
    final int FRAME_HEIGHT = 3;
    final int FRAME_ROTATION = 180;

    observer.onFrameCaptured(new VideoFrame(
        JavaI420Buffer.allocate(FRAME_WIDTH, FRAME_HEIGHT), FRAME_ROTATION, 0 /* timestampNs */));
  }
}
