/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import javax.annotation.Nullable;

/**
 * Implements VideoCapturer.CapturerObserver and feeds frames to
 * webrtc::jni::AndroidVideoTrackSource.
 */
class NativeCapturerObserver implements VideoCapturer.CapturerObserver {
  // Pointer to webrtc::jni::AndroidVideoTrackSource.
  private final long nativeSource;
  // TODO(bugs.webrtc.org/9181): Remove.
  @Nullable private final SurfaceTextureHelper surfaceTextureHelper;

  @CalledByNative
  public NativeCapturerObserver(long nativeSource) {
    this.nativeSource = nativeSource;
    this.surfaceTextureHelper = null;
  }

  // TODO(bugs.webrtc.org/9181): Remove.
  public NativeCapturerObserver(long nativeSource, SurfaceTextureHelper surfaceTextureHelper) {
    this.nativeSource = nativeSource;
    this.surfaceTextureHelper = surfaceTextureHelper;
  }

  @Override
  public void onCapturerStarted(boolean success) {
    nativeCapturerStarted(nativeSource, success);
  }

  @Override
  public void onCapturerStopped() {
    nativeCapturerStopped(nativeSource);
  }

  // TODO(bugs.webrtc.org/9181): Remove.
  @Override
  @SuppressWarnings("deprecation")
  public void onByteBufferFrameCaptured(
      byte[] data, int width, int height, int rotation, long timestampNs) {
    // This NV21Buffer is not possible to retain. This is safe only because the native code will
    // always call cropAndScale() and directly make a deep copy of the buffer.
    final VideoFrame.Buffer nv21Buffer =
        new NV21Buffer(data, width, height, null /* releaseCallback */);
    final VideoFrame frame = new VideoFrame(nv21Buffer, rotation, timestampNs);
    onFrameCaptured(frame);
    frame.release();
  }

  // TODO(bugs.webrtc.org/9181): Remove.
  @Override
  @SuppressWarnings("deprecation")
  public void onTextureFrameCaptured(int width, int height, int oesTextureId,
      float[] transformMatrix, int rotation, long timestampNs) {
    final VideoFrame.Buffer buffer = surfaceTextureHelper.createTextureBuffer(
        width, height, RendererCommon.convertMatrixToAndroidGraphicsMatrix(transformMatrix));
    final VideoFrame frame = new VideoFrame(buffer, rotation, timestampNs);
    onFrameCaptured(frame);
    frame.release();
  }

  @Override
  public void onFrameCaptured(VideoFrame frame) {
    nativeOnFrameCaptured(nativeSource, frame.getBuffer().getWidth(), frame.getBuffer().getHeight(),
        frame.getRotation(), frame.getTimestampNs(), frame.getBuffer());
  }

  public void dispose() {
    if (surfaceTextureHelper != null) {
      surfaceTextureHelper.dispose();
    }
  }

  private static native void nativeCapturerStarted(long source, boolean success);
  private static native void nativeCapturerStopped(long source);
  private static native void nativeOnFrameCaptured(
      long source, int width, int height, int rotation, long timestampNs, VideoFrame.Buffer frame);
}
