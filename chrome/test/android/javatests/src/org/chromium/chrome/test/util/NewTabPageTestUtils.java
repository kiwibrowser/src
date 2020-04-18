// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import android.annotation.TargetApi;
import android.os.Build;

import org.chromium.chrome.browser.ntp.IncognitoNewTabPage;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * Utilities for testing the NTP.
 */
public class NewTabPageTestUtils {

    /**
     * Waits for the NTP owned by the passed in tab to be fully loaded.
     *
     * @param tab The tab to be monitored for NTP loading.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static void waitForNtpLoaded(final Tab tab) {
        CriteriaHelper.pollUiThread(new Criteria("NTP never fully loaded") {
            @Override
            public boolean isSatisfied() {
                if (!tab.isIncognito()) {
                    // TODO(tedchoc): Make MostVisitedPage also have a isLoaded() concept.
                    if (tab.getNativePage() instanceof NewTabPage) {
                        return ((NewTabPage) tab.getNativePage()).isLoadedForTests();
                    } else {
                        return false;
                    }
                } else {
                    if (!(tab.getNativePage() instanceof IncognitoNewTabPage)) {
                        return false;
                    }
                    return ((IncognitoNewTabPage) tab.getNativePage()).isLoadedForTests();
                }
            }
        });
    }

}
