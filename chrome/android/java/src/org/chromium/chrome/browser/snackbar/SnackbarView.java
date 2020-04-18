// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.snackbar;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.support.annotation.Nullable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLayoutChangeListener;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

/**
 * Visual representation of a snackbar. On phone it matches the width of the activity; on tablet it
 * has a fixed width and is anchored at the start-bottom corner of the current window.
 */
class SnackbarView {
    private static final int MAX_LINES = 5;

    private final Activity mActivity;
    private final ViewGroup mContainerView;
    private final ViewGroup mSnackbarView;
    private final TemplatePreservingTextView mMessageView;
    private final TextView mActionButtonView;
    private final ImageView mProfileImageView;
    private final int mAnimationDuration;
    private final boolean mIsTablet;
    private ViewGroup mOriginalParent;
    private ViewGroup mParent;
    private Snackbar mSnackbar;
    private boolean mAnimateOverWebContent;
    private View mRootContentView;

    // Variables used to calculate the virtual keyboard's height.
    private Rect mCurrentVisibleRect = new Rect();
    private Rect mPreviousVisibleRect = new Rect();
    private int[] mTempLocation = new int[2];

    private OnLayoutChangeListener mLayoutListener = new OnLayoutChangeListener() {
        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                int oldTop, int oldRight, int oldBottom) {
            adjustViewPosition();
        }
    };

    /**
     * Creates an instance of the {@link SnackbarView}.
     * @param activity The activity that displays the snackbar.
     * @param listener An {@link OnClickListener} that will be called when the action button is
     *                 clicked.
     * @param snackbar The snackbar to be displayed.
     * @param parentView The ViewGroup used to display this snackbar. If this is null, this class
     *                   will determine where to attach the snackbar.
     */
    SnackbarView(Activity activity, OnClickListener listener, Snackbar snackbar,
            @Nullable ViewGroup parentView) {
        mActivity = activity;
        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity);

        if (parentView == null) {
            mOriginalParent = findParentView(activity);
            if (activity instanceof ChromeActivity) mAnimateOverWebContent = true;
        } else {
            mOriginalParent = parentView;
        }

        mRootContentView = activity.findViewById(android.R.id.content);
        mParent = mOriginalParent;
        mContainerView = (ViewGroup) LayoutInflater.from(activity).inflate(
                R.layout.snackbar, mParent, false);
        mSnackbarView = mContainerView.findViewById(R.id.snackbar);
        mAnimationDuration =
                mContainerView.getResources().getInteger(android.R.integer.config_mediumAnimTime);
        mMessageView =
                (TemplatePreservingTextView) mContainerView.findViewById(R.id.snackbar_message);
        mActionButtonView = (TextView) mContainerView.findViewById(R.id.snackbar_button);
        mActionButtonView.setOnClickListener(listener);
        mProfileImageView = (ImageView) mContainerView.findViewById(R.id.snackbar_profile_image);

        updateInternal(snackbar, false);
    }

    void show() {
        addToParent();
        mContainerView.addOnLayoutChangeListener(new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                mContainerView.removeOnLayoutChangeListener(this);
                mContainerView.setTranslationY(
                        mContainerView.getHeight() + getLayoutParams().bottomMargin);
                Animator animator = ObjectAnimator.ofFloat(mContainerView, View.TRANSLATION_Y, 0);
                animator.setInterpolator(new DecelerateInterpolator());
                animator.setDuration(mAnimationDuration);
                startAnimatorOnSurfaceView(animator);
            }
        });
    }

    void dismiss() {
        // Disable action button during animation.
        mActionButtonView.setEnabled(false);
        AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.setDuration(mAnimationDuration);
        animatorSet.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mRootContentView.removeOnLayoutChangeListener(mLayoutListener);
                mParent.removeView(mContainerView);
            }
        });
        Animator moveDown = ObjectAnimator.ofFloat(mContainerView, View.TRANSLATION_Y,
                mContainerView.getHeight() + getLayoutParams().bottomMargin);
        moveDown.setInterpolator(new DecelerateInterpolator());
        Animator fadeOut = ObjectAnimator.ofFloat(mContainerView, View.ALPHA, 0f);
        fadeOut.setInterpolator(BakedBezierInterpolator.FADE_OUT_CURVE);

        animatorSet.playTogether(fadeOut, moveDown);
        startAnimatorOnSurfaceView(animatorSet);
    }

    /**
     * Adjusts the position of the snackbar on top of the soft keyboard, if any.
     */
    void adjustViewPosition() {
        mParent.getWindowVisibleDisplayFrame(mCurrentVisibleRect);
        // Only update if the visible frame has changed, otherwise there will be a layout loop.
        if (!mCurrentVisibleRect.equals(mPreviousVisibleRect)) {
            mPreviousVisibleRect.set(mCurrentVisibleRect);

            mParent.getLocationInWindow(mTempLocation);
            int keyboardHeight =
                    mParent.getHeight() + mTempLocation[1] - mCurrentVisibleRect.bottom;
            keyboardHeight = Math.max(0, keyboardHeight);
            FrameLayout.LayoutParams lp = getLayoutParams();

            int prevBottomMargin = lp.bottomMargin;
            int prevWidth = lp.width;
            int prevGravity = lp.gravity;

            lp.bottomMargin = keyboardHeight;
            if (mIsTablet) {
                int margin = mParent.getResources()
                        .getDimensionPixelSize(R.dimen.snackbar_margin_tablet);
                int width = mParent.getResources()
                        .getDimensionPixelSize(R.dimen.snackbar_width_tablet);
                lp.width = Math.min(width, mParent.getWidth() - 2 * margin);
                lp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM;
            }

            if (prevBottomMargin != lp.bottomMargin || prevWidth != lp.width
                    || prevGravity != lp.gravity) {
                mContainerView.setLayoutParams(lp);
            }
        }
    }

    /**
     * @see SnackbarManager#overrideParent(ViewGroup)
     */
    void overrideParent(ViewGroup overridingParent) {
        mRootContentView.removeOnLayoutChangeListener(mLayoutListener);
        mParent = overridingParent == null ? mOriginalParent : overridingParent;
        if (isShowing()) {
            ((ViewGroup) mContainerView.getParent()).removeView(mContainerView);
        }
        addToParent();
    }

    boolean isShowing() {
        return mContainerView.isShown();
    }

    void bringToFront() {
        mContainerView.bringToFront();
    }

    /**
     * Sends an accessibility event to mMessageView announcing that this window was added so that
     * the mMessageView content description is read aloud if accessibility is enabled.
     */
    void announceforAccessibility() {
        mMessageView.announceForAccessibility(mMessageView.getContentDescription() + " "
                + mContainerView.getResources().getString(R.string.bottom_bar_screen_position));
    }

    /**
     * Updates the view to display data from the given snackbar. No-op if the view is already
     * showing the given snackbar.
     * @param snackbar The snackbar to display
     * @return Whether update has actually been executed.
     */
    boolean update(Snackbar snackbar) {
        return updateInternal(snackbar, true);
    }

    private void addToParent() {
        mParent.addView(mContainerView);

        // Why setting listener on parent? It turns out that if we force a relayout in the layout
        // change listener of the view itself, the force layout flag will be reset to 0 when
        // layout() returns. Therefore we have to do request layout on one level above the requested
        // view.
        mRootContentView.addOnLayoutChangeListener(mLayoutListener);
    }

    private boolean updateInternal(Snackbar snackbar, boolean animate) {
        if (mSnackbar == snackbar) return false;
        mSnackbar = snackbar;
        mMessageView.setMaxLines(snackbar.getSingleLine() ? 1 : MAX_LINES);
        mMessageView.setTemplate(snackbar.getTemplateText());
        setViewText(mMessageView, snackbar.getText(), animate);
        String actionText = snackbar.getActionText();

        int backgroundColor = snackbar.getBackgroundColor();
        if (backgroundColor == 0) {
            backgroundColor = ApiCompatibilityUtils.getColor(mContainerView.getResources(),
                    FeatureUtilities.isChromeModernDesignEnabled()
                            ? R.color.modern_primary_color
                            : R.color.snackbar_background_color);
        }

        int textAppearanceResId = snackbar.getTextAppearance();
        if (textAppearanceResId == 0) {
            textAppearanceResId = FeatureUtilities.isChromeModernDesignEnabled()
                    ? R.style.BlackBodyDefault
                    : R.style.WhiteBody;
        }
        ApiCompatibilityUtils.setTextAppearance(mMessageView, textAppearanceResId);

        if (mIsTablet) {
            // On tablet, snackbars have rounded corners.
            mSnackbarView.setBackgroundResource(R.drawable.snackbar_background_tablet);
            GradientDrawable backgroundDrawable =
                    (GradientDrawable) mSnackbarView.getBackground().mutate();
            backgroundDrawable.setColor(backgroundColor);
        } else {
            mSnackbarView.setBackgroundColor(backgroundColor);
        }

        if (actionText != null) {
            mActionButtonView.setVisibility(View.VISIBLE);
            setViewText(mActionButtonView, snackbar.getActionText(), animate);
        } else {
            mActionButtonView.setVisibility(View.GONE);
        }
        Drawable profileImage = snackbar.getProfileImage();
        if (profileImage != null) {
            mProfileImageView.setVisibility(View.VISIBLE);
            mProfileImageView.setImageDrawable(profileImage);
        } else {
            mProfileImageView.setVisibility(View.GONE);
        }

        if (FeatureUtilities.isChromeModernDesignEnabled()) {
            mActionButtonView.setTextColor(ApiCompatibilityUtils.getColor(
                    mContainerView.getResources(), R.color.blue_when_enabled));

            mContainerView.findViewById(R.id.snackbar_shadow_top).setVisibility(View.VISIBLE);
            if (mIsTablet) {
                mContainerView.findViewById(R.id.snackbar_shadow_left).setVisibility(View.VISIBLE);
                mContainerView.findViewById(R.id.snackbar_shadow_right).setVisibility(View.VISIBLE);
            }
        }
        return true;
    }

    /**
     * @return The parent {@link ViewGroup} that {@link #mContainerView} will be added to.
     */
    private ViewGroup findParentView(Activity activity) {
        if (activity instanceof ChromeActivity) {
            return (ViewGroup) activity.findViewById(R.id.bottom_container);
        } else {
            return (ViewGroup) activity.findViewById(android.R.id.content);
        }
    }

    /**
     * Starts the {@link Animator} with {@link SurfaceView} optimization disabled. If a
     * {@link SurfaceView} is not present in the given {@link Activity}, start the {@link Animator}
     * in the normal way.
     */
    private void startAnimatorOnSurfaceView(Animator animator) {
        if (mAnimateOverWebContent) {
            ((ChromeActivity) mActivity).getWindowAndroid().startAnimationOverContent(animator);
        } else {
            animator.start();
        }
    }

    private FrameLayout.LayoutParams getLayoutParams() {
        return (FrameLayout.LayoutParams) mContainerView.getLayoutParams();
    }

    private void setViewText(TextView view, CharSequence text, boolean animate) {
        if (view.getText().toString().equals(text)) return;
        view.animate().cancel();
        if (animate) {
            view.setAlpha(0.0f);
            view.setText(text);
            view.animate().alpha(1.f).setDuration(mAnimationDuration).setListener(null);
        } else {
            view.setText(text);
        }
    }
}
