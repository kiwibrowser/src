// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

/**
 * Contains command line switches that are specific to the media layer.
 */
public abstract class MediaSwitches {
    // Set the autoplay policy to ignore user gesture requirements
    public static final String AUTOPLAY_NO_GESTURE_REQUIRED_POLICY =
            "autoplay-policy=no-user-gesture-required";

    // TODO(819383): Remove this and its usage.
    public static final String USE_MODERN_MEDIA_CONTROLS = "UseModernMediaControls";

    // Prevents instantiation.
    private MediaSwitches() {}
}
