// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.accessibility.captioning;

import android.content.Context;
import android.os.Build;

/**
 * Returns the best captions bridge available.  If the API level is lower than KitKat, a no-op
 * bridge is returned since those systems didn't support this functionality.
 */
public class CaptioningBridgeFactory {
    /**
     * Create and return the best SystemCaptioningBridge available.
     *
     * @param context Context associated with the activity.
     * @return the best SystemCaptioningBridge available.
     */
    public static SystemCaptioningBridge getSystemCaptioningBridge(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            return KitKatCaptioningBridge.getInstance(context);
        }
        // On older systems, return a CaptioningBridge that does nothing.
        return new EmptyCaptioningBridge();
    }
}
