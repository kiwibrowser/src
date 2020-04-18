/*
 * Copyright (C) 2011 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.ex.photo;

import android.content.Context;
import android.support.v4.view.MotionEventCompat;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

/**
 * View pager for photo view fragments. Define our own class so we can specify the
 * view pager in XML.
 */
public class PhotoViewPager extends ViewPager {
    /**
     * A type of intercept that should be performed
     */
    public static enum InterceptType { NONE, LEFT, RIGHT, BOTH }

    /**
     * Provides an ability to intercept touch events.
     * <p>
     * {@link ViewPager} intercepts all touch events and we need to be able to override this
     * behavior. Instead, we could perform a similar function by declaring a custom
     * {@link android.view.ViewGroup} to contain the pager and intercept touch events at a higher
     * level.
     */
    public static interface OnInterceptTouchListener {
        /**
         * Called when a touch intercept is about to occur.
         *
         * @param origX the raw x coordinate of the initial touch
         * @param origY the raw y coordinate of the initial touch
         * @return Which type of touch, if any, should should be intercepted.
         */
        public InterceptType onTouchIntercept(float origX, float origY);
    }

    private static final int INVALID_POINTER = -1;

    private float mLastMotionX;
    private int mActivePointerId;
    /** The x coordinate where the touch originated */
    private float mActivatedX;
    /** The y coordinate where the touch originated */
    private float mActivatedY;
    private OnInterceptTouchListener mListener;

    public PhotoViewPager(Context context) {
        super(context);
        initialize();
    }

    public PhotoViewPager(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialize();
    }

    private void initialize() {
        // Set the page transformer to perform the transition animation
        // for each page in the view.
        setPageTransformer(true, new PageTransformer() {
            @Override
            public void transformPage(View page, float position) {

                // The >= 1 is needed so that the page
                // (page A) that transitions behind the newly visible
                // page (page B) that comes in from the left does not
                // get the touch events because it is still on screen
                // (page A is still technically on screen despite being
                // invisible). This makes sure that when the transition
                // has completely finished, we revert it to its default
                // behavior and move it off of the screen.
                if (position < 0 || position >= 1.f) {
                    page.setTranslationX(0);
                    page.setAlpha(1.f);
                    page.setScaleX(1);
                    page.setScaleY(1);
                } else {
                    page.setTranslationX(-position * page.getWidth());
                    page.setAlpha(Math.max(0,1.f - position));
                    final float scale = Math.max(0, 1.f - position * 0.3f);
                    page.setScaleX(scale);
                    page.setScaleY(scale);
                }
            }
        });
    }

    /**
     * {@inheritDoc}
     * <p>
     * We intercept touch event intercepts so we can prevent switching views when the
     * current view is internally scrollable.
     */
    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        final InterceptType intercept = (mListener != null)
                ? mListener.onTouchIntercept(mActivatedX, mActivatedY)
                : InterceptType.NONE;
        final boolean ignoreScrollLeft =
                (intercept == InterceptType.BOTH || intercept == InterceptType.LEFT);
        final boolean ignoreScrollRight =
                (intercept == InterceptType.BOTH || intercept == InterceptType.RIGHT);

        // Only check ability to page if we can't scroll in one / both directions
        final int action = ev.getAction() & MotionEventCompat.ACTION_MASK;

        if (action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_UP) {
            mActivePointerId = INVALID_POINTER;
        }

        switch (action) {
            case MotionEvent.ACTION_MOVE: {
                if (ignoreScrollLeft || ignoreScrollRight) {
                    final int activePointerId = mActivePointerId;
                    if (activePointerId == INVALID_POINTER) {
                        // If we don't have a valid id, the touch down wasn't on content.
                        break;
                    }

                    final int pointerIndex =
                            MotionEventCompat.findPointerIndex(ev, activePointerId);
                    final float x = MotionEventCompat.getX(ev, pointerIndex);

                    if (ignoreScrollLeft && ignoreScrollRight) {
                        mLastMotionX = x;
                        return false;
                    } else if (ignoreScrollLeft && (x > mLastMotionX)) {
                        mLastMotionX = x;
                        return false;
                    } else if (ignoreScrollRight && (x < mLastMotionX)) {
                        mLastMotionX = x;
                        return false;
                    }
                }
                break;
            }

            case MotionEvent.ACTION_DOWN: {
                mLastMotionX = ev.getX();
                // Use the raw x/y as the children can be located anywhere and there isn't a
                // single offset that would be meaningful
                mActivatedX = ev.getRawX();
                mActivatedY = ev.getRawY();
                mActivePointerId = MotionEventCompat.getPointerId(ev, 0);
                break;
            }

            case MotionEventCompat.ACTION_POINTER_UP: {
                final int pointerIndex = MotionEventCompat.getActionIndex(ev);
                final int pointerId = MotionEventCompat.getPointerId(ev, pointerIndex);
                if (pointerId == mActivePointerId) {
                    // Our active pointer going up; select a new active pointer
                    final int newPointerIndex = pointerIndex == 0 ? 1 : 0;
                    mLastMotionX = MotionEventCompat.getX(ev, newPointerIndex);
                    mActivePointerId = MotionEventCompat.getPointerId(ev, newPointerIndex);
                }
                break;
            }
        }

        return super.onInterceptTouchEvent(ev);
    }

    /**
     * sets the intercept touch listener.
     */
    public void setOnInterceptTouchListener(OnInterceptTouchListener l) {
        mListener = l;
    }
}
