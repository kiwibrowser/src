// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting;

/**
 * A state machine to indicate user input actions. It stores the start action (tap or long tap),
 * finger count, detected action, etc.
 */
public class InputState {
    /**
     * A settable {@link InputState}.
     */
    public static final class Settable extends InputState {
        public void setFingerCount(int fingerCount) {
            mFingerCount = fingerCount;
            if (mFingerCount == 0) {
                mStartAction = StartAction.UNDEFINED;
                mDetectedAction = DetectedAction.UNDEFINED;
            }
        }

        public void setStartAction(StartAction startAction) {
            Preconditions.isTrue(startAction != StartAction.UNDEFINED);
            mStartAction = startAction;
        }

        public void setDetectedAction(DetectedAction detectedAction) {
            Preconditions.isTrue(detectedAction != DetectedAction.UNDEFINED);
            mDetectedAction = detectedAction;
        }
    }

    public enum StartAction {
        UNDEFINED,
        // The action started from a long press. Note, a tap won't need to impact InputState.
        LONG_PRESS,
    }

    public enum DetectedAction {
        UNDEFINED,
        SCROLL,
        SCROLL_FLING,
        // AFTER_SCROLL_FLING is a fake action to indicate the state after a scroll fling has been
        // performed.
        AFTER_SCROLL_FLING,
        FLING,
        SCALE,
        SWIPE,
        MOVE,
        SCROLL_EDGE,
    }

    protected int mFingerCount;
    protected StartAction mStartAction;
    protected DetectedAction mDetectedAction;

    public InputState() {
        mStartAction = StartAction.UNDEFINED;
        mFingerCount = 0;
        mDetectedAction = DetectedAction.UNDEFINED;
    }

    public int getFingerCount() {
        return mFingerCount;
    }

    public StartAction getStartAction() {
        return mStartAction;
    }

    public DetectedAction getDetectedAction() {
        return mDetectedAction;
    }

    public boolean shouldSuppressCursorMovement() {
        return mDetectedAction == DetectedAction.SWIPE
                || mDetectedAction == DetectedAction.SCROLL_FLING
                || mDetectedAction == DetectedAction.SCROLL_EDGE;
    }

    public boolean shouldSuppressFling() {
        return mDetectedAction == DetectedAction.SWIPE
                || mStartAction == StartAction.LONG_PRESS;
    }

    public boolean isScrollFling() {
        return mDetectedAction == DetectedAction.SCROLL_FLING;
    }

    public boolean swipeCompleted() {
        return mDetectedAction == DetectedAction.SWIPE;
    }

    public boolean isDragging() {
        return mStartAction == StartAction.LONG_PRESS;
    }
}
