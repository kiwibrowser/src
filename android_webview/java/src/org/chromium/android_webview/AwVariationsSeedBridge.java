// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

/**
 * AwVariationsSeedBridge stores the seed loaded in Java for use in native. Native should clear the
 * stored seed after retrieving it, since the seed isn't needed in Java.
 */
public class AwVariationsSeedBridge {
    private static SeedInfo sSeed;

    public static void setSeed(SeedInfo seed) {
        assert sSeed == null; // The seed should only be loaded once.
        sSeed = seed;
    }

    @CalledByNative
    private static boolean haveSeed() {
        return sSeed != null;
    }

    @CalledByNative
    private static void clearSeed() {
        sSeed = null; // Allow GC.
    }

    @CalledByNative
    private static String getSignature() {
        return sSeed.signature;
    }

    @CalledByNative
    private static String getCountry() {
        return sSeed.country;
    }

    @CalledByNative
    private static String getDate() {
        return sSeed.date;
    }

    @CalledByNative
    private static boolean getIsGzipCompressed() {
        return sSeed.isGzipCompressed;
    }

    @CalledByNative
    private static byte[] getData() {
        return sSeed.seedData;
    }
}
