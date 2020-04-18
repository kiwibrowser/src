// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.snippets;

import org.chromium.chrome.browser.ChromeFeatureList;

/**
 * Provides configuration details for NTP snippets.
 */
public final class SnippetsConfig {
    private SnippetsConfig() {}

    /** https://crbug.com/660837 */
    public static boolean isIncreasedCardVisibilityEnabled() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.NTP_SNIPPETS_INCREASED_VISIBILITY);
    }

    public static boolean isFaviconsFromNewServerEnabled() {
        return ChromeFeatureList.isEnabled(
                ChromeFeatureList.CONTENT_SUGGESTIONS_FAVICONS_FROM_NEW_SERVER);
    }
}
