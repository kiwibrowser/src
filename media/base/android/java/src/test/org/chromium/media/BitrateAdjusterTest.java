// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import static org.junit.Assert.assertEquals;

import android.support.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

/**
 * Tests for BitrateAdjuster, a class used to adjust the target bitrate and framerate for certain
 * codecs in video encoders with fixed framerates.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class BitrateAdjusterTest {
    private static final int BITRATE_4_KBPS = 4000000;
    private static final int BITRATE_8_KBPS = 8000000;
    private static final int BITRATE_16_KBPS = 16000000;

    @Test
    @SmallTest
    public void testNoAdjustmentDoesNotChangeTargetBitrate() {
        assertEquals(
                BitrateAdjuster.NO_ADJUSTMENT.getTargetBitrate(BITRATE_8_KBPS, 30), BITRATE_8_KBPS);
        assertEquals(
                BitrateAdjuster.NO_ADJUSTMENT.getTargetBitrate(BITRATE_8_KBPS, 15), BITRATE_8_KBPS);
    }

    @Test
    @SmallTest
    public void testNoAdjustmentInitialFrameRateIsClamped() {
        assertEquals(BitrateAdjuster.NO_ADJUSTMENT.getInitialFrameRate(15), 15);
        assertEquals(BitrateAdjuster.NO_ADJUSTMENT.getInitialFrameRate(30), 30);
        assertEquals(BitrateAdjuster.NO_ADJUSTMENT.getInitialFrameRate(60), 30);
    }

    @Test
    @SmallTest
    public void testFrameRateAdjustmentAdjustsAccordingToFrameRate() {
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getTargetBitrate(BITRATE_8_KBPS, 30),
                BITRATE_8_KBPS);
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getTargetBitrate(BITRATE_8_KBPS, 15),
                BITRATE_16_KBPS);
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getTargetBitrate(BITRATE_8_KBPS, 60),
                BITRATE_4_KBPS);
    }

    @Test
    @SmallTest
    public void testFrameRateAdjustmentDoesNotDivideByZero() {
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getTargetBitrate(BITRATE_8_KBPS, 0),
                BITRATE_8_KBPS);
    }

    @Test
    @SmallTest
    public void testFrameRateAdjustmentUsesFixedInitialFrameRate() {
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getInitialFrameRate(15), 30);
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getInitialFrameRate(30), 30);
        assertEquals(BitrateAdjuster.FRAMERATE_ADJUSTMENT.getInitialFrameRate(60), 30);
    }
}
