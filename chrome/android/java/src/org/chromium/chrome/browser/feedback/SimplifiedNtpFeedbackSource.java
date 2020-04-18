// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feedback;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.NewTabPageView;

import java.util.HashMap;
import java.util.Map;

/**
 * Provides information about whether the simplified NTP is enabled for use in feedback reports.
 */
public class SimplifiedNtpFeedbackSource implements FeedbackSource {
    private static final String SIMPLIFIED_NTP_KEY = "Simplified NTP";
    private static final String ENABLED_VALUE = "Enabled";
    private static final String ENABLED_ABLATION_VALUE = "Enabled-ablation";
    private static final String DISABLED_VALUE = "Disabled";

    private final HashMap<String, String> mMap;

    SimplifiedNtpFeedbackSource() {
        mMap = new HashMap<String, String>(1);

        boolean isEnabled = ChromeFeatureList.isEnabled(ChromeFeatureList.SIMPLIFIED_NTP);
        if (!isEnabled) {
            mMap.put(SIMPLIFIED_NTP_KEY, DISABLED_VALUE);
        } else {
            boolean isAblationEnabled = NewTabPageView.isSimplifiedNtpAblationEnabled();
            mMap.put(
                    SIMPLIFIED_NTP_KEY, isAblationEnabled ? ENABLED_ABLATION_VALUE : ENABLED_VALUE);
        }
    }

    @Override
    public Map<String, String> getFeedback() {
        return mMap;
    }
}
