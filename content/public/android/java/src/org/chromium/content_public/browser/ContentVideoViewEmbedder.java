// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.view.View;

/**
 *  Main callback class used by {@link ContentVideoView}.
 *
 *  This contains the superset of callbacks that must be implemented by the embedder
 *  to support fullscreen video.
 *
 *  {@link #enterFullscreenVideo(View)} and {@link #exitFullscreenVideo()} must be implemented,
 *  {@link #getVideoLoadingProgressView()} is optional, and may return null if not required.
 *
 *  The implementer is responsible for displaying the Android view when
 *  {@link #enterFullscreenVideo(View)} is called.
 */
public interface ContentVideoViewEmbedder {
    /**
     * Called when the {@link ContentVideoView}, which contains the fullscreen video,
     * is ready to be shown. Must be implemented.
     * @param view The view containing the fullscreen video that embedders must show.
     * @param isVideoLoaded The flag telling whether video has already been loaded.
     *                      When it is false, embedder might show the progress view.
     */
    public void enterFullscreenVideo(View view, boolean isVideoLoaded);

    /**
     * Tells the embedder to remove the progress view.
     */
    public void fullscreenVideoLoaded();

    /**
     * Called to exit fullscreen video. Embedders must stop showing the view given in
     * {@link #enterFullscreenVideo(View)}. Must be implemented.
     */
    public void exitFullscreenVideo();

    /**
     * Sets the system ui visibility after entering or exiting fullscreen.
     * @param enterFullscreen True if video is going fullscreen, or false otherwise.
     */
    public void setSystemUiVisibility(boolean enterFullscreen);
}
