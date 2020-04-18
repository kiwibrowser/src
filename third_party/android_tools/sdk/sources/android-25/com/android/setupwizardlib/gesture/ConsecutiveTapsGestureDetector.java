/*
 * Copyright (C) 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.setupwizardlib.gesture;

import android.graphics.Rect;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

/**
 * Helper class to detect the consective-tap gestures on a view.
 *
 * <p/>This class is instantiated and used similar to a GestureDetector, where onTouchEvent should
 * be called when there are MotionEvents this detector should know about.
 */
public final class ConsecutiveTapsGestureDetector {

    public interface OnConsecutiveTapsListener {
        /**
         * Callback method when the user tapped on the target view X number of times.
         */
        void onConsecutiveTaps(int numOfConsecutiveTaps);
    }

    private final View mView;
    private final OnConsecutiveTapsListener mListener;
    private final int mConsecutiveTapTouchSlopSquare;

    private int mConsecutiveTapsCounter = 0;
    private MotionEvent mPreviousTapEvent;

    /**
     * @param listener The listener that responds to the gesture.
     * @param view  The target view that associated with consecutive-tap gesture.
     */
    public ConsecutiveTapsGestureDetector(
            OnConsecutiveTapsListener listener,
            View view) {
        mListener = listener;
        mView = view;
        int doubleTapSlop = ViewConfiguration.get(mView.getContext()).getScaledDoubleTapSlop();
        mConsecutiveTapTouchSlopSquare = doubleTapSlop * doubleTapSlop;
    }

    /**
     * This method should be called from the relevant activity or view, typically in
     * onTouchEvent, onInterceptTouchEvent or dispatchTouchEvent.
     *
     * @param ev The motion event
     */
    public void onTouchEvent(MotionEvent ev) {
        if (ev.getAction() == MotionEvent.ACTION_UP) {
            Rect viewRect = new Rect();
            int[] leftTop = new int[2];
            mView.getLocationOnScreen(leftTop);
            viewRect.set(
                    leftTop[0],
                    leftTop[1],
                    leftTop[0] + mView.getWidth(),
                    leftTop[1] + mView.getHeight());
            if (viewRect.contains((int) ev.getX(), (int) ev.getY())) {
                if (isConsecutiveTap(ev)) {
                    mConsecutiveTapsCounter++;
                } else {
                    mConsecutiveTapsCounter = 1;
                }
                mListener.onConsecutiveTaps(mConsecutiveTapsCounter);
            } else {
                // Touch outside the target view. Reset counter.
                mConsecutiveTapsCounter = 0;
            }

            if (mPreviousTapEvent != null) {
                mPreviousTapEvent.recycle();
            }
            mPreviousTapEvent = MotionEvent.obtain(ev);
        }
    }

    /**
     * Resets the consecutive-tap counter to zero.
     */
    public void resetCounter() {
        mConsecutiveTapsCounter = 0;
    }

    /**
     * Returns true if the distance between consecutive tap is within
     * {@link #mConsecutiveTapTouchSlopSquare}. False, otherwise.
     */
    private boolean isConsecutiveTap(MotionEvent currentTapEvent) {
        if (mPreviousTapEvent == null) {
            return false;
        }

        double deltaX = mPreviousTapEvent.getX() - currentTapEvent.getX();
        double deltaY = mPreviousTapEvent.getY() - currentTapEvent.getY();
        return (deltaX * deltaX + deltaY * deltaY <= mConsecutiveTapTouchSlopSquare);
    }
}
