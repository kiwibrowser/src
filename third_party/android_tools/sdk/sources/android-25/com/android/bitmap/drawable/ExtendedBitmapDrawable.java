/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.bitmap.drawable;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.util.Log;
import android.view.animation.LinearInterpolator;

import com.android.bitmap.BitmapCache;
import com.android.bitmap.DecodeAggregator;
import com.android.bitmap.DecodeTask;
import com.android.bitmap.R;
import com.android.bitmap.RequestKey;
import com.android.bitmap.ReusableBitmap;
import com.android.bitmap.util.Trace;

/**
 * This class encapsulates all functionality needed to display a single image bitmap,
 * including request creation/cancelling, data unbinding and re-binding, and fancy animations
 * to draw upon state changes.
 * <p>
 * The actual bitmap decode work is handled by {@link DecodeTask}.
 */
public class ExtendedBitmapDrawable extends BasicBitmapDrawable implements
    Runnable, Parallaxable, DecodeAggregator.Callback {

    public static final int LOAD_STATE_UNINITIALIZED = 0;
    public static final int LOAD_STATE_NOT_YET_LOADED = 1;
    public static final int LOAD_STATE_LOADING = 2;
    public static final int LOAD_STATE_LOADED = 3;
    public static final int LOAD_STATE_FAILED = 4;

    public static final boolean DEBUG = false;
    private static final String TAG = ExtendedBitmapDrawable.class.getSimpleName();

    private final Resources mResources;
    private final ExtendedOptions mOpts;

    // Parallax.
    private float mParallaxFraction = 1f / 2;

    // State changes.
    private int mLoadState = LOAD_STATE_UNINITIALIZED;
    private Placeholder mPlaceholder;
    private Progress mProgress;
    private int mProgressDelayMs;
    private final Handler mHandler = new Handler();

    public ExtendedBitmapDrawable(final Resources res, final BitmapCache cache,
            final boolean limitDensity, ExtendedOptions opts) {
        super(res, cache, limitDensity);
        mResources = res;
        if (opts == null) {
            opts = new ExtendedOptions(0);
        }
        mOpts = opts;

        onOptsChanged();
    }

    /**
     * Called after a field is changed in an {@link ExtendedOptions}, if that field requests this
     * method to be called.
     */
    public void onOptsChanged() {
        mOpts.validate();

        // Placeholder and progress.
        if ((mOpts.features & ExtendedOptions.FEATURE_STATE_CHANGES) != 0) {
            final int fadeOutDurationMs = mResources.getInteger(R.integer.bitmap_fade_animation_duration);
            mProgressDelayMs = mResources.getInteger(R.integer.bitmap_progress_animation_delay);

            // Placeholder is not optional because backgroundColor is part of it.
            Drawable placeholder = null;
            int placeholderWidth = mResources.getDimensionPixelSize(R.dimen.placeholder_size);
            int placeholderHeight = mResources.getDimensionPixelSize(R.dimen.placeholder_size);
            if (mOpts.placeholder != null) {
                ConstantState constantState = mOpts.placeholder.getConstantState();
                if (constantState != null) {
                    placeholder = constantState.newDrawable(mResources);
                } else {
                    placeholder = mOpts.placeholder;
                }

                Rect bounds = mOpts.placeholder.getBounds();
                if (bounds.width() != 0) {
                    placeholderWidth = bounds.width();
                } else if (placeholder.getIntrinsicWidth() != -1) {
                    placeholderWidth = placeholder.getIntrinsicWidth();
                }
                if (bounds.height() != 0) {
                    placeholderHeight = bounds.height();
                } else if (placeholder.getIntrinsicHeight() != -1) {
                    placeholderHeight = placeholder.getIntrinsicHeight();
                }
            }

            mPlaceholder = new Placeholder(placeholder, mResources, placeholderWidth, placeholderHeight,
                    fadeOutDurationMs, mOpts);
            mPlaceholder.setCallback(this);
            mPlaceholder.setBounds(getBounds());

            // Progress bar is optional.
            if (mOpts.progressBar != null) {
                int progressBarSize = mResources.getDimensionPixelSize(R.dimen.progress_bar_size);
                mProgress = new Progress(mOpts.progressBar.getConstantState().newDrawable(mResources), mResources,
                        progressBarSize, progressBarSize, fadeOutDurationMs, mOpts);
                mProgress.setCallback(this);
                mProgress.setBounds(getBounds());
            } else {
                mProgress = null;
            }
        }

        setLoadState(mLoadState);
    }

    @Override
    public void setParallaxFraction(float fraction) {
        mParallaxFraction = fraction;
        invalidateSelf();
    }

    /**
     * Get the ExtendedOptions used to instantiate this ExtendedBitmapDrawable. Any changes made to
     * the parameters inside the options will take effect immediately.
     */
    public ExtendedOptions getExtendedOptions() {
        return mOpts;
    }

    /**
     * This sets the drawable to the failed state, which remove all animations from the placeholder.
     * This is different from unbinding to the uninitialized state, where we expect animations.
     */
    public void showStaticPlaceholder() {
        setLoadState(LOAD_STATE_FAILED);
    }

    /**
     * Directly sets the decode width and height. The given height should already have had the
     * parallaxSpeedMultiplier applied to it.
     */
    public void setExactDecodeDimensions(int width, int height) {
        super.setDecodeDimensions(width, height);
    }

    /**
     * {@inheritDoc}
     *
     * The given height should not have had the parallaxSpeedMultiplier applied to it.
     */
    @Override
    public void setDecodeDimensions(int width, int height) {
        super.setDecodeDimensions(width, (int) (height * mOpts.parallaxSpeedMultiplier));
    }

    @Override
    protected void setImage(final RequestKey key) {
        if (mCurrKey != null && getDecodeAggregator() != null) {
            getDecodeAggregator().forget(mCurrKey);
        }

        mHandler.removeCallbacks(this);
        // start from a clean slate on every bind
        // this allows the initial transition to be specially instantaneous, so e.g. a cache hit
        // doesn't unnecessarily trigger a fade-in
        setLoadState(LOAD_STATE_UNINITIALIZED);

        super.setImage(key);

        if (key == null) {
            showStaticPlaceholder();
        }
    }

    @Override
    protected void setBitmap(ReusableBitmap bmp) {
        if (bmp != null) {
            setLoadState(LOAD_STATE_LOADED);
        } else {
            onDecodeFailed();
        }

        super.setBitmap(bmp);
    }

    @Override
    protected void loadFileDescriptorFactory() {
        boolean executeStateChange = shouldExecuteStateChange();
        if (mCurrKey == null || mDecodeWidth == 0 || mDecodeHeight == 0) {
          return;
        }

        if (executeStateChange) {
            setLoadState(LOAD_STATE_NOT_YET_LOADED);
        }

        super.loadFileDescriptorFactory();
    }

    @Override
    protected void onDecodeFailed() {
        super.onDecodeFailed();

        setLoadState(LOAD_STATE_FAILED);
    }

    protected boolean shouldExecuteStateChange() {
        // TODO: AttachmentDrawable should override this method to match prev and curr request keys.
        return /* opts.stateChanges */ true;
    }

    @Override
    public float getDrawVerticalCenter() {
        return mParallaxFraction;
    }

    @Override
    protected final float getDrawVerticalOffsetMultiplier() {
        return mOpts.parallaxSpeedMultiplier;
    }

    @Override
    protected float getDecodeHorizontalCenter() {
        return mOpts.decodeHorizontalCenter;
    }

    @Override
    protected float getDecodeVerticalCenter() {
        return mOpts.decodeVerticalCenter;
    }

    private DecodeAggregator getDecodeAggregator() {
        return mOpts.decodeAggregator;
    }

    /**
     * Instead of overriding this method, subclasses should override {@link #onDraw(Canvas)}.
     *
     * The reason for this is that we need the placeholder and progress bar to be drawn over our
     * content. Those two drawables fade out, giving the impression that our content is fading in.
     *
     * Only override this method for custom drawings on top of all the drawable layers.
     */
    @Override
    public void draw(final Canvas canvas) {
        final Rect bounds = getBounds();
        if (bounds.isEmpty()) {
            return;
        }

        onDraw(canvas);

        // Draw the two possible overlay layers in reverse-priority order.
        // (each layer will no-op the draw when appropriate)
        // This ordering means cross-fade transitions are just fade-outs of each layer.
        if (mProgress != null) onDrawPlaceholderOrProgress(canvas, mProgress);
        if (mPlaceholder != null) onDrawPlaceholderOrProgress(canvas, mPlaceholder);
    }

    /**
     * Overriding this method to add your own custom drawing.
     */
    protected void onDraw(final Canvas canvas) {
        super.draw(canvas);
    }

    /**
     * Overriding this method to add your own custom placeholder or progress drawing.
     */
    protected void onDrawPlaceholderOrProgress(final Canvas canvas, final TileDrawable drawable) {
        drawable.draw(canvas);
    }

    @Override
    public void setAlpha(int alpha) {
        final int old = mPaint.getAlpha();
        super.setAlpha(alpha);
        if (mPlaceholder != null) mPlaceholder.setAlpha(alpha);
        if (mProgress != null) mProgress.setAlpha(alpha);
        if (alpha != old) {
            invalidateSelf();
        }
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        super.setColorFilter(cf);
        if (mPlaceholder != null) mPlaceholder.setColorFilter(cf);
        if (mProgress != null) mProgress.setColorFilter(cf);
        invalidateSelf();
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        super.onBoundsChange(bounds);
        if (mPlaceholder != null) mPlaceholder.setBounds(bounds);
        if (mProgress != null) mProgress.setBounds(bounds);
    }

    @Override
    public void onDecodeBegin(final RequestKey key) {
        if (getDecodeAggregator() != null) {
            getDecodeAggregator().expect(key, this);
        } else {
            onBecomeFirstExpected(key);
        }
        super.onDecodeBegin(key);
    }

    @Override
    public void onBecomeFirstExpected(final RequestKey key) {
        if (!key.equals(mCurrKey)) {
            return;
        }
        // normally, we'd transition to the LOADING state now, but we want to delay that a bit
        // to minimize excess occurrences of the rotating spinner
        mHandler.postDelayed(this, mProgressDelayMs);
    }

    @Override
    public void run() {
        if (mLoadState == LOAD_STATE_NOT_YET_LOADED) {
            setLoadState(LOAD_STATE_LOADING);
        }
    }

    @Override
    public void onDecodeComplete(final RequestKey key, final ReusableBitmap result) {
        if (getDecodeAggregator() != null) {
            getDecodeAggregator().execute(key, new Runnable() {
                @Override
                public void run() {
                    ExtendedBitmapDrawable.super.onDecodeComplete(key, result);
                }

                @Override
                public String toString() {
                    return "DONE";
                }
            });
        } else {
            super.onDecodeComplete(key, result);
        }
    }

    @Override
    public void onDecodeCancel(final RequestKey key) {
        if (getDecodeAggregator() != null) {
            getDecodeAggregator().forget(key);
        }
        super.onDecodeCancel(key);
    }

    /**
     * Get the load state of this drawable. Return one of the LOAD_STATE constants.
     */
    public int getLoadState() {
        return mLoadState;
    }

    /**
     * Each attachment gets its own placeholder and progress indicator, to be shown, hidden,
     * and animated based on Drawable#setVisible() changes, which are in turn driven by
     * setLoadState().
     */
    private void setLoadState(int loadState) {
        if (DEBUG) {
            Log.v(TAG, String.format("IN setLoadState. old=%s new=%s key=%s this=%s",
                    mLoadState, loadState, mCurrKey, this));
        }

        Trace.beginSection("set load state");
        switch (loadState) {
            // This state differs from LOADED in that the subsequent state transition away from
            // UNINITIALIZED will not have a fancy transition. This allows list item binds to
            // cached data to take immediate effect without unnecessary whizzery.
            case LOAD_STATE_UNINITIALIZED:
                if (mPlaceholder != null) mPlaceholder.reset();
                if (mProgress != null) mProgress.reset();
                break;
            case LOAD_STATE_NOT_YET_LOADED:
                if (mPlaceholder != null) {
                    mPlaceholder.setPulseEnabled(true);
                    mPlaceholder.setVisible(true);
                }
                if (mProgress != null) mProgress.setVisible(false);
                break;
            case LOAD_STATE_LOADING:
                if (mProgress == null) {
                    // Stay in same visual state as LOAD_STATE_NOT_YET_LOADED.
                    break;
                }
                if (mPlaceholder != null) mPlaceholder.setVisible(false);
                if (mProgress != null) mProgress.setVisible(true);
                break;
            case LOAD_STATE_LOADED:
                if (mPlaceholder != null) mPlaceholder.setVisible(false);
                if (mProgress != null) mProgress.setVisible(false);
                break;
            case LOAD_STATE_FAILED:
                if (mPlaceholder != null) {
                    mPlaceholder.setPulseEnabled(false);
                    mPlaceholder.setVisible(true);
                }
                if (mProgress != null) mProgress.setVisible(false);
                break;
        }
        Trace.endSection();

        mLoadState = loadState;
        boolean placeholderVisible = mPlaceholder != null && mPlaceholder.isVisible();
        boolean progressVisible = mProgress != null && mProgress.isVisible();

        if (DEBUG) {
            Log.v(TAG, String.format("OUT stateful setLoadState. new=%s placeholder=%s progress=%s",
                    loadState, placeholderVisible, progressVisible));
        }
    }

    private static class Placeholder extends TileDrawable {

        private final ValueAnimator mPulseAnimator;
        private boolean mPulseEnabled = true;
        private float mPulseAlphaFraction = 1f;

        public Placeholder(Drawable placeholder, Resources res, int placeholderWidth,
                int placeholderHeight, int fadeOutDurationMs, ExtendedOptions opts) {
            super(placeholder, placeholderWidth, placeholderHeight, fadeOutDurationMs, opts);

            if (opts.placeholderAnimationDuration == -1) {
                mPulseAnimator = null;
            } else {
                final long pulseDuration;
                if (opts.placeholderAnimationDuration == 0) {
                    pulseDuration = res.getInteger(R.integer.bitmap_placeholder_animation_duration);
                } else {
                    pulseDuration = opts.placeholderAnimationDuration;
                }
                mPulseAnimator = ValueAnimator.ofInt(55, 255).setDuration(pulseDuration);
                mPulseAnimator.setRepeatCount(ValueAnimator.INFINITE);
                mPulseAnimator.setRepeatMode(ValueAnimator.REVERSE);
                mPulseAnimator.addUpdateListener(new AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        mPulseAlphaFraction = ((Integer) animation.getAnimatedValue()) / 255f;
                        setInnerAlpha(getCurrentAlpha());
                    }
                });
            }
            mFadeOutAnimator.addListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    stopPulsing();
                }
            });
        }

        @Override
        public void setInnerAlpha(final int alpha) {
            super.setInnerAlpha((int) (alpha * mPulseAlphaFraction));
        }

        public void setPulseEnabled(boolean enabled) {
            mPulseEnabled = enabled;
            if (!mPulseEnabled) {
                stopPulsing();
            } else {
                startPulsing();
            }
        }

        private void stopPulsing() {
            if (mPulseAnimator != null) {
                mPulseAnimator.cancel();
                mPulseAlphaFraction = 1f;
                setInnerAlpha(getCurrentAlpha());
            }
        }

        private void startPulsing() {
            if (mPulseAnimator != null && !mPulseAnimator.isStarted()) {
                mPulseAnimator.start();
            }
        }

        @Override
        public boolean setVisible(boolean visible) {
            final boolean changed = super.setVisible(visible);
            if (changed) {
                if (isVisible()) {
                    // start
                    if (mPulseAnimator != null && mPulseEnabled && !mPulseAnimator.isStarted()) {
                        mPulseAnimator.start();
                    }
                } else {
                    // can't cancel the pulsing yet-- wait for the fade-out animation to end
                    // one exception: if alpha is already zero, there is no fade-out, so stop now
                    if (getCurrentAlpha() == 0) {
                        stopPulsing();
                    }
                }
            }
            return changed;
        }

    }

    private static class Progress extends TileDrawable {

        private final ValueAnimator mRotateAnimator;

        public Progress(Drawable progress, Resources res,
                int progressBarWidth, int progressBarHeight, int fadeOutDurationMs,
                ExtendedOptions opts) {
            super(progress, progressBarWidth, progressBarHeight, fadeOutDurationMs, opts);

            mRotateAnimator = ValueAnimator.ofInt(0, 10000)
                    .setDuration(res.getInteger(R.integer.bitmap_progress_animation_duration));
            mRotateAnimator.setInterpolator(new LinearInterpolator());
            mRotateAnimator.setRepeatCount(ValueAnimator.INFINITE);
            mRotateAnimator.addUpdateListener(new AnimatorUpdateListener() {
                @Override
                public void onAnimationUpdate(ValueAnimator animation) {
                    setLevel((Integer) animation.getAnimatedValue());
                }
            });
            mFadeOutAnimator.addListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    if (mRotateAnimator != null) {
                        mRotateAnimator.cancel();
                    }
                }
            });
        }

        @Override
        public boolean setVisible(boolean visible) {
            final boolean changed = super.setVisible(visible);
            if (changed) {
                if (isVisible()) {
                    if (mRotateAnimator != null) {
                        mRotateAnimator.start();
                    }
                } else {
                    // can't cancel the rotate yet-- wait for the fade-out animation to end
                    // one exception: if alpha is already zero, there is no fade-out, so stop now
                    if (getCurrentAlpha() == 0 && mRotateAnimator != null) {
                        mRotateAnimator.cancel();
                    }
                }
            }
            return changed;
        }
    }

    /**
     * This class contains the features a client can specify, and arguments to those features.
     * Clients can later retrieve the ExtendedOptions from an ExtendedBitmapDrawable and change the
     * parameters, which will be reflected immediately.
     */
    public static class ExtendedOptions {

        /**
         * Summary:
         * This feature enables you to draw decoded bitmap in order on the screen, to give the
         * visual effect of a single decode thread.
         *
         * <p/>
         * Explanation:
         * Since DecodeTasks are asynchronous, multiple tasks may finish decoding at different
         * times. To have a smooth user experience, provide a shared {@link DecodeAggregator} to all
         * the ExtendedBitmapDrawables, and the decode aggregator will hold finished decodes so they
         * come back in order.
         *
         * <p/>
         * Pros:
         * Visual consistency. Images are not popping up randomly all over the place.
         *
         * <p/>
         * Cons:
         * Artificial delay. Images are not drawn as soon as they are decoded. They must wait
         * for their turn.
         *
         * <p/>
         * Requirements:
         * Set {@link #decodeAggregator} to a shared {@link DecodeAggregator}.
         */
        public static final int FEATURE_ORDERED_DISPLAY = 1;

        /**
         * Summary:
         * This feature enables the image to move in parallax as the user scrolls, to give visual
         * flair to your images.
         *
         * <p/>
         * Explanation:
         * When the user scrolls D pixels in the vertical direction, this ExtendedBitmapDrawable
         * shifts its Bitmap f(D) pixels in the vertical direction before drawing to the screen.
         * Depending on the function f, the parallax effect can give varying interesting results.
         *
         * <p/>
         * Pros:
         * Visual pop and playfulness. Feeling of movement. Pleasantly surprise your users.
         *
         * <p/>
         * Cons:
         * Some users report motion sickness with certain speed multiplier values. Decode height
         * must be greater than visual bounds to account for the parallax. This uses more memory and
         * decoding time.
         *
         * <p/>
         * Requirements:
         * Set {@link #parallaxSpeedMultiplier} to the ratio between the decoded height and the
         * visual bound height. Call {@link ExtendedBitmapDrawable#setDecodeDimensions(int, int)}
         * with the height multiplied by {@link #parallaxSpeedMultiplier}.
         * Call {@link ExtendedBitmapDrawable#setParallaxFraction(float)} when the user scrolls.
         */
        public static final int FEATURE_PARALLAX = 1 << 1;

        /**
         * Summary:
         * This feature enables fading in between multiple decode states, to give smooth transitions
         * to and from the placeholder, progress bars, and decoded image.
         *
         * <p/>
         * Explanation:
         * The states are: {@link ExtendedBitmapDrawable#LOAD_STATE_UNINITIALIZED},
         * {@link ExtendedBitmapDrawable#LOAD_STATE_NOT_YET_LOADED},
         * {@link ExtendedBitmapDrawable#LOAD_STATE_LOADING},
         * {@link ExtendedBitmapDrawable#LOAD_STATE_LOADED}, and
         * {@link ExtendedBitmapDrawable#LOAD_STATE_FAILED}. These states affect whether the
         * placeholder and/or the progress bar is showing and animating. We first show the
         * pulsating placeholder when an image begins decoding. After 2 seconds, we fade in a
         * spinning progress bar. When the decode completes, we fade in the image.
         *
         * <p/>
         * Pros:
         * Smooth, beautiful transitions avoid perceived jank. Progress indicator informs users that
         * work is being done and the app is not stalled.
         *
         * <p/>
         * Cons:
         * Very fast decodes' short decode time would be eclipsed by the animation duration. Static
         * placeholder could be accomplished by {@link BasicBitmapDrawable} without the added
         * complexity of states.
         *
         * <p/>
         * Requirements:
         * Set {@link #backgroundColor} to the color used for the background of the placeholder and
         * progress bar. Use the alternative constructor to populate {@link #placeholder} and
         * {@link #progressBar}. Optionally set {@link #placeholderAnimationDuration}.
         */
        public static final int FEATURE_STATE_CHANGES = 1 << 2;

        /**
         * Non-changeable bit field describing the features you want the
         * {@link ExtendedBitmapDrawable} to support.
         *
         * <p/>
         * Example:
         * <code>
         * opts.features = FEATURE_ORDERED_DISPLAY | FEATURE_PARALLAX | FEATURE_STATE_CHANGES;
         * </code>
         */
        public final int features;

        /**
         * Optional field for general decoding.
         *
         * This field determines which section of the source image to decode from. A value of 0
         * indicates a preference for the far left of the source, while a value of 1 indicates a
         * preference for the far right of the source. A value of .5 will result in the center
         * of the source being decoded.
         */
        public float decodeHorizontalCenter = 1f / 2;

        /**
         * Optional field for general decoding.
         *
         * This field determines which section of the source image to decode from. A value of 0
         * indicates a preference for the very top of the source, while a value of 1 indicates a
         * preference for the very bottom of the source. A value of .5 will result in the center
         * of the source being decoded.
         *
         * This should not be confused with {@link #setParallaxFraction(float)}. This field
         * determines the general section for decode. The parallax fraction then determines the
         * slice from within that section for display.
         */
        public float decodeVerticalCenter = 1f / 2;

        /**
         * Required field if {@link #FEATURE_ORDERED_DISPLAY} is supported.
         */
        public DecodeAggregator decodeAggregator = null;

        /**
         * Required field if {@link #FEATURE_PARALLAX} is supported.
         *
         * A value of 1.5f gives a subtle parallax, and is a good value to
         * start with. 2.0f gives a more obvious parallax, arguably exaggerated. Some users report
         * motion sickness with 2.0f. A value of 1.0f is synonymous with no parallax. Be careful not
         * to set too high a value, since we will start cropping the widths if the image's height is
         * not sufficient.
         */
        public float parallaxSpeedMultiplier = 1;

        /**
         * Optional field if {@link #FEATURE_STATE_CHANGES} is supported. Must be an opaque color.
         *
         * See {@link android.graphics.Color}.
         */
        public int backgroundColor = 0;

        /**
         * Optional field if {@link #FEATURE_STATE_CHANGES} is supported.
         *
         * If you modify this field you must call
         * {@link ExtendedBitmapDrawable#onOptsChanged(Resources, ExtendedOptions)} on the
         * appropriate ExtendedBitmapDrawable.
         */
        public Drawable placeholder;

        /**
         * Optional field if {@link #FEATURE_STATE_CHANGES} is supported.
         *
         * Special value 0 means default animation duration. Special value -1 means disable the
         * animation (placeholder will be at maximum alpha always). Any value > 0 defines the
         * duration in milliseconds.
         */
        public int placeholderAnimationDuration = 0;

        /**
         * Optional field if {@link #FEATURE_STATE_CHANGES} is supported.
         *
         * If you modify this field you must call
         * {@link ExtendedBitmapDrawable#onOptsChanged(Resources, ExtendedOptions)} on the
         * appropriate ExtendedBitmapDrawable.
         */
        public Drawable progressBar;

        /**
         * Use this constructor when all the feature parameters are changeable.
         */
        public ExtendedOptions(final int features) {
            this(features, null, null);
        }

        /**
         * Use this constructor when you have to specify non-changeable feature parameters.
         */
        public ExtendedOptions(final int features, final Drawable placeholder,
                final Drawable progressBar) {
            this.features = features;
            this.placeholder = placeholder;
            this.progressBar = progressBar;
        }

        /**
         * Validate this ExtendedOptions instance to make sure that all the required fields are set
         * for the requested features.
         *
         * This will throw an IllegalStateException if validation fails.
         */
        private void validate()
                throws IllegalStateException {
            if (decodeHorizontalCenter < 0 || decodeHorizontalCenter > 1) {
                throw new IllegalStateException(
                        "ExtendedOptions: decodeHorizontalCenter must be within 0 and 1, " +
                                "inclusive");
            }
            if (decodeVerticalCenter < 0 || decodeVerticalCenter > 1) {
                throw new IllegalStateException(
                        "ExtendedOptions: decodeVerticalCenter must be within 0 and 1, inclusive");
            }
            if ((features & FEATURE_ORDERED_DISPLAY) != 0 && decodeAggregator == null) {
                throw new IllegalStateException(
                        "ExtendedOptions: To support FEATURE_ORDERED_DISPLAY, "
                                + "decodeAggregator must be set.");
            }
            if ((features & FEATURE_PARALLAX) != 0 && parallaxSpeedMultiplier <= 1) {
                throw new IllegalStateException(
                        "ExtendedOptions: To support FEATURE_PARALLAX, "
                                + "parallaxSpeedMultiplier must be greater than 1.");
            }
            if ((features & FEATURE_STATE_CHANGES) != 0) {
                if (backgroundColor == 0
                        && placeholder == null) {
                    throw new IllegalStateException(
                            "ExtendedOptions: To support FEATURE_STATE_CHANGES, "
                                    + "either backgroundColor or placeholder must be set.");
                }
                if (placeholderAnimationDuration < -1) {
                    throw new IllegalStateException(
                            "ExtendedOptions: To support FEATURE_STATE_CHANGES, "
                                    + "placeholderAnimationDuration must be set correctly.");
                }
                if (backgroundColor != 0 && Color.alpha(backgroundColor) != 255) {
                    throw new IllegalStateException(
                            "ExtendedOptions: To support FEATURE_STATE_CHANGES, "
                                    + "backgroundColor must be set to an opaque color.");
                }
            }
        }
    }
}
