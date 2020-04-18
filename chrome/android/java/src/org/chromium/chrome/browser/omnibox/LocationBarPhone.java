// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.animation.ObjectAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.support.annotation.Nullable;
import android.support.v4.view.animation.FastOutLinearInInterpolator;
import android.util.AttributeSet;
import android.view.TouchDelegate;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.Interpolator;
import android.widget.FrameLayout;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.WindowDelegate;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet.StateChangeReason;
import org.chromium.chrome.browser.widget.bottomsheet.EmptyBottomSheetObserver;
import org.chromium.ui.UiUtils;

/**
 * A location bar implementation specific for smaller/phone screens.
 */
public class LocationBarPhone extends LocationBarLayout {
    private static final int KEYBOARD_MODE_CHANGE_DELAY_MS = 300;
    private static final int KEYBOARD_HIDE_DELAY_MS = 150;

    private static final int ACTION_BUTTON_TOUCH_OVERFLOW_LEFT = 15;

    private static final Interpolator GOOGLE_G_FADE_INTERPOLATOR =
            new FastOutLinearInInterpolator();

    private View mFirstVisibleFocusedView;
    private @Nullable View mIncognitoBadge;
    private View mGoogleGContainer;
    private View mGoogleG;
    private int mIncognitoBadgePadding;
    private int mGoogleGWidth;
    private int mGoogleGMargin;

    private Runnable mKeyboardResizeModeTask;
    private ObjectAnimator mOmniboxBackgroundAnimator;

    /**
     * Constructor used to inflate from XML.
     */
    public LocationBarPhone(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mFirstVisibleFocusedView = findViewById(R.id.url_bar);
        mIncognitoBadge = findViewById(R.id.incognito_badge);
        mIncognitoBadgePadding =
                getResources().getDimensionPixelSize(R.dimen.location_bar_incognito_badge_padding);

        mGoogleGContainer = findViewById(R.id.google_g_container);
        mGoogleG = findViewById(R.id.google_g);
        mGoogleGWidth = getResources().getDimensionPixelSize(R.dimen.location_bar_google_g_width);
        mGoogleGMargin = getResources().getDimensionPixelSize(R.dimen.location_bar_google_g_margin);

        Rect delegateArea = new Rect();
        mUrlActionContainer.getHitRect(delegateArea);
        delegateArea.left -= ACTION_BUTTON_TOUCH_OVERFLOW_LEFT;
        TouchDelegate touchDelegate = new TouchDelegate(delegateArea, mUrlActionContainer);
        assert mUrlActionContainer.getParent() == this;
        setTouchDelegate(touchDelegate);
    }

    /**
     * @return The first view visible when the location bar is focused.
     */
    public View getFirstViewVisibleWhenFocused() {
        return mFirstVisibleFocusedView;
    }

    /**
     * Updates percentage of current the URL focus change animation.
     * @param percent 1.0 is 100% focused, 0 is completely unfocused.
     */
    public void setUrlFocusChangePercent(float percent) {
        mUrlFocusChangePercent = percent;

        if (percent > 0f) {
            mUrlActionContainer.setVisibility(VISIBLE);
        } else if (percent == 0f && !isUrlFocusChangeInProgress()) {
            // If a URL focus change is in progress, then it will handle setting the visibility
            // correctly after it completes.  If done here, it would cause the URL to jump due
            // to a badly timed layout call.
            mUrlActionContainer.setVisibility(GONE);
        }

        updateButtonVisibility();
    }

    @Override
    public void onUrlFocusChange(boolean hasFocus) {
        if (mOmniboxBackgroundAnimator != null && mOmniboxBackgroundAnimator.isRunning()) {
            mOmniboxBackgroundAnimator.cancel();
            mOmniboxBackgroundAnimator = null;
        }
        if (hasFocus) {
            // Remove the focus of this view once the URL field has taken focus as this view no
            // longer needs it.
            setFocusable(false);
            setFocusableInTouchMode(false);
        }
        setUrlFocusChangeInProgress(true);
        super.onUrlFocusChange(hasFocus);
    }

