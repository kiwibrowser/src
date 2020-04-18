// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.content.Context;
import android.graphics.Bitmap;
import android.view.View;
import android.widget.TextView;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.snackbar.SnackbarManager;

/**
 * The base class for all InfoBar classes.
 * Note that infobars expire by default when a new navigation occurs.
 * Make sure to use setExpireOnNavigation(false) if you want an infobar to be sticky.
 */
public abstract class InfoBar implements InfoBarView {
    private static final String TAG = "InfoBar";

    private final int mIconDrawableId;
    private final Bitmap mIconBitmap;
    private final CharSequence mMessage;

    private InfoBarContainer mContainer;
    private View mView;
    private Context mContext;

    private boolean mIsDismissed;
    private boolean mControlsEnabled = true;

    // This points to the InfoBarAndroid class not any of its subclasses.
    private long mNativeInfoBarPtr;

    /**
     * Constructor for regular infobars.
     * @param iconDrawableId ID of the resource to use for the Icon.  If 0, no icon will be shown.
     * @param iconBitmap Icon to draw, in bitmap form.  Used mainly for generated icons.
     * @param message The message to show in the infobar.
     */
    public InfoBar(int iconDrawableId, Bitmap iconBitmap, CharSequence message) {
        mIconDrawableId = iconDrawableId;
        mIconBitmap = iconBitmap;
        mMessage = message;
    }

    /**
     * Stores a pointer to the native-side counterpart of this InfoBar.
     * @param nativeInfoBarPtr Pointer to the native InfoBarAndroid, not to its subclass.
     */
    @CalledByNative
    private final void setNativeInfoBar(long nativeInfoBarPtr) {
        mNativeInfoBarPtr = nativeInfoBarPtr;
    }

    @CalledByNative
    protected void onNativeDestroyed() {
        mNativeInfoBarPtr = 0;
    }

    public SnackbarManager getSnackbarManager() {
        return mContainer != null ? mContainer.getSnackbarManager() : null;
    }

    /**
     * Sets the Context used when creating the InfoBar.
     */
    protected void setContext(Context context) {
        mContext = context;
    }

    /**
     * @return The Context used to create the InfoBar.  This will be null until the InfoBar is added
     *         to the InfoBarContainer, and should never be null afterward.
     */
    protected Context getContext() {
        return mContext;
    }

    /**
     * Creates the View that represents the InfoBar.
     * @return The View representing the InfoBar.
     */
    protected final View createView() {
        assert mContext != null;

        if (usesCompactLayout()) {
            InfoBarCompactLayout layout =
                    new InfoBarCompactLayout(mContext, this, mIconDrawableId, mIconBitmap);
            createCompactLayoutContent(layout);
            mView = layout;
        } else {
            InfoBarLayout layout =
                    new InfoBarLayout(mContext, this, mIconDrawableId, mIconBitmap, mMessage);
            createContent(layout);
            layout.onContentCreated();
            mView = layout;
        }

        return mView;
    }

    /**
     * If this returns true, the infobar contents will be replaced with a one-line layout.
     * When overriding this, also override {@link #getAccessibilityMessage}.
     */
    protected boolean usesCompactLayout() {
        return false;
    }

    /**
     * Prepares and inserts views into an {@link InfoBarCompactLayout}.
     * {@link #usesCompactLayout} must return 'true' for this function to be called.
     * @param layout Layout to plug views into.
     */
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {}

    /**
     * Replaces the View currently shown in the infobar with the given View. Triggers the swap
     * animation via the InfoBarContainer.
     */
    protected void replaceView(View newView) {
        mView = newView;
        mContainer.notifyInfoBarViewChanged();
    }

    /**
     * Returns the View shown in this infobar. Only valid after createView() has been called.
     */
    @Override
    public View getView() {
        return mView;
    }

    /**
     * Returns the accessibility message to announce when this infobar is first shown.
     * Override this if the InfoBar doesn't have {@link R.id.infobar_message}. It is usually the
     * case when it is in CompactLayout.
     */
    protected CharSequence getAccessibilityMessage(CharSequence defaultTitle) {
        return defaultTitle == null ? "" : defaultTitle;
    }

    @Override
    public CharSequence getAccessibilityText() {
        if (mView == null) return "";

        CharSequence title = null;
        TextView messageView = (TextView) mView.findViewById(R.id.infobar_message);
        if (messageView != null) {
            title = messageView.getText();
        }
        title = getAccessibilityMessage(title);
        if (title.length() > 0) {
            title = title + " ";
        }
        // TODO(crbug/773717): Avoid string concatenation due to i18n.
        return title + mContext.getString(R.string.bottom_bar_screen_position);
    }

    @Override
    public boolean isLegalDisclosure() {
        return false;
    }

    @Override
    public boolean isBottomMostInfoBar() {
        return false;
    }

    @Override
    @InfoBarIdentifier
    public int getInfoBarIdentifier() {
        if (mNativeInfoBarPtr == 0) return InfoBarIdentifier.INVALID;
        return nativeGetInfoBarIdentifier(mNativeInfoBarPtr);
    }

    /**
     * @return whether the infobar actually needed closing.
     */
    @CalledByNative
    private boolean closeInfoBar() {
        if (!mIsDismissed) {
            mIsDismissed = true;
            if (!mContainer.hasBeenDestroyed()) {
                // If the container was destroyed, it's already been emptied of all its infobars.
                onStartedHiding();
                mContainer.removeInfoBar(this);
            }
            return true;
        }
        return false;
    }

    /**
     * @return If the infobar is the front infobar (i.e. visible and not hidden behind other
     *         infobars).
     */
    public boolean isFrontInfoBar() {
        return mContainer.getFrontInfoBar() == this;
    }

    /**
     * Called just before the Java infobar has begun hiding.  Give the chance to clean up any child
     * UI that may remain open.
     */
    protected void onStartedHiding() {}

    long getNativeInfoBarPtr() {
        return mNativeInfoBarPtr;
    }

    void setInfoBarContainer(InfoBarContainer container) {
        mContainer = container;
    }

    @Override
    public boolean areControlsEnabled() {
        return mControlsEnabled;
    }

    @Override
    public void setControlsEnabled(boolean state) {
        mControlsEnabled = state;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
    }

    @Override
    public void onLinkClicked() {
        if (mNativeInfoBarPtr != 0) nativeOnLinkClicked(mNativeInfoBarPtr);
    }

    /**
     * Performs some action related to the button being clicked.
     * @param action The type of action defined in {@link ActionType} in this class.
     */
    protected void onButtonClicked(@ActionType int action) {
        if (mNativeInfoBarPtr != 0) nativeOnButtonClicked(mNativeInfoBarPtr, action);
    }

    @Override
    public void onCloseButtonClicked() {
        if (mNativeInfoBarPtr != 0) nativeOnCloseButtonClicked(mNativeInfoBarPtr);
    }

    @Override
    public void createContent(InfoBarLayout layout) {
    }

    @InfoBarIdentifier
    private native int nativeGetInfoBarIdentifier(long nativeInfoBarAndroid);
    private native void nativeOnLinkClicked(long nativeInfoBarAndroid);
    private native void nativeOnButtonClicked(long nativeInfoBarAndroid, int action);
    private native void nativeOnCloseButtonClicked(long nativeInfoBarAndroid);
}
