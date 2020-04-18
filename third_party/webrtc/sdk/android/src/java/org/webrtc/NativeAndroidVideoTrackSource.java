/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import android.support.annotation.Nullable;
import org.webrtc.VideoFrame;

/**
 * This class is meant to be a simple layer that only handles the JNI wrapping of a C++
 * AndroidVideoTrackSource, that can easily be mocked out in Java unit tests. Refrain from adding
 * any unnecessary logic to this class.
 */
class NativeAndroidVideoTrackSource {
  // Pointer to webrtc::jni::AndroidVideoTrackSource.
  private final long nativeAndroidVideoTrackSource;

  public NativeAndroidVideoTrackSource(long nativeAndroidVideoTrackSource) {
    this.nativeAndroidVideoTrackSource = nativeAndroidVideoTrackSource;
  }

  /**
   * Set the state for the native MediaSourceInterface. Maps boolean to either
   * SourceState::kLive or SourceState::kEnded.
   */
  public void setState(boolean isLive) {
    nativeSetState(nativeAndroidVideoTrackSource, isLive);
  }

  /** Pass a frame to the native AndroidVideoTrackSource. */
  public void onFrameCaptured(VideoFrame frame) {
    nativeOnFrameCaptured(nativeAndroidVideoTrackSource, frame.getBuffer().getWidth(),
        frame.getBuffer().getHeight(), frame.getRotation(), frame.getTimestampNs(),
        frame.getBuffer());
  }

  /**
   * Calling this function will cause frames to be scaled down to the requested resolution. Also,
   * frames will be cropped to match the requested aspect ratio, and frames will be dropped to match
   * the requested fps.
   */
  public void adaptOutputFormat(VideoSource.AspectRatio targetLandscapeAspectRatio,
      @Nullable Integer maxLandscapePixelCount, VideoSource.AspectRatio targetPortraitAspectRatio,
      @Nullable Integer maxPortraitPixelCount, @Nullable Integer maxFps) {
    nativeAdaptOutputFormat(nativeAndroidVideoTrackSource, targetLandscapeAspectRatio.width,
        targetLandscapeAspectRatio.height, maxLandscapePixelCount, targetPortraitAspectRatio.width,
        targetPortraitAspectRatio.height, maxPortraitPixelCount, maxFps);
  }

  private static native void nativeSetState(long nativeAndroidVideoTrackSource, boolean isLive);
  private static native void nativeAdaptOutputFormat(long nativeAndroidVideoTrackSource,
      int landscapeWidth, int landscapeHeight, @Nullable Integer maxLandscapePixelCount,
      int portraitWidth, int portraitHeight, @Nullable Integer maxPortraitPixelCount,
      @Nullable Integer maxFps);
  private static native void nativeOnFrameCaptured(long nativeAndroidVideoTrackSource, int width,
      int height, int rotation, long timestampNs, VideoFrame.Buffer buffer);
}
