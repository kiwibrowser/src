// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import static org.junit.Assert.assertEquals;

import android.support.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.test.BottomSheetTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.ui.test.util.UiRestriction;

import java.util.concurrent.TimeoutException;

/** This class tests the functionality of the {@link BottomSheetObserver}. */
@RunWith(ChromeJUnit4ClassRunner.class)
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // ChromeHome is only enabled on phones
public class BottomSheetObserverTest {
    @Rule
    public BottomSheetTestRule mBottomSheetTestRule = new BottomSheetTestRule();
    private BottomSheetTestRule.Observer mObserver;

    @Before
    public void setUp() throws Exception {
        mBottomSheetTestRule.startMainActivityOnBlankPage();
        mObserver = mBottomSheetTestRule.getObserver();
    }

    /**
     * Test that the onSheetClosed event is triggered if the sheet is closed without animation.
     */
    @Test
    @MediumTest
    public void testCloseEventCalledNoAnimation() throws InterruptedException, TimeoutException {
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_FULL, false);

        CallbackHelper closedCallbackHelper = mObserver.mClosedCallbackHelper;

        int initialOpenedCount = mObserver.mOpenedCallbackHelper.getCallCount();

        int closedCallbackCount = closedCallbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_PEEK, false);
        closedCallbackHelper.waitForCallback(closedCallbackCount, 1);

        assertEquals(initialOpenedCount, mObserver.mOpenedCallbackHelper.getCallCount());
    }

    /**
     * Test that the onSheetClosed event is triggered if the sheet is closed with animation.
     */
    @Test
    @MediumTest
    public void testCloseEventCalledWithAnimation() throws InterruptedException, TimeoutException {
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_FULL, false);

        CallbackHelper closedCallbackHelper = mObserver.mClosedCallbackHelper;

        int initialOpenedCount = mObserver.mOpenedCallbackHelper.getCallCount();

        int closedCallbackCount = closedCallbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_PEEK, true);
        closedCallbackHelper.waitForCallback(closedCallbackCount, 1);

        assertEquals(initialOpenedCount, mObserver.mOpenedCallbackHelper.getCallCount());
    }

    /**
     * Test that the onSheetOpened event is triggered if the sheet is opened without animation.
     */
    @Test
    @MediumTest
    public void testOpenedEventCalledNoAnimation() throws InterruptedException, TimeoutException {
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_PEEK, false);

        CallbackHelper openedCallbackHelper = mObserver.mOpenedCallbackHelper;

        int initialClosedCount = mObserver.mClosedCallbackHelper.getCallCount();

        int openedCallbackCount = openedCallbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_FULL, false);
        openedCallbackHelper.waitForCallback(openedCallbackCount, 1);

        assertEquals(initialClosedCount, mObserver.mClosedCallbackHelper.getCallCount());
    }

    /**
     * Test that the onSheetOpened event is triggered if the sheet is opened with animation.
     */
    @Test
    @MediumTest
    public void testOpenedEventCalledWithAnimation() throws InterruptedException, TimeoutException {
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_PEEK, false);

        CallbackHelper openedCallbackHelper = mObserver.mOpenedCallbackHelper;

        int initialClosedCount = mObserver.mClosedCallbackHelper.getCallCount();

        int openedCallbackCount = openedCallbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetState(BottomSheet.SHEET_STATE_FULL, true);
        openedCallbackHelper.waitForCallback(openedCallbackCount, 1);

        assertEquals(initialClosedCount, mObserver.mClosedCallbackHelper.getCallCount());
    }

    /**
     * Test the onOffsetChanged event.
     */
    @Test
    @MediumTest
    public void testOffsetChangedEvent() throws InterruptedException, TimeoutException {
        CallbackHelper callbackHelper = mObserver.mOffsetChangedCallbackHelper;

        BottomSheet bottomSheet = mBottomSheetTestRule.getBottomSheet();
        float hiddenHeight = bottomSheet.getHiddenRatio() * bottomSheet.getSheetContainerHeight();
        float fullHeight = bottomSheet.getFullRatio() * bottomSheet.getSheetContainerHeight();

        // The sheet's half state is not necessarily 50% of the way to the top.
        float midPeekFull = (hiddenHeight + fullHeight) / 2f;

        // When in the hidden state, the transition value should be 0.
        int callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(hiddenHeight);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(0f, mObserver.getLastOffsetChangedValue(), MathUtils.EPSILON);

        // When in the full state, the transition value should be 1.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(fullHeight);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(1f, mObserver.getLastOffsetChangedValue(), MathUtils.EPSILON);

        // Halfway between peek and full should send 0.5.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(midPeekFull);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(0.5f, mObserver.getLastOffsetChangedValue(), MathUtils.EPSILON);
    }

    /**
     * Test the onTransitionPeekToHalf event.
     */
    @Test
    @MediumTest
    public void testPeekToHalfTransition() throws InterruptedException, TimeoutException {
        CallbackHelper callbackHelper = mObserver.mPeekToHalfCallbackHelper;

        BottomSheet bottomSheet = mBottomSheetTestRule.getBottomSheet();
        float peekHeight = bottomSheet.getPeekRatio() * bottomSheet.getSheetContainerHeight();
        float halfHeight = bottomSheet.getHalfRatio() * bottomSheet.getSheetContainerHeight();
        float fullHeight = bottomSheet.getFullRatio() * bottomSheet.getSheetContainerHeight();

        float midPeekHalf = (peekHeight + halfHeight) / 2f;
        float midHalfFull = (halfHeight + fullHeight) / 2f;

        // When in the peeking state, the transition value should be 0.
        int callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(peekHeight);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(0f, mObserver.getLastPeekToHalfValue(), MathUtils.EPSILON);

        // When in between peek and half states, the transition value should be 0.5.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(midPeekHalf);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(0.5f, mObserver.getLastPeekToHalfValue(), MathUtils.EPSILON);

        // After jumping to the full state (skipping the half state), the event should have
        // triggered once more with a max value of 1.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(fullHeight);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(1f, mObserver.getLastPeekToHalfValue(), MathUtils.EPSILON);

        // Moving from full to somewhere between half and full should not trigger the event.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(midHalfFull);
        assertEquals(callbackCount, callbackHelper.getCallCount());

        // Reset the sheet to be between peek and half states.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(midPeekHalf);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(0.5f, mObserver.getLastPeekToHalfValue(), MathUtils.EPSILON);

        // At the half state the event should send 1.
        callbackCount = callbackHelper.getCallCount();
        mBottomSheetTestRule.setSheetOffsetFromBottom(halfHeight);
        callbackHelper.waitForCallback(callbackCount, 1);
        assertEquals(1f, mObserver.getLastPeekToHalfValue(), MathUtils.EPSILON);
    }
}
