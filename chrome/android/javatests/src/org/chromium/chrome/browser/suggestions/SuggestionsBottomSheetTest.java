// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.MotionEvent;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ntp.NtpUiCaptureTestData;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.suggestions.SuggestionsDependenciesRule;
import org.chromium.content.browser.test.util.TestTouchUtils;
import org.chromium.ui.test.util.UiRestriction;

import java.util.concurrent.ExecutionException;

/**
 * Instrumentation tests for {@link SuggestionsBottomSheetContent}.
 */
@DisabledTest(message = "https://crbug.com/805160")
@RunWith(ChromeJUnit4ClassRunner.class)
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // ChromeHome is only enabled on phones
public class SuggestionsBottomSheetTest {
    @Rule
    public SuggestionsBottomSheetTestRule mActivityRule = new SuggestionsBottomSheetTestRule();

    @Rule
    public SuggestionsDependenciesRule createSuggestions() {
        return new SuggestionsDependenciesRule(NtpUiCaptureTestData.createFactory());
    }

    @Before
    public void setUp() throws InterruptedException {
        mActivityRule.startMainActivityOnBlankPage();
    }

    @Test
    @RetryOnFailure
    @MediumTest
    public void testContextMenu() throws InterruptedException, ExecutionException {
        ViewHolder suggestionViewHolder =
                mActivityRule.scrollToFirstItemOfType(ItemViewType.SNIPPET);
        assertFalse(mActivityRule.getBottomSheet().onInterceptTouchEvent(createTapEvent()));

        TestTouchUtils.performLongClickOnMainSync(
                InstrumentationRegistry.getInstrumentation(), suggestionViewHolder.itemView);
        assertTrue(mActivityRule.getBottomSheet().onInterceptTouchEvent(createTapEvent()));

        ThreadUtils.runOnUiThreadBlocking(mActivityRule.getActivity()::closeContextMenu);
        assertFalse(mActivityRule.getBottomSheet().onInterceptTouchEvent(createTapEvent()));
    }

    private static MotionEvent createTapEvent() {
        return MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
                MotionEvent.ACTION_DOWN, 0f, 0f, 0);
    }
}
