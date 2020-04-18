// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.TimeAnimator;
import android.animation.TimeAnimator.TimeListener;
import android.animation.TimeInterpolator;
import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.support.v4.view.ViewCompat;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.animation.AccelerateInterpolator;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ProgressBar;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.vr_shell.VrShellDelegate;
import org.chromium.ui.UiUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

/**
 * Progress bar for use in the Toolbar view. If no progress updates are received for 5 seconds, an
 * indeterminate animation will begin playing and the animation will move across the screen smoothly
 * instead of jumping.
 */
public class ToolbarProgressBar extends ClipDrawableProgressBar {

    /**
     * Interface for progress bar animation interpolation logics.
     */
    interface AnimationLogic {
        /**
         * Resets internal data. It must be called on every loading start.
         * @param startProgress The progress for the animation to start at. This is used when the
         *                      animation logic switches.
         */
        void reset(float startProgress);

        /**
         * Returns interpolated progress for animation.
         *
         * @param targetProgress Actual page loading progress.
         * @param frameTimeSec   Duration since the last call.
         * @param resolution     Resolution of the displayed progress bar. Mainly for rounding.
         */
        float updateProgress(float targetProgress, float frameTimeSec, int resolution);
    }

    /**
     * The amount of time in ms that the progress bar has to be stopped before the indeterminate
     * animation starts.
     */
    private static final long ANIMATION_START_THRESHOLD = 5000;
    private static final long HIDE_DELAY_MS = 100;

    private static final float THEMED_BACKGROUND_WHITE_FRACTION = 0.2f;
    private static final float ANIMATION_WHITE_FRACTION = 0.4f;

    private static final long PROGRESS_THROTTLE_UPDATE_INTERVAL = 30;
    private static final float PROGRESS_THROTTLE_MAX_UPDATE_AMOUNT = 0.03f;

    private static final long PROGRESS_FRAME_TIME_CAP_MS = 50;
    private static final long ALPHA_ANIMATION_DURATION_MS = 140;

    /** Whether or not the progress bar has started processing updates. */
    private boolean mIsStarted;

    /** The target progress the smooth animation should move to (when animating smoothly). */
    private float mTargetProgress;

    /** The logic used to animate the progress bar during smooth animation. */
    private AnimationLogic mAnimationLogic;

    /** Whether or not the animation has been initialized. */
    private boolean mAnimationInitialized;

    /** The progress bar's top margin. */
    private int mMarginTop;

    /** The parent view of the progress bar. */
    private ViewGroup mProgressBarContainer;

    /** The number of times the progress bar has started (used for testing). */
    private int mProgressStartCount;

    /** The theme color currently being used. */
    private int mThemeColor;

    /** Whether or not to use the status bar color as the background of the toolbar. */
    private boolean mUseStatusBarColorAsBackground;

    /** The animator responsible for updating progress once it has been throttled. */
    private TimeAnimator mProgressThrottle;

    /** The listener for the progress throttle. */
    private ThrottleTimeListener mProgressThrottleListener;

    /**
     * The indeterminate animating view for the progress bar. This will be null for Android
     * versions < K.
     */
    private ToolbarProgressBarAnimatingView mAnimatingView;

    /** Whether or not the progress bar is attached to the window. */
    private boolean mIsAttachedToWindow;

    private final Runnable mStartSmoothIndeterminate = new Runnable() {
        @Override
        public void run() {
            if (!mIsStarted) return;
            mAnimationLogic.reset(getProgress());
            mSmoothProgressAnimator.start();

            if (mAnimatingView != null) {
                int width =
                        Math.abs(getDrawable().getBounds().right - getDrawable().getBounds().left);
                mAnimatingView.update(getProgress() * width);
                mAnimatingView.startAnimation();
            }
        }
    };

    private final TimeAnimator mSmoothProgressAnimator = new TimeAnimator();
    {
        mSmoothProgressAnimator.setTimeListener(new TimeListener() {
            @Override
            public void onTimeUpdate(TimeAnimator animation, long totalTimeMs, long deltaTimeMs) {
                // If we are at the target progress already, do nothing.
                if (MathUtils.areFloatsEqual(getProgress(), mTargetProgress)) return;

                // Cap progress bar animation frame time so that it doesn't jump too much even when
                // the animation is janky.
                float progress = mAnimationLogic.updateProgress(mTargetProgress,
                        Math.min(deltaTimeMs, PROGRESS_FRAME_TIME_CAP_MS) * 0.001f, getWidth());
                progress = Math.max(progress, 0);

                // TODO(mdjones): Find a sane way to have this call setProgressInternal so the
                // finish logic can be recycled. Consider stopping the progress throttle if the
                // smooth animation is running.
                ToolbarProgressBar.super.setProgress(progress);

                if (mAnimatingView != null) {
                    int width = Math.abs(
                            getDrawable().getBounds().right - getDrawable().getBounds().left);
                    mAnimatingView.update(progress * width);
                }

                // If progress is at 100%, start hiding the progress bar.
                if (MathUtils.areFloatsEqual(getProgress(), 1.f)) finish(true);
            }
        });
    }

