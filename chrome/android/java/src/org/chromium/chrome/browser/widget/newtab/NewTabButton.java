// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.newtab;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.support.graphics.drawable.VectorDrawableCompat;
import android.util.AttributeSet;
import android.widget.Button;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.device.DeviceClassManager;
import org.chromium.chrome.browser.widget.animation.AnimatorProperties;

import java.util.ArrayList;
import java.util.List;

/**
 * Button for creating new tabs.
 */
public class NewTabButton extends Button implements Drawable.Callback {

    private final ColorStateList mLightModeTint;
    private final ColorStateList mDarkModeTint;
    private Drawable mNormalDrawable;
    private Drawable mIncognitoDrawable;
    private VectorDrawableCompat mModernDrawable;
    private boolean mIsIncognito;
    private AnimatorSet mTransitionAnimation;

    /**
     * Constructor for inflating from XML.
     */
    public NewTabButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        mNormalDrawable = ApiCompatibilityUtils.getDrawable(
                getResources(), R.drawable.btn_new_tab_white);
        mNormalDrawable.setBounds(
                0, 0, mNormalDrawable.getIntrinsicWidth(), mNormalDrawable.getIntrinsicHeight());
        mNormalDrawable.setCallback(this);
        mIncognitoDrawable = ApiCompatibilityUtils.getDrawable(
                getResources(), R.drawable.btn_new_tab_incognito);
        mIncognitoDrawable.setBounds(
                0, 0,
                mIncognitoDrawable.getIntrinsicWidth(), mIncognitoDrawable.getIntrinsicHeight());
        mIncognitoDrawable.setCallback(this);
        mIsIncognito = false;
        mLightModeTint =
                ApiCompatibilityUtils.getColorStateList(getResources(), R.color.light_mode_tint);
        mDarkModeTint =
                ApiCompatibilityUtils.getColorStateList(getResources(), R.color.dark_mode_tint);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int desiredWidth;
        if (mModernDrawable != null) {
            desiredWidth = mModernDrawable.getIntrinsicWidth();
        } else {
            desiredWidth = Math.max(
                    mIncognitoDrawable.getIntrinsicWidth(), mNormalDrawable.getIntrinsicWidth());
        }
        desiredWidth += getPaddingLeft() + getPaddingRight();
        widthMeasureSpec = MeasureSpec.makeMeasureSpec(desiredWidth, MeasureSpec.EXACTLY);
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        boolean isRtl = ApiCompatibilityUtils.isLayoutRtl(this);
        int paddingStart = ApiCompatibilityUtils.getPaddingStart(this);
        int widthWithoutPadding = getWidth() - paddingStart;

        canvas.save();
        if (!isRtl) canvas.translate(paddingStart, 0);

        if (mModernDrawable != null) {
            drawIcon(canvas, mModernDrawable, isRtl, widthWithoutPadding);
        } else {
            drawIcon(canvas, mNormalDrawable, isRtl, widthWithoutPadding);
            if (mIsIncognito
                    || (mTransitionAnimation != null && mTransitionAnimation.isRunning())) {
                drawIcon(canvas, mIncognitoDrawable, isRtl, widthWithoutPadding);
            }
        }

        canvas.restore();
    }

    private void drawIcon(Canvas canvas, Drawable drawable, boolean isRtl, int widthNoPadding) {
        canvas.save();
        canvas.translate(0, (getHeight() - drawable.getIntrinsicHeight()) / 2.f);
        if (isRtl) {
            canvas.translate(widthNoPadding - drawable.getIntrinsicWidth(), 0);
        }
        drawable.draw(canvas);
        canvas.restore();
    }

    @Override
    public void invalidateDrawable(Drawable dr) {
        if (dr == mIncognitoDrawable || dr == mNormalDrawable || dr == mModernDrawable) {
            invalidate();
        } else {
            super.invalidateDrawable(dr);
        }
    }

    /**
     * Set the icon to use the drawable for Chrome Modern.
     */
    public void setIsModern() {
        mModernDrawable = VectorDrawableCompat.create(
                getContext().getResources(), R.drawable.new_tab_icon, getContext().getTheme());
        mModernDrawable.setState(getDrawableState());
        updateDrawableTint();
        mModernDrawable.setBounds(
                0, 0, mModernDrawable.getIntrinsicWidth(), mModernDrawable.getIntrinsicHeight());
        mModernDrawable.setCallback(this);

        mNormalDrawable = null;
        mIncognitoDrawable = null;
    }

    /**
     * Updates the visual state based on whether incognito or normal tabs are being created.
     * @param incognito Whether the button is now used for creating incognito tabs.
     */
    public void setIsIncognito(boolean incognito) {
        if (mIsIncognito == incognito) return;
        mIsIncognito = incognito;

        if (mModernDrawable != null) {
            updateDrawableTint();
            invalidateDrawable(mModernDrawable);
            return;
        }

        if (mTransitionAnimation != null) {
            mTransitionAnimation.cancel();
            mTransitionAnimation = null;
        }

        Drawable fadeOutDrawable = incognito ? mNormalDrawable : mIncognitoDrawable;
        Drawable fadeInDrawable = incognito ? mIncognitoDrawable : mNormalDrawable;

        if (getVisibility() != VISIBLE) {
            fadeOutDrawable.setAlpha(0);
            fadeInDrawable.setAlpha(255);
            return;
        }

        List<Animator> animations = new ArrayList<Animator>();
        Animator animation = ObjectAnimator.ofInt(
                fadeOutDrawable, AnimatorProperties.DRAWABLE_ALPHA_PROPERTY, 255, 0);
        animation.setDuration(100);
        animations.add(animation);

        animation = ObjectAnimator.ofInt(
                fadeInDrawable, AnimatorProperties.DRAWABLE_ALPHA_PROPERTY, 0, 255);
        animation.setStartDelay(150);
        animation.setDuration(100);
        animations.add(animation);

        mTransitionAnimation = new AnimatorSet();
        mTransitionAnimation.playTogether(animations);
        mTransitionAnimation.start();
    }

    /** Called when accessibility status is changed. */
    public void onAccessibilityStatusChanged() {
        if (mModernDrawable != null) updateDrawableTint();
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();

        if (mModernDrawable != null) {
            mModernDrawable.setState(getDrawableState());
        } else {
            mNormalDrawable.setState(getDrawableState());
            mIncognitoDrawable.setState(getDrawableState());
        }
    }

    /** Update the tint for the icon drawable for Chrome Modern. */
    private void updateDrawableTint() {
        final boolean shouldUseLightMode =
                (DeviceClassManager.enableAccessibilityLayout()
                        || ChromeFeatureList.isEnabled(
                                   ChromeFeatureList.HORIZONTAL_TAB_SWITCHER_ANDROID))
                && mIsIncognito;
        mModernDrawable.setTintList(shouldUseLightMode ? mLightModeTint : mDarkModeTint);
    }
}
