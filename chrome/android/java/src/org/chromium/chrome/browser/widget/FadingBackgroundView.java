// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.base.ObserverList;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

/**
 * This view is used to obscure content and bring focus to a foreground view (i.e. the Chrome Home
 * bottom sheet or the omnibox suggestions).
 *
 * If the view is disabled, then its alpha will be set to 0f and it will not receive touch events.
 */
public class FadingBackgroundView extends View implements View.OnClickListener {
    /**
     * An interface for listening to events on the fading view.
     */
    public interface FadingViewObserver {
        /**
         * An event that triggers when the view is clicked.
         */
        void onFadingViewClick();

        /**
         * An event that triggers when the visibility of the overlay has changed. Visibility is true
         * if the overlay's opacity is > 0f.
         * @param visible True if the overlay has become visible.
         */
        void onFadingViewVisibilityChanged(boolean visible);
    }

    /** The duration for the fading animation. */
    private static final int FADE_DURATION_MS = 250;

    /** List of observers for this view. */
    private final ObserverList<FadingViewObserver> mObservers = new ObserverList<>();

    /** The animator for fading the view out. */
    private ObjectAnimator mOverlayFadeInAnimator;

    /** The animator for fading the view in. */
    private ObjectAnimator mOverlayFadeOutAnimator;

    /** The active animator (if any). */
    private Animator mOverlayAnimator;

    public FadingBackgroundView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setAlpha(0.0f);
        setVisibility(View.GONE);
        setOnClickListener(this);
    }

    /**
     * Set the alpha for the fading view. This specifically does not override
     * {@link #setAlpha(float)} so animations can be canceled if this is called.
     * @param alpha The desired alpha for this view.
     */
    public void setViewAlpha(float alpha) {
        if (!isEnabled() || MathUtils.areFloatsEqual(alpha, getAlpha())) return;

        setAlpha(alpha);

        if (mOverlayAnimator != null) mOverlayAnimator.cancel();
    }

    @Override
    public void setEnabled(boolean isEnabled) {
        super.setEnabled(isEnabled);

        if (!isEnabled) {
            if (mOverlayAnimator != null) mOverlayAnimator.cancel();
            setAlpha(0f);
        }
    }

    /**
     * Sets the alpha for this view and alters visibility based on that value.
     * WARNING: This method should not be called externally for this view! Use setViewAlpha instead.
     * @param alpha The alpha to set the view to.
     */
    @Override
    public void setAlpha(float alpha) {
        super.setAlpha(alpha);

        int newVisibility = alpha <= 0f ? View.GONE : View.VISIBLE;
        setVisibility(newVisibility);
    }

    @Override
    public void setVisibility(int visibility) {
        if (getAlpha() <= 0f && visibility == View.VISIBLE) return;
        super.setVisibility(visibility);
    }

    @Override
    protected void dispatchVisibilityChanged(View view, int visibility) {
        if (getAlpha() <= 0f && visibility == View.VISIBLE) return;
        super.dispatchVisibilityChanged(view, visibility);
    }

    @Override
    public void onVisibilityChanged(View view, int visibility) {
        super.onVisibilityChanged(view, visibility);

        // This check is added for the exclusive purpose of testing on Android K. Later versions
        // of Android do not run into the problem of the observer list being null.
        if (mObservers != null) {
            for (FadingViewObserver o : mObservers) {
                o.onFadingViewVisibilityChanged(visibility == View.VISIBLE);
            }
        }
    }

    /**
     * Triggers a fade in of the omnibox results background creating a new animation if necessary.
     */
    public void showFadingOverlay() {
        if (mOverlayFadeInAnimator == null) {
            mOverlayFadeInAnimator = ObjectAnimator.ofFloat(this, ALPHA, 1f);
            mOverlayFadeInAnimator.setDuration(FADE_DURATION_MS);
            mOverlayFadeInAnimator.setInterpolator(
                    BakedBezierInterpolator.FADE_IN_CURVE);
        }

        runFadeOverlayAnimation(mOverlayFadeInAnimator);
    }

    /**
     * Triggers a fade out of the omnibox results background creating a new animation if necessary.
     */
    public void hideFadingOverlay(boolean fadeOut) {
        if (mOverlayFadeOutAnimator == null) {
            mOverlayFadeOutAnimator = ObjectAnimator.ofFloat(this, ALPHA, 0f);
            mOverlayFadeOutAnimator.setDuration(FADE_DURATION_MS);
            mOverlayFadeOutAnimator.setInterpolator(BakedBezierInterpolator.FADE_OUT_CURVE);
        }

        mOverlayFadeOutAnimator.setFloatValues(getAlpha(), 0f);
        runFadeOverlayAnimation(mOverlayFadeOutAnimator);
        if (!fadeOut) mOverlayFadeOutAnimator.end();
    }

    /**
     * Runs an animation for this view. If one is running, the existing one will be canceled.
     * @param fadeAnimation The animation to run.
     */
    private void runFadeOverlayAnimation(Animator fadeAnimation) {
        if (mOverlayAnimator == fadeAnimation && mOverlayAnimator.isRunning()) {
            return;
        } else if (mOverlayAnimator != null) {
            mOverlayAnimator.cancel();
        }
        mOverlayAnimator = fadeAnimation;
        mOverlayAnimator.start();
    }

    /**
     * Adds an observer to this fading view.
     * @param observer The observer to be added.
     */
    public void addObserver(FadingViewObserver observer) {
        mObservers.addObserver(observer);
    }

    @Override
    public void onClick(View view) {
        for (FadingViewObserver o : mObservers) o.onFadingViewClick();
    }
}
