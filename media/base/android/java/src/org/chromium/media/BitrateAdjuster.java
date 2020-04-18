// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

enum BitrateAdjuster {
    // No adjustment - video encoder has no known bitrate problem.
    NO_ADJUSTMENT {
        private static final int MAXIMUM_INITIAL_FPS = 30;

        @Override
        public int getTargetBitrate(int bps, int frameRate) {
            return bps;
        }

        @Override
        public int getInitialFrameRate(int frameRateHint) {
            return Math.min(frameRateHint, MAXIMUM_INITIAL_FPS);
        }
    },

    // Framerate based bitrate adjustment is required - HW encoder does not use frame
    // timestamps to calculate frame bitrate budget and instead is relying on initial
    // fps configuration assuming that all frames are coming at fixed initial frame rate.
    FRAMERATE_ADJUSTMENT {
        private static final int BITRATE_ADJUSTMENT_FPS = 30;

        @Override
        public int getTargetBitrate(int bps, int frameRate) {
            if (frameRate == 0) {
                return bps;
            }
            return BITRATE_ADJUSTMENT_FPS * bps / frameRate;
        }

        @Override
        public int getInitialFrameRate(int frameRateHint) {
            return BITRATE_ADJUSTMENT_FPS;
        }
    };

    // Gets the adjusted bitrate according to the implementation's adjustment policy.
    public abstract int getTargetBitrate(int bps, int frameRate);

    // Gets the initial frame rate of the media. The frameRateHint can be used as a default or a
    // constraint.
    public abstract int getInitialFrameRate(int frameRateHint);
}
