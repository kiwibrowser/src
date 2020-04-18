// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.os.StrictMode;
import android.preference.PreferenceFragment;
import android.support.annotation.XmlRes;

/**
 * A helper class for Preferences.
 */
public class PreferenceUtils {
    /**
     * A helper that is used to load preferences from XML resources without causing a
     * StrictModeViolation. See http://crbug.com/692125.
     *
     * @param preferenceFragment A PreferenceFragment.
     * @param preferencesResId   The id of the XML resource to add to the PreferenceFragment.
     */
    public static void addPreferencesFromResource(
            PreferenceFragment preferenceFragment, @XmlRes int preferencesResId) {
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            preferenceFragment.addPreferencesFromResource(preferencesResId);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }
}
