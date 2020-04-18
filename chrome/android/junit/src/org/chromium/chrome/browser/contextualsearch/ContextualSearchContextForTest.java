// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

/**
 * Provides a {@link ContextualSearchContext} suitable for injection into a class for testing.  This
 * has native calls stubbed out and is set up to provide some simple test feedback.
 */
public class ContextualSearchContextForTest extends ContextualSearchContext {
    private boolean mDidSelectionChange;
    private String mDetectedLanguage;

    /**
     * @return Whether {@link #onSelectionChanged} was called.
     */
    boolean getDidSelectionChange() {
        return mDidSelectionChange;
    }

    void setLanguageToDetect(String language) {
        mDetectedLanguage = language;
    }

    @Override
    void onSelectionChanged() {
        mDidSelectionChange = true;
    }

    @Override
    protected long nativeInit() {
        return -1;
    }

    @Override
    protected void nativeDestroy(long nativeContextualSearchContext) {}

    @Override
    protected void nativeSetResolveProperties(
            long nativeContextualSearchContext, String homeCountry, boolean maySendBasePageUrl) {}

    @Override
    protected void nativeAdjustSelection(
            long nativeContextualSearchContext, int startAdjust, int endAdjust) {}

    @Override
    protected String nativeDetectLanguage(long nativeContextualSearchContext) {
        return mDetectedLanguage;
    }
}
