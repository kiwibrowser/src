// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import org.chromium.base.BuildConfig;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.util.Arrays;

/**
 * This class provides the resource bundle related methods for the native
 * library.
 */
@JNINamespace("ui")
final class ResourceBundle {
    private ResourceBundle() {}

    @CalledByNative
    private static String getLocalePakResourcePath(String locale) {
        if (Arrays.binarySearch(BuildConfig.UNCOMPRESSED_LOCALES, locale) >= 0) {
            return "assets/stored-locales/" + locale + ".pak";
        }
        return null;
    }
}