    @Override
    protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
        boolean needsCanvasRestore = false;
        if (child == mUrlBar && mUrlActionContainer.getVisibility() == VISIBLE) {
            canvas.save();

            // Clip the URL bar contents to ensure they do not draw under the URL actions during
            // focus animations.  Based on the RTL state of the location bar, the url actions
            // container can be on the left or right side, so clip accordingly.
            if (mUrlBar.getLeft() < mUrlActionContainer.getLeft()) {
                canvas.clipRect(0, 0, (int) mUrlActionContainer.getX(), getBottom());
            } else {
                canvas.clipRect(mUrlActionContainer.getX() + mUrlActionContainer.getWidth(), 0,
                        getWidth(), getBottom());
            }
            needsCanvasRestore = true;
        }
        boolean retVal = super.drawChild(canvas, child, drawingTime);
        if (needsCanvasRestore) {
            canvas.restore();
        }
        return retVal;
    }

    /**
     * Handles any actions to be performed after all other actions triggered by the URL focus
     * change.  This will be called after any animations are performed to transition from one
     * focus state to the other.
     * @param hasFocus Whether the URL field has gained focus.
     */
    public void finishUrlFocusChange(boolean hasFocus) {
        if (!hasFocus) {
            // Scroll to ensure the TLD is visible, if necessary.
            if (getScrollType() == UrlBar.SCROLL_TO_TLD) mUrlBar.scrollDisplayText();

            // The animation rendering may not yet be 100% complete and hiding the keyboard makes
            // the animation quite choppy.
            postDelayed(new Runnable() {
                @Override
                public void run() {
                    UiUtils.hideKeyboard(mUrlBar);
                }
            }, KEYBOARD_HIDE_DELAY_MS);
            // Convert the keyboard back to resize mode (delay the change for an arbitrary amount
            // of time in hopes the keyboard will be completely hidden before making this change).
            // If Chrome Home is enabled, it will handle its own mode changes.
            if (mBottomSheet == null) {
                setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE, true);
            }
            mUrlActionContainer.setVisibility(GONE);
        } else {
            // If Chrome Home is enabled, it will handle its own mode changes.
            if (mBottomSheet == null) {
                setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN, false);
            }
            UiUtils.showKeyboard(mUrlBar);
            // As the position of the navigation icon has changed, ensure the suggestions are
            // updated to reflect the new position.
            if (getSuggestionList() != null && getSuggestionList().isShown()) {
                getSuggestionList().invalidateSuggestionViews();
            }
        }
        setUrlFocusChangeInProgress(false);

        NewTabPage ntp = getToolbarDataProvider().getNewTabPageForCurrentTab();
        if (hasFocus && ntp != null && ntp.isLocationBarShownInNTP() && mBottomSheet == null) {
            if (mFadingView == null) initFadingOverlayView();
            mFadingView.showFadingOverlay();
        }
    }

    @Override
    protected void updateButtonVisibility() {
        super.updateButtonVisibility();
        updateMicButtonVisibility(mUrlFocusChangePercent);
        updateGoogleG();
    }

    private void updateGoogleG() {
        // Inflation might not be finished yet during startup.
        if (mGoogleGContainer == null) return;

        // The toolbar data provider can be null during startup, before the ToolbarManager has been
        // initialized.
        ToolbarDataProvider toolbarDataProvider = getToolbarDataProvider();
        if (toolbarDataProvider == null) return;

        if (!getToolbarDataProvider().shouldShowGoogleG(mUrlBar.getEditableText().toString())) {
            mGoogleGContainer.setVisibility(View.GONE);
            return;
        }

        mGoogleGContainer.setVisibility(View.VISIBLE);
        float animationProgress =
                GOOGLE_G_FADE_INTERPOLATOR.getInterpolation(mUrlFocusChangePercent);

        final float finalGScale = 0.3f;
        // How much we have reduced the size of the G, 0 at the beginning, 0.7 at the end.
        final float shrinkingProgress = animationProgress * (1 - finalGScale);

        FrameLayout.LayoutParams layoutParams =
                (FrameLayout.LayoutParams) mGoogleG.getLayoutParams();
        layoutParams.width = Math.round(mGoogleGWidth * (1f - shrinkingProgress));

        // Shrink the margin down to 50% minus half of the G width (in the end state).
        final float finalGoogleGMargin = (mGoogleGMargin - mGoogleGWidth * finalGScale) / 2f;
        ApiCompatibilityUtils.setMarginEnd(layoutParams, Math.round(MathUtils.interpolate(
                mGoogleGMargin, finalGoogleGMargin, animationProgress)));
        // Just calling requestLayout() would not resolve the end margin.
        mGoogleG.setLayoutParams(layoutParams);

        // We want the G to be fully transparent when it is 45% of its size.
        final float scaleWhenTransparent = 0.45f;
        assert scaleWhenTransparent >= finalGScale;

        // How much we have faded out the G, 0 at the beginning, 1 when we've reduced size to 0.45.
        final float fadingProgress = Math.min(1, shrinkingProgress / (1 - scaleWhenTransparent));
        mGoogleG.setAlpha(1 - fadingProgress);
    }

    @Override
    protected void updateLocationBarIconContainerVisibility() {
        super.updateLocationBarIconContainerVisibility();
        updateIncognitoBadgePadding();
    }

    private void updateIncognitoBadgePadding() {
        // This can be triggered in the super.onFinishInflate, so we need to null check in this
        // place only.
        if (mIncognitoBadge == null) return;

        if (findViewById(R.id.location_bar_icon).getVisibility() == GONE) {
            ApiCompatibilityUtils.setPaddingRelative(
                    mIncognitoBadge, 0, 0, mIncognitoBadgePadding, 0);
        } else {
            ApiCompatibilityUtils.setPaddingRelative(mIncognitoBadge, 0, 0, 0, 0);
        }
    }

    @Override
    public void updateVisualsForState() {
        super.updateVisualsForState();

        if (mIncognitoBadge == null) return;

        boolean showIncognitoBadge =
                getToolbarDataProvider() != null && getToolbarDataProvider().isIncognito();
        mIncognitoBadge.setVisibility(showIncognitoBadge ? VISIBLE : GONE);
        updateIncognitoBadgePadding();
    }

    @Override
    protected boolean shouldAnimateIconChanges() {
        return super.shouldAnimateIconChanges() || isUrlFocusChangeInProgress();
    }

    @Override
    public void setLayoutDirection(int layoutDirection) {
        super.setLayoutDirection(layoutDirection);
        updateIncognitoBadgePadding();
    }

    /**
     * @return Whether the incognito badge is currently visible.
     */
    public boolean isIncognitoBadgeVisible() {
        return mIncognitoBadge != null && mIncognitoBadge.getVisibility() == View.VISIBLE;
    }

    /**
     * @param softInputMode The software input resize mode.
     * @param delay Delay the change in input mode.
     */
    private void setSoftInputMode(final int softInputMode, boolean delay) {
        final WindowDelegate delegate = getWindowDelegate();

        if (mKeyboardResizeModeTask != null) {
            removeCallbacks(mKeyboardResizeModeTask);
            mKeyboardResizeModeTask = null;
        }

        if (delegate == null || delegate.getWindowSoftInputMode() == softInputMode) return;

        if (delay) {
            mKeyboardResizeModeTask = new Runnable() {
                @Override
                public void run() {
                    delegate.setWindowSoftInputMode(softInputMode);
                    mKeyboardResizeModeTask = null;
                }
            };
            postDelayed(mKeyboardResizeModeTask, KEYBOARD_MODE_CHANGE_DELAY_MS);
        } else {
            delegate.setWindowSoftInputMode(softInputMode);
        }
    }

    @Override
    public void setBottomSheet(BottomSheet sheet) {
        super.setBottomSheet(sheet);

        sheet.addObserver(new EmptyBottomSheetObserver() {
            @Override
            public void onSheetStateChanged(int state) {
                switch (state) {
                    case BottomSheet.SHEET_STATE_FULL:
                        setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN, false);
                        break;
                    case BottomSheet.SHEET_STATE_PEEK:
                        setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE, true);
                        break;
                    default:
                        setSoftInputMode(
                                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING, false);
                }
            }

            @Override
            public void onSheetOpened(@StateChangeReason int reason) {
                updateGoogleG();
            }

            @Override
            public void onSheetClosed(@StateChangeReason int reason) {
                updateGoogleG();
            }
        });

        // Chrome Home does not use the incognito badge. Remove the View to save memory.
        removeView(mIncognitoBadge);
        mIncognitoBadge = null;

        // TODO(twellington): remove and null out mGoogleG and mGoogleGContainer if we remove
        //                    support for the Google 'G' to save memory.
    }

    @Override
    public void onNativeLibraryReady() {
        super.onNativeLibraryReady();

        // TODO(twellington): Move this to constructor when isModernUiEnabled() is available before
        // native is loaded.
        if (useModernDesign()) {
            // Modern does not use the incognito badge. Remove the View to save memory.
            removeView(mIncognitoBadge);
            mIncognitoBadge = null;
        }
    }
}