    /** A {@link TimeListener} responsible for updating progress once throttling has started. */
    private final class ThrottleTimeListener
            extends AnimatorListenerAdapter implements TimeListener {
        /** Time interpolator for progress updates. */
        private final TimeInterpolator mAccelerateInterpolator = new AccelerateInterpolator();

        /** The target progress for the throttle animator. */
        private float mThrottledProgressTarget;

        /** The number of increments expected to reach the target progress since the last update. */
        private int mExpectedIncrements;

        /** Keeps track of the increment count since the last progress update. */
        private int mCurrentIncrementCount;

        /** The duration the progress update should take to complete. */
        private long mExpectedDuration;

        /** The amount of time until the next update. */
        private long mNextUpdateTime;

        @Override
        public void onAnimationStart(Animator animation) {
            float progressDiff = mThrottledProgressTarget - getProgress();
            mExpectedIncrements =
                    (int) Math.ceil(progressDiff / PROGRESS_THROTTLE_MAX_UPDATE_AMOUNT);
            mExpectedIncrements = Math.max(mExpectedIncrements, 1);
            mCurrentIncrementCount = 0;
            mNextUpdateTime = 0;
            mExpectedDuration = PROGRESS_THROTTLE_UPDATE_INTERVAL * mExpectedIncrements;
        }

        @Override
        public void onTimeUpdate(TimeAnimator animation, long totalTime, long deltaTime) {
            if (totalTime < mNextUpdateTime || mExpectedIncrements <= 0) return;

            mCurrentIncrementCount++;

            float completionFraction = mCurrentIncrementCount / (float) mExpectedIncrements;

            // This uses an accelerate interpolator to produce progressively longer times so the
            // progress bar appears to slow down.
            mNextUpdateTime = (long) (mAccelerateInterpolator.getInterpolation(completionFraction)
                    * mExpectedDuration);

            float updatedProgress = getProgress() + PROGRESS_THROTTLE_MAX_UPDATE_AMOUNT;
            if (updatedProgress >= mThrottledProgressTarget) animation.end();

            setProgressInternal(MathUtils.clamp(updatedProgress, 0f, mThrottledProgressTarget));
        }
    }

    /**
     * Creates a toolbar progress bar.
     *
     * @param context The application environment.
     * @param height The height of the progress bar in px.
     * @param topMargin The top margin of the progress bar.
     * @param useStatusBarColorAsBackground Whether or not to use the status bar color as the
     *                                      background of the toolbar.
     */
    public ToolbarProgressBar(
            Context context, int height, int topMargin, boolean useStatusBarColorAsBackground) {
        super(context, height);
        setAlpha(0.0f);
        mMarginTop = topMargin;
        mUseStatusBarColorAsBackground = useStatusBarColorAsBackground;
        mAnimationLogic = new ProgressAnimationSmooth();

        // This tells accessibility services that progress bar changes are important enough to
        // announce to the user even when not focused.
        ViewCompat.setAccessibilityLiveRegion(this, ViewCompat.ACCESSIBILITY_LIVE_REGION_POLITE);
    }

    /**
     * Set the top progress bar's top margin.
     * @param topMargin The top margin of the progress bar in px.
     */
    public void setTopMargin(int topMargin) {
        mMarginTop = topMargin;

        if (mIsAttachedToWindow) {
            assert getLayoutParams() != null;
            ((ViewGroup.MarginLayoutParams) getLayoutParams()).topMargin = mMarginTop;
        }
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        mIsAttachedToWindow = true;

        ((ViewGroup.MarginLayoutParams) getLayoutParams()).topMargin = mMarginTop;
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mIsAttachedToWindow = false;
    }

    /**
     * @param container The view containing the progress bar.
     */
    public void setProgressBarContainer(ViewGroup container) {
        mProgressBarContainer = container;
    }

    @Override
    public void setAlpha(float alpha) {
        super.setAlpha(alpha);
        if (mAnimatingView != null) mAnimatingView.setAlpha(alpha);
    }

