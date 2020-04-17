// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.gesturenav;

import android.content.Context;
import android.support.annotation.IntDef;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Transformation;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.third_party.android.swiperefresh.CircleImageView;

/**
 * The SideSlideLayout can be used whenever the user navigates the contents
 * of a view using horizontal gesture. Shows an arrow widget moving horizontally
 * in reaction to the gesture which, if goes over a threshold, triggers navigation.
 * The caller that instantiates this view should add an {@link #OnNavigateListener}
 * to be notified whenever the gesture is completed.
 * Based on {@link org.chromium.third_party.android.swiperefresh.SwipeRefreshLayout}
 * and modified accordingly to support horizontal gesture.
 */
public class SideSlideLayout extends ViewGroup {
    // Used to record the UMA histogram Overscroll.* This definition should be
    // in sync with that in content/browser/web_contents/aura/types.h
    // TODO(jinsukkim): Generate java enum from the native header.
    @IntDef({UmaNavigationType.NAVIGATION_TYPE_NONE, UmaNavigationType.FORWARD_TOUCHPAD,
            UmaNavigationType.BACK_TOUCHPAD, UmaNavigationType.FORWARD_TOUCHSCREEN,
            UmaNavigationType.BACK_TOUCHSCREEN, UmaNavigationType.RELOAD_TOUCHPAD,
            UmaNavigationType.RELOAD_TOUCHSCREEN, UmaNavigationType.NAVIGATION_TYPE_COUNT})
    private @interface UmaNavigationType {
        int NAVIGATION_TYPE_NONE = 0;
        int FORWARD_TOUCHPAD = 1;
        int BACK_TOUCHPAD = 2;
        int FORWARD_TOUCHSCREEN = 3;
        int BACK_TOUCHSCREEN = 4;
        int RELOAD_TOUCHPAD = 5;
        int RELOAD_TOUCHSCREEN = 6;
        int NAVIGATION_TYPE_COUNT = 7;
    }

    /**
     * Classes that wish to be notified when the swipe gesture correctly
     * triggers navigation should implement this interface.
     */
    public interface OnNavigateListener { void onNavigate(boolean isForward); }

    /**
     * Classes that wish to be notified when a reset is triggered should
     * implement this interface.
     */
    public interface OnResetListener { void onReset(); }

    private static final int MAX_ALPHA = 255;
    private static final int STARTING_ALPHA = (int) (.3f * MAX_ALPHA);

    private static final int CIRCLE_DIAMETER_DP = 40;
    private static final int MAX_CIRCLE_RADIUS_DP = 30;

    // Offset in dips from the border of the view. Gesture triggers the navigation
    // if slid by this amount or more.
    private static final int TARGET_THRESHOLD_DP = 64;

    private static final float DECELERATE_INTERPOLATION_FACTOR = 2f;

    private static final int SCALE_DOWN_DURATION_MS = 500;
    private static final int ANIMATE_TO_START_DURATION_MS = 500;

    // Minimum number of pull updates necessary to trigger a side nav.
    private static final int MIN_PULLS_TO_ACTIVATE = 1;

    private final DecelerateInterpolator mDecelerateInterpolator;
    private final float mTotalDragDistance;
    private final int mMediumAnimationDuration;
    private final int mCircleWidth;
    private final int mCircleHeight;

    private OnNavigateListener mListener;
    private OnResetListener mResetListener;

    // Flag indicating that the navigation will be activated.
    private boolean mNavigating;

    private int mCurrentTargetOffset;
    private float mTotalMotionY;

    // Whether or not the starting offset has been determined.
    private boolean mOriginalOffsetCalculated;

    // True while side gesture is in progress.
    private boolean mIsBeingDragged;

    private CircleImageView mCircleView;
    private ArrowDrawable mArrow;

    // Start position for animation moving the UI back to original offset.
    private int mFrom;
    private int mOriginalOffset;

    private Animation mScaleDownAnimation;
    private AnimationListener mCancelAnimationListener;

    private boolean mIsForward;

    private final AnimationListener mNavigateListener = new AnimationListener() {
        @Override
        public void onAnimationStart(Animation animation) {}

        @Override
        public void onAnimationRepeat(Animation animation) {}

        @Override
        public void onAnimationEnd(Animation animation) {
            if (mNavigating) {
                // Make sure the arrow widget is fully visible
                mArrow.setAlpha(MAX_ALPHA);
                if (mListener != null) mListener.onNavigate(mIsForward);
                recordHistogram("Overscroll.Navigated3", mIsForward);
            } else {
                reset();
            }
            mCurrentTargetOffset = mCircleView.getLeft();
        }
    };

