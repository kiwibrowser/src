// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * Checks and waits for certain overview mode events to happen.  Can be used to block test threads
 * until certain overview mode state criteria are met.
 */
public class OverviewModeBehaviorWatcher implements OverviewModeObserver {
    private final OverviewModeBehavior mOverviewModeBehavior;
    private boolean mWaitingForShow;
    private boolean mWaitingForHide;

    private final Criteria mCriteria = new Criteria() {
        @Override
        public boolean isSatisfied() {
            if (mWaitingForShow) {
                updateFailureReason(
                        "OverviewModeObserver#onOverviewModeFinishedShowing() not called.");
                return false;
            }
            if (mWaitingForHide) {
                updateFailureReason(
                        "OverviewModeObserver#onOverviewModeFinishedHiding() not called.");
                return false;
            }
            return true;
        }
    };

    /**
     * Creates an instance of an {@link OverviewModeBehaviorWatcher}.  Note that at this point
     * it will be registered for overview mode events.
     *
     * @param behavior          The {@link OverviewModeBehavior} to watch.
     * @param waitForShow Whether or not to wait for overview mode to finish showing.
     * @param waitForHide Whether or not to wait for overview mode to finish hiding.
     */
    public OverviewModeBehaviorWatcher(OverviewModeBehavior behavior, boolean waitForShow,
            boolean waitForHide) {
        mOverviewModeBehavior = behavior;
        mOverviewModeBehavior.addOverviewModeObserver(this);

        mWaitingForShow = waitForShow;
        mWaitingForHide = waitForHide;
    }

    @Override
    public void onOverviewModeStartedShowing(boolean showToolbar) { }

    @Override
    public void onOverviewModeFinishedShowing() {
        mWaitingForShow = false;
    }

    @Override
    public void onOverviewModeStartedHiding(boolean showToolbar, boolean delayAnimation) { }

    @Override
    public void onOverviewModeFinishedHiding() {
        mWaitingForHide = false;
    }

    /**
     * Blocks until all of the expected events have occurred.  Once the events of this class
     * are met it will always return immediately from {@link #waitForBehavior()}.
     */
    public void waitForBehavior() {
        try {
            CriteriaHelper.pollUiThread(mCriteria);
        } finally {
            mOverviewModeBehavior.removeOverviewModeObserver(this);
        }
    }
}