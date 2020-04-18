// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

/** Encapsulates server-provided conditions for "peeking" the contextual suggestions UI. */
public class PeekConditions {
    private final float mPageScrollPercentage;
    private final float mMinimumSecondsOnPage;
    private final float mMaximumNumberOfPeeks;

    public PeekConditions() {
        this(0, 0, 0);
    }

    /**
     * Constructs a new PeekConditions.
     * @param pageScrollPercentage float The percentage of the page that the user scrolls required
     * for an auto peek to occur.
     * @param minimumSecondsOnPage float The minimum time (seconds) the user spends on the page
     * required for auto peek.
     * @param maximumNumberOfPeeks float The maximum number of auto peeks that we can show for this
     * page.
     */
    public PeekConditions(
            float pageScrollPercentage, float minimumSecondsOnPage, float maximumNumberOfPeeks) {
        mPageScrollPercentage = pageScrollPercentage;
        mMinimumSecondsOnPage = minimumSecondsOnPage;
        mMaximumNumberOfPeeks = maximumNumberOfPeeks;
    }

    /**
     * @return The percentage of the page that the user scrolls required for an auto peek to occur.
     */
    public float getPageScrollPercentage() {
        return mPageScrollPercentage;
    }

    /** @return The minimum time (seconds) the user spends on the page required for auto peek. */
    public float getMinimumSecondsOnPage() {
        return mMinimumSecondsOnPage;
    }

    /** @return The maximum number of auto peeks that we can show for this page. */
    public float getMaximumNumberOfPeeks() {
        return mMaximumNumberOfPeeks;
    }
}