    private final Animation mAnimateToStartPosition = new Animation() {
        @Override
        public void applyTransformation(float interpolatedTime, Transformation t) {
            int targetTop = mFrom + (int) ((mOriginalOffset - mFrom) * interpolatedTime);
            int offset = targetTop - mCircleView.getLeft();
            mTotalMotionY += offset;

            float progress = Math.min(1.f, Math.abs(mTotalMotionY) / mTotalDragDistance);
            mCircleView.setProgress(progress);
            setTargetOffsetLeftAndRight(offset);
        }
    };

    public SideSlideLayout(Context context) {
        super(context);

        mMediumAnimationDuration =
                getResources().getInteger(android.R.integer.config_mediumAnimTime);

        setWillNotDraw(false);
        mDecelerateInterpolator = new DecelerateInterpolator(DECELERATE_INTERPOLATION_FACTOR);

        final float density = getResources().getDisplayMetrics().density;
        mCircleWidth = (int) (CIRCLE_DIAMETER_DP * density);
        mCircleHeight = (int) (CIRCLE_DIAMETER_DP * density);

        mCircleView = new CircleImageView(getContext(), R.color.white_alpha_70, CIRCLE_DIAMETER_DP / 2,
                MAX_CIRCLE_RADIUS_DP, R.color.white_alpha_70);

        mArrow = new ArrowDrawable(getContext().getResources());
        mArrow.setBackgroundColor(R.color.white_alpha_70);
        mArrow.setForegroundColor(R.color.white_alpha_70);
        mCircleView.setImageDrawable(mArrow);
        mCircleView.setVisibility(View.GONE);
        addView(mCircleView);

        // The absolute offset has to take into account that the circle starts at an offset
        mTotalDragDistance = TARGET_THRESHOLD_DP * density;
    }

    /**
     * Set the listener to be notified when the navigation is triggered.
     */
    public void setOnNavigationListener(OnNavigateListener listener) {
        mListener = listener;
    }

    /**
     * Set the reset listener to be notified when a reset is triggered.
     */
    public void setOnResetListener(OnResetListener listener) {
        mResetListener = listener;
    }

    /**
     * Stop navigation.
     */
    public void stopNavigating() {
        setNavigating(false);
    }

    private void setNavigating(boolean navigating) {
        if (mNavigating != navigating) {
            mNavigating = navigating;
            if (mNavigating) startScaleDownAnimation(mNavigateListener);
        }
    }

    private void startScaleDownAnimation(AnimationListener listener) {
        if (mScaleDownAnimation == null) {
            mScaleDownAnimation = new Animation() {
                @Override
                public void applyTransformation(float interpolatedTime, Transformation t) {
                    float progress = 1 - interpolatedTime; // [0..1]
                    mCircleView.setScaleX(progress);
                    mCircleView.setScaleY(progress);
                }
            };
            mScaleDownAnimation.setDuration(SCALE_DOWN_DURATION_MS);
        }
        mCircleView.setAnimationListener(listener);
        mCircleView.clearAnimation();
        mCircleView.startAnimation(mScaleDownAnimation);
    }

