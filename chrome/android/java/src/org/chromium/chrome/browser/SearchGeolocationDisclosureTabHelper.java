// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import org.chromium.base.VisibleForTesting;

/**
 * Helper that shows the search geolocation disclosure when required. This class currently is only
 * used to set static test variables in the native code.
 */
public class SearchGeolocationDisclosureTabHelper {
    @VisibleForTesting
    public static void setIgnoreUrlChecksForTesting() {
        nativeSetIgnoreUrlChecksForTesting();
    }

    @VisibleForTesting
    public static void setDayOffsetForTesting(int days) {
        nativeSetDayOffsetForTesting(days);
    }

    private static native void nativeSetIgnoreUrlChecksForTesting();
    private static native void nativeSetDayOffsetForTesting(int days);
}
