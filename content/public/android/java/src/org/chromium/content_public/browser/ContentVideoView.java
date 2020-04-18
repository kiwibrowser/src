// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.content.Context;

import org.chromium.content.browser.ContentVideoViewImpl;

/**
 * Interface for a fullscreen video playback using surface view.
 */
public interface ContentVideoView {
    /**
     * @return {@link ContentVideoView} instance available for embedder.
     */
    public static ContentVideoView getInstance() {
        return ContentVideoViewImpl.getInstance();
    }

    /**
     * Checks if the content video view was created with the given context.
     * @param context {@link Context} to compare with.
     * @return {@code true} if the given context was used to create the video view.
     */
    boolean createdWithContext(Context context);

    /**
     * Exit from fullscreen mode.
     * @param releaseMediaPlayer Set to {@code true} to release media player instance.
     */
    void exitFullscreen(boolean releaseMediaPlayer);

    /**
     * Called when the fullscreen window gets focused.
     */
    void onFullscreenWindowFocused();
}
