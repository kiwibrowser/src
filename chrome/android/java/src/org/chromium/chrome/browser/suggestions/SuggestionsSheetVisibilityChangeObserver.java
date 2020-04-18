// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.app.Activity;
import android.support.annotation.CallSuper;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.BottomSheetContent;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.StateChangeReason;
import org.chromium.chrome.browser.widget.bottomsheet.EmptyBottomSheetObserver;

/**
 * Notifies of events dedicated to changes in visibility of a
 * {@link SuggestionsBottomSheetContent}.
 */
public abstract class SuggestionsSheetVisibilityChangeObserver
        extends EmptyBottomSheetObserver implements ApplicationStatus.ActivityStateListener {
    @BottomSheet.SheetState
    private int mCurrentContentState;
    private boolean mCurrentVisibility;
    private boolean mWasShownSinceLastOpen;

    private final ChromeActivity mActivity;
    private final BottomSheet.BottomSheetContent mContentObserved;
    private final BottomSheet mBottomSheet;

    /**
     * Creates and register the observer to receive events related to changes to the provided
     * {@link BottomSheetContent}'s visibility.
     * @param bottomSheetContent BottomSheetContent to observe visibility for.
     * @param chromeActivity The BottomSheet the observed content is registered with. Note: the
     *                    constructor does not register the object as observer!
     */
    public SuggestionsSheetVisibilityChangeObserver(
            BottomSheetContent bottomSheetContent, ChromeActivity chromeActivity) {
        mActivity = chromeActivity;
        mContentObserved = bottomSheetContent;
        mBottomSheet = chromeActivity.getBottomSheet();
        assert mBottomSheet != null;

        ApplicationStatus.registerStateListenerForActivity(this, chromeActivity);
        mBottomSheet.addObserver(this);

        // This event is swallowed when the observer is registered after the sheet is opened.
        // (e.g. Chrome starts on the NTP). This allows taking it into account.
        if (mBottomSheet.isSheetOpen()) onSheetOpened(StateChangeReason.NONE);
    }

    public void onDestroy() {
        ApplicationStatus.unregisterActivityStateListener(this);
        mBottomSheet.removeObserver(this);
    }

    /** Called when the observed sheet content becomes visible.
     * @param isFirstShown Only {@code true} the first time suggestions are shown each time the
     *                     sheet is opened.
     */
    public abstract void onContentShown(boolean isFirstShown);

    /** Called when the observed sheet content becomes invisible. */
    public abstract void onContentHidden();

    /**
     * Called when the visible state of the observed sheet content changes.
     * @param contentState The new state, restricted to stable ones:
     *                     {@link BottomSheet#SHEET_STATE_FULL},{@link BottomSheet#SHEET_STATE_HALF}
     *                     or {@link BottomSheet#SHEET_STATE_PEEK}
     */
    public abstract void onContentStateChanged(@BottomSheet.SheetState int contentState);

    @Override
    @CallSuper
    public void onSheetOpened(@StateChangeReason int reason) {
        mWasShownSinceLastOpen = false;
        onStateChange();
    }

    @Override
    @CallSuper
    public void onSheetClosed(@StateChangeReason int reason) {
        onStateChange();
    }

    @Override
    @CallSuper
    public void onSheetContentChanged(BottomSheetContent newContent) {
        onStateChange();
    }

    @Override
    @CallSuper
    public void onSheetStateChanged(int newState) {
        onStateChange();
    }

    @Override
    @CallSuper
    public void onActivityStateChange(Activity activity, @ActivityState int newState) {
        if (newState == ActivityState.DESTROYED) {
            onDestroy();
            return;
        }

        if (!mBottomSheet.isSheetOpen()) return;

        onStateChange();
    }

    /**
     * @return Whether the observed sheet content is currently visible.
     */
    boolean isVisible() {
        return mCurrentVisibility;
    }

    /**
     * Compares the current state of the bottom sheet and activity with the ones recorded at the
     * previous call and generates events based on the difference.
     * @see #onContentShown(boolean)
     * @see #onContentHidden()
     * @see #onContentStateChanged(int)
     */
    private void onStateChange() {
        @ActivityState
        int activityState = ApplicationStatus.getStateForActivity(mActivity);
        boolean isActivityVisible =
                activityState == ActivityState.RESUMED || activityState == ActivityState.PAUSED;

        boolean newVisibility =
                isActivityVisible && mBottomSheet.isSheetOpen() && isObservedContentCurrent();

        // As the visibility we track is the one for a specific sheet content rather than the
        // whole BottomSheet, we also need to reflect that in the state, marking it "peeking" here
        // even though the BottomSheet itself is not.
        @BottomSheet.SheetState
        int newContentState =
                newVisibility ? mBottomSheet.getSheetState() : BottomSheet.SHEET_STATE_PEEK;

        // Flag overall changes to the visible state of the content, while ignoring transient states
        // like |STATE_SCROLLING|.
        boolean hasMeaningfulStateChange = BottomSheet.isStateStable(newContentState)
                && (mCurrentContentState != newContentState || mCurrentVisibility != newVisibility);

        if (newVisibility != mCurrentVisibility) {
            if (newVisibility) {
                onContentShown(!mWasShownSinceLastOpen);
                mWasShownSinceLastOpen = true;
            } else {
                onContentHidden();
            }
            mCurrentVisibility = newVisibility;
        }

        if (hasMeaningfulStateChange) {
            onContentStateChanged(newContentState);
            mCurrentContentState = newContentState;
        }
    }

    @VisibleForTesting
    protected boolean isObservedContentCurrent() {
        return mContentObserved == mBottomSheet.getCurrentSheetContent();
    }
}
