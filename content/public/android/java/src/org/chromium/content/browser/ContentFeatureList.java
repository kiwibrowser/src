// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;

/**
 * Java accessor for base/feature_list.h state.
 */
@JNINamespace("content::android")
@MainDex
public abstract class ContentFeatureList {
    // Prevent instantiation.
    private ContentFeatureList() {}

    /**
     * Returns whether the specified feature is enabled or not.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in content/browser/android/content_feature_list.cc
     *
     * @param featureName The name of the feature to query.
     * @return Whether the feature is enabled or not.
     */
    public static boolean isEnabled(String featureName) {
        return nativeIsEnabled(featureName);
    }

    // Alphabetical:
    public static final String ENHANCED_SELECTION_INSERTION_HANDLE =
            "EnhancedSelectionInsertionHandle";

    private static native boolean nativeIsEnabled(String featureName);
}
