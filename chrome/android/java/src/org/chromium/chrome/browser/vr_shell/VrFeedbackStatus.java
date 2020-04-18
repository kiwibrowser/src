// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ChromeFeatureList;

/**
 * Gets and sets preferences related to the status of the Vr feedback infobar.
 */
public class VrFeedbackStatus {
    private static final String FEEDBACK_FREQUENCY_PARAM_NAME = "feedback_frequency";
    private static final int DEFAULT_FEEDBACK_FREQUENCY = 10;

    private static final String VR_FEEDBACK_OPT_OUT = "VR_FEEDBACK_OPT_OUT";
    private static final String VR_EXIT_TO_2D_COUNT = "VR_EXIT_TO_2D_COUNT";

    /**
     * Returns how often we should show the feedback prompt.
     */
    public static int getFeedbackFrequency() {
        return ChromeFeatureList.getFieldTrialParamByFeatureAsInt(
                ChromeFeatureList.VR_BROWSING_FEEDBACK, FEEDBACK_FREQUENCY_PARAM_NAME,
                DEFAULT_FEEDBACK_FREQUENCY);
    }

    /**
     * Sets the "opted out of entering VR feedback" preference.
     * @param optOut Whether the VR feedback option has been opted-out of.
     */
    public static void setFeedbackOptOut(boolean optOut) {
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(VR_FEEDBACK_OPT_OUT, optOut)
                .apply();
    }

    /**
     * Returns whether the user opted out of entering feedback for their VR experience.
     */
    public static boolean getFeedbackOptOut() {
        return ContextUtils.getAppSharedPreferences().getBoolean(VR_FEEDBACK_OPT_OUT, false);
    }

    /**
     * Sets the "exited VR mode into 2D browsing" preference.
     * @param count The number of times the user exited VR and entered 2D browsing mode
     */
    public static void setUserExitedAndEntered2DCount(int count) {
        ContextUtils.getAppSharedPreferences().edit().putInt(VR_EXIT_TO_2D_COUNT, count).apply();
    }

    /**
     * Returns the number of times the user has exited VR mode and entered the 2D browsing
     * mode.
     */
    public static int getUserExitedAndEntered2DCount() {
        return ContextUtils.getAppSharedPreferences().getInt(VR_EXIT_TO_2D_COUNT, 0);
    }
}