    @Override
    public void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        // If the size changed, the animation width needs to be manually updated.
        if (mAnimatingView != null) mAnimatingView.update(width * getProgress());
    }

    /**
     * Initializes animation based on command line configuration. This must be called when native
     * library is ready.
     */
    public void initializeAnimation() {
        if (mAnimationInitialized) return;

        mAnimationInitialized = true;

        // Only use the indeterminate animation if the Android version is > J.
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN_MR2) {
            LayoutParams animationParams = new LayoutParams(getLayoutParams());
            animationParams.width = 1;
            animationParams.topMargin = mMarginTop;

            mAnimatingView = new ToolbarProgressBarAnimatingView(getContext(), animationParams);

            // The primary theme color may not have been set.
            if (mThemeColor != 0 || mUseStatusBarColorAsBackground) {
                setThemeColor(mThemeColor, false);
            } else {
                setForegroundColor(getForegroundColor());
            }
            UiUtils.insertAfter(mProgressBarContainer, mAnimatingView, this);
        }
    }

    /**
     * Start showing progress bar animation.
     */
    public void start() {
        ThreadUtils.assertOnUiThread();

        mIsStarted = true;
        mProgressStartCount++;

        removeCallbacks(mStartSmoothIndeterminate);
        postDelayed(mStartSmoothIndeterminate, ANIMATION_START_THRESHOLD);

        super.setProgress(0.0f);
        mAnimationLogic.reset(0.0f);
        animateAlphaTo(1.0f);
    }

    /**
     * @return True if the progress bar is showing and started.
     */
    public boolean isStarted() {
        return mIsStarted;
    }

    /**
     * Start hiding progress bar animation. Progress does not necessarily need to be at 100% to
     * finish. If 'fadeOut' is set to true, progress will forced to 100% (if not already) and then
     * fade out. If false, the progress will hide regardless of where it currently is.
     * @param fadeOut Whether the progress bar should fade out. If false, the progress bar will
     *                disappear immediately, regardless of animation.
     *                TODO(mdjones): This param should be "force" but involves inverting all calls
     *                to this method.
     */
    public void finish(boolean fadeOut) {
        ThreadUtils.assertOnUiThread();

        if (!MathUtils.areFloatsEqual(getProgress(), 1.0f)) {
            // If any of the animators are running while this method is called, set the internal
            // progress and wait for the animation to end.
            setProgress(1.0f);
            if (areProgressAnimatorsRunning() && fadeOut) return;
        }

        mIsStarted = false;
        mTargetProgress = 0;

        removeCallbacks(mStartSmoothIndeterminate);
        if (mAnimatingView != null) mAnimatingView.cancelAnimation();
        if (mProgressThrottle != null) mProgressThrottle.cancel();
        mSmoothProgressAnimator.cancel();

        if (fadeOut) {
            postDelayed(() -> hideProgressBar(true), HIDE_DELAY_MS);
        } else {
            hideProgressBar(false);
        }
    }

    /**
     * Hide the progress bar.
     * @param animate Whether to animate the opacity.
     */
    private void hideProgressBar(boolean animate) {
        ThreadUtils.assertOnUiThread();

        if (mIsStarted) return;
        if (!animate) animate().cancel();

        // Make invisible.
        if (animate) {
            animateAlphaTo(0.0f);
        } else {
            setAlpha(0.0f);
        }
    }

    /**
     * @return Whether any animator that delays the showing of progress is running.
     */
    private boolean areProgressAnimatorsRunning() {
        return (mProgressThrottle != null && mProgressThrottle.isRunning())
                || mSmoothProgressAnimator.isRunning();
    }

    /**
     * Animate the alpha of all of the parts of the progress bar.
     * @param targetAlpha The alpha in range [0, 1] to animate to.
     */
    private void animateAlphaTo(float targetAlpha) {
        float alphaDiff = targetAlpha - getAlpha();
        if (alphaDiff == 0.0f) return;

        long duration = (long) Math.abs(alphaDiff * ALPHA_ANIMATION_DURATION_MS);

        BakedBezierInterpolator interpolator = BakedBezierInterpolator.FADE_IN_CURVE;
        if (alphaDiff < 0) interpolator = BakedBezierInterpolator.FADE_OUT_CURVE;

        animate().alpha(targetAlpha)
                .setDuration(duration)
                .setInterpolator(interpolator);

        if (mAnimatingView != null) {
            mAnimatingView.animate().alpha(targetAlpha)
                    .setDuration(duration)
                    .setInterpolator(interpolator);
        }
    }

    // ClipDrawableProgressBar implementation.

    @Override
    public void setProgress(float progress) {
        ThreadUtils.assertOnUiThread();

        // TODO(mdjones): Maybe subclass this to be ThrottledToolbarProgressBar.
        if (mProgressThrottle == null && ChromeFeatureList.isInitialized()
                && ChromeFeatureList.isEnabled(ChromeFeatureList.PROGRESS_BAR_THROTTLE)) {
            mProgressThrottle = new TimeAnimator();
            mProgressThrottleListener = new ThrottleTimeListener();
            mProgressThrottle.addListener(mProgressThrottleListener);
            mProgressThrottle.setTimeListener(mProgressThrottleListener);
        }

        // Throttle progress if the increment was greater than 5%.
        if (mProgressThrottle != null
                && (progress - getProgress() > PROGRESS_THROTTLE_MAX_UPDATE_AMOUNT
                           || mProgressThrottle.isRunning())) {
            mProgressThrottleListener.mThrottledProgressTarget = progress;

            mProgressThrottle.cancel();
            mProgressThrottle.start();
        } else {
            setProgressInternal(progress);
        }
    }

    /**
     * Set the progress bar state based on the external updates coming in.
     * @param progress The current progress.
     */
    private void setProgressInternal(float progress) {
        if (!mIsStarted || MathUtils.areFloatsEqual(mTargetProgress, progress)) return;
        mTargetProgress = progress;

        // If the progress bar was updated, reset the callback that triggers the
        // smooth-indeterminate animation.
        removeCallbacks(mStartSmoothIndeterminate);

        if (!mSmoothProgressAnimator.isRunning()) {
            postDelayed(mStartSmoothIndeterminate, ANIMATION_START_THRESHOLD);
            super.setProgress(mTargetProgress);
        }

        sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_SELECTED);

        if (MathUtils.areFloatsEqual(progress, 1.0f) || progress > 1.0f) finish(true);
    }

    @Override
    public void setVisibility(int visibility) {
        // The progress bar should never show up while in VR.
        if (VrShellDelegate.isInVr()) visibility = GONE;
        super.setVisibility(visibility);
        if (mAnimatingView != null) mAnimatingView.setVisibility(visibility);
    }

    /**
     * Color the progress bar based on the toolbar theme color.
     * @param color The Android color the toolbar is using.
     */
    public void setThemeColor(int color, boolean isIncognito) {
        mThemeColor = color;
        boolean isDefaultTheme = ColorUtils.isUsingDefaultToolbarColor(
                getResources(), mUseStatusBarColorAsBackground, isIncognito, mThemeColor);

        // All colors use a single path if using the status bar color as the background.
        if (mUseStatusBarColorAsBackground) {
            if (isDefaultTheme) color = Color.BLACK;
            setForegroundColor(
                    ApiCompatibilityUtils.getColor(getResources(), R.color.white_alpha_70));
            setBackgroundColor(ColorUtils.getDarkenedColorForStatusBar(color));
            return;
        }

        // The default toolbar has specific colors to use.
        if ((isDefaultTheme || !ColorUtils.isValidThemeColor(color)) && !isIncognito) {
            setForegroundColor(ApiCompatibilityUtils.getColor(getResources(),
                    R.color.progress_bar_foreground));
            setBackgroundColor(ApiCompatibilityUtils.getColor(getResources(),
                    R.color.progress_bar_background));
            return;
        }

        setForegroundColor(ColorUtils.getThemedAssetColor(color, isIncognito));

        if (mAnimatingView != null
                && (ColorUtils.shouldUseLightForegroundOnBackground(color) || isIncognito)) {
            mAnimatingView.setColor(ColorUtils.getColorWithOverlay(color, Color.WHITE,
                    ANIMATION_WHITE_FRACTION));
        }

        setBackgroundColor(ColorUtils.getColorWithOverlay(color, Color.WHITE,
                THEMED_BACKGROUND_WHITE_FRACTION));
    }

    @Override
    public void setForegroundColor(int color) {
        super.setForegroundColor(color);
        if (mAnimatingView != null) {
            mAnimatingView.setColor(ColorUtils.getColorWithOverlay(color, Color.WHITE,
                    ANIMATION_WHITE_FRACTION));
        }
    }

    @Override
    public CharSequence getAccessibilityClassName() {
        return ProgressBar.class.getName();
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setCurrentItemIndex((int) (mTargetProgress * 100));
        event.setItemCount(100);
    }

    /**
     * @return The number of times the progress bar has been triggered.
     */
    @VisibleForTesting
    public int getStartCountForTesting() {
        return mProgressStartCount;
    }

    /**
     * Reset the number of times the progress bar has been triggered.
     */
    @VisibleForTesting
    public void resetStartCountForTesting() {
        mProgressStartCount = 0;
    }

    /**
     * Start the indeterminate progress bar animation.
     */
    @VisibleForTesting
    public void startIndeterminateAnimationForTesting() {
        mStartSmoothIndeterminate.run();
    }

    /**
     * @return The indeterminate animator.
     */
    @VisibleForTesting
    public Animator getIndeterminateAnimatorForTesting() {
        return mSmoothProgressAnimator;
    }
}
