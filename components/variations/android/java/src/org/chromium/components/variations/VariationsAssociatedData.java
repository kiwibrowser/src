// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.variations;

import org.chromium.base.annotations.JNINamespace;

import java.util.HashMap;

/**
 * Wrapper for variations.
 */
@JNINamespace("variations::android")
public final class VariationsAssociatedData {

    private VariationsAssociatedData() {
    }

    /**
     * @param trialName The name of the trial to get the param value for.
     * @param paramName The name of the param for which to get the value.
     * @return The parameter value. Empty string if the field trial does not exist or the specified
     *     parameter does not exist.
     */
    public static String getVariationParamValue(String trialName, String paramName) {
        return nativeGetVariationParamValue(trialName, paramName);
    }

    public static HashMap<String, String> getFeedbackMap() {
        HashMap<String, String> map = new HashMap<String, String>();
        map.put("Chrome Variations", nativeGetFeedbackVariations());
        return map;
    }

    private static native String nativeGetVariationParamValue(String trialName, String paramName);
    private static native String nativeGetFeedbackVariations();
}