    /**
     * Set the direction used for sliding gesture.
     * @param forward {@code true} if direction is forward.
     */
    public void setDirection(boolean forward) {
        mIsForward = forward;
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        if (getChildCount() == 0) return;

        final int height = getMeasuredHeight();
        int circleWidth = mCircleView.getMeasuredWidth();
        int circleHeight = mCircleView.getMeasuredHeight();
        mCircleView.layout(mCurrentTargetOffset, height / 2 - circleHeight / 2,
                mCurrentTargetOffset + circleWidth, height / 2 + circleHeight / 2);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        mCircleView.measure(MeasureSpec.makeMeasureSpec(mCircleWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(mCircleHeight, MeasureSpec.EXACTLY));
        if (!mOriginalOffsetCalculated) {
            initializeOffset();
            mOriginalOffsetCalculated = true;
        }
    }

    private void initializeOffset() {
        mCurrentTargetOffset = mOriginalOffset =
                mIsForward ? getMeasuredWidth() : -mCircleView.getMeasuredWidth();
    }

    /**
     * Start the pull effect. If the effect is disabled or a navigation animation
     * is currently active, the request will be ignored.
     * @return whether a new pull sequence has started.
     */
    public boolean start() {
        if (!isEnabled() || mNavigating || mListener == null) return false;
        mCircleView.clearAnimation();
        mTotalMotionY = 0;
        mIsBeingDragged = true;
        mArrow.setAlpha(STARTING_ALPHA);
        mArrow.setDirection(mIsForward);
        initializeOffset();
        return true;
    }

    /**
     * Apply a pull impulse to the effect. If the effect is disabled or has yet
     * to start, the pull will be ignored.
     * @param delta the magnitude of the pull.
     */
    public void pull(float delta) {
        if (!isEnabled() || !mIsBeingDragged) return;

        float maxDelta = mTotalDragDistance / MIN_PULLS_TO_ACTIVATE;
        delta = Math.max(-maxDelta, Math.min(maxDelta, delta));
        mTotalMotionY += delta;

        float overscroll = mTotalMotionY;
        float extraOs = Math.abs(overscroll) - mTotalDragDistance;
        float slingshotDist = mTotalDragDistance;
        float tensionSlingshotPercent =
                Math.max(0, Math.min(extraOs, slingshotDist * 2) / slingshotDist);
        float tensionPercent =
                (float) ((tensionSlingshotPercent / 4) - Math.pow((tensionSlingshotPercent / 4), 2))
                * 2f;

        if (mCircleView.getVisibility() != View.VISIBLE) mCircleView.setVisibility(View.VISIBLE);
        mCircleView.setScaleX(1f);
        mCircleView.setScaleY(1f);

        float originalDragPercent = Math.abs(overscroll) / mTotalDragDistance;
        float dragPercent = Math.min(1f, Math.abs(originalDragPercent));
        float adjustedPercent = (float) Math.max(dragPercent - .4, 0) * 5 / 3;
        mArrow.setArrowScale(Math.min(1f, adjustedPercent));

        float alphaStrength = Math.max(0f, Math.min(1f, (dragPercent - .9f) / .1f));
        mArrow.setAlpha(STARTING_ALPHA + (int) (alphaStrength * (MAX_ALPHA - STARTING_ALPHA)));
        mCircleView.setProgress(Math.min(1.f, Math.abs(overscroll) / mTotalDragDistance));

        float extraMove = slingshotDist * tensionPercent * 2;
        int targetDiff = (int) (slingshotDist * dragPercent + extraMove);
        int targetX = mOriginalOffset + (mIsForward ? -targetDiff : targetDiff);
        setTargetOffsetLeftAndRight(targetX - mCurrentTargetOffset);
    }

    private void setTargetOffsetLeftAndRight(int offset) {
        mCircleView.offsetLeftAndRight(offset);
        mCurrentTargetOffset = mCircleView.getLeft();
    }

    /**
     * Release the active pull. If no pull has started, the release will be ignored.
     * If the pull was sufficiently large, the navigation sequence will be initiated.
     * @param allowNav whether to allow a sufficiently large pull to trigger
     *                     the navigation action and animation sequence.
     */
    public void release(boolean allowNav) {
        if (!mIsBeingDragged) return;

        // See ACTION_UP handling in {@link #onTouchEvent(...)}.
        mIsBeingDragged = false;
        final float overscroll = Math.abs(mTotalMotionY);
        if (isEnabled() && allowNav && overscroll > mTotalDragDistance) {
            setNavigating(true);
            return;
        }
        // Cancel navigation
        mNavigating = false;
        if (mCancelAnimationListener == null) {
            mCancelAnimationListener = new AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {}

                @Override
                public void onAnimationEnd(Animation animation) {
                    startScaleDownAnimation(mNavigateListener);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {}
            };
        }
        mFrom = mCurrentTargetOffset;
        mAnimateToStartPosition.reset();
        mAnimateToStartPosition.setDuration(ANIMATE_TO_START_DURATION_MS);
        mAnimateToStartPosition.setInterpolator(mDecelerateInterpolator);
        mCircleView.setAnimationListener(mCancelAnimationListener);
        mCircleView.clearAnimation();
        mCircleView.startAnimation(mAnimateToStartPosition);
        recordHistogram("Overscroll.Cancelled3", mIsForward);
    }

    /**
     * Reset the effect, clearing any active animations.
     */
    public void reset() {
        mIsBeingDragged = false;
        setNavigating(false);
        mCircleView.setVisibility(View.GONE);
        mCircleView.getBackground().setAlpha(MAX_ALPHA);
        mArrow.setAlpha(MAX_ALPHA);

        // Return the circle to its start position
        setTargetOffsetLeftAndRight(mOriginalOffset - mCurrentTargetOffset);
        mCurrentTargetOffset = mCircleView.getLeft();
        if (mResetListener != null) mResetListener.onReset();
    }

    private static void recordHistogram(String name, boolean forward) {
        RecordHistogram.recordEnumeratedHistogram(name,
                forward ? UmaNavigationType.FORWARD_TOUCHSCREEN
                        : UmaNavigationType.BACK_TOUCHSCREEN,
                UmaNavigationType.NAVIGATION_TYPE_COUNT);
    }
}
