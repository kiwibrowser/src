// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.view.ViewGroup;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.PopupZoomer.OnTapListener;
import org.chromium.content.browser.PopupZoomer.OnVisibilityChangedListener;
import org.chromium.content.browser.input.ImeAdapterImpl;
import org.chromium.content_public.browser.ImeEventObserver;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;

/**
 * Class that handles tap disambiguation feature.  When a tap lands ambiguously
 * between two tiny touch targets (usually links) on a desktop site viewed on a phone,
 * a magnified view of the content is shown, the screen is grayed out and the user
 * must re-tap the magnified content in order to clarify their intent.
 */
@JNINamespace("content")
public class TapDisambiguator implements ImeEventObserver, PopupController.HideablePopup {
    private final WebContents mWebContents;
    private PopupZoomer mPopupView;
    private boolean mInitialized;
    private long mNativeTapDisambiguator;

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<TapDisambiguator> INSTANCE = TapDisambiguator::new;
    }

    public static TapDisambiguator create(
            Context context, WebContents webContents, ViewGroup containerView) {
        TapDisambiguator tabDismabiguator = webContents.getOrSetUserData(
                TapDisambiguator.class, UserDataFactoryLazyHolder.INSTANCE);
        assert tabDismabiguator != null;
        assert !tabDismabiguator.initialized();

        tabDismabiguator.init(context, containerView);
        return tabDismabiguator;
    }

    public static TapDisambiguator fromWebContents(WebContents webContents) {
        return webContents.getOrSetUserData(TapDisambiguator.class, null);
    }

    /**
     * Creates TapDisambiguation instance.
     * @param webContents WebContents instance with which this TapDisambiguator is associated.
     */
    public TapDisambiguator(WebContents webContents) {
        mWebContents = webContents;
    }

    private void init(Context context, ViewGroup containerView) {
        // OnVisibilityChangedListener, OnTapListener can only be used to add and remove views
        // from the container view at creation.
        OnVisibilityChangedListener visibilityListener = new OnVisibilityChangedListener() {
            @Override
            public void onPopupZoomerShown(final PopupZoomer zoomer) {
                containerView.post(new Runnable() {
                    @Override
                    public void run() {
                        if (containerView.indexOfChild(zoomer) == -1) {
                            containerView.addView(zoomer);
                        }
                    }
                });
            }

            @Override
            public void onPopupZoomerHidden(final PopupZoomer zoomer) {
                containerView.post(new Runnable() {
                    @Override
                    public void run() {
                        if (containerView.indexOfChild(zoomer) != -1) {
                            containerView.removeView(zoomer);
                            containerView.invalidate();
                        }
                    }
                });
            }
        };

        OnTapListener tapListener = new OnTapListener() {
            @Override
            public void onResolveTapDisambiguation(
                    long timeMs, float x, float y, boolean isLongPress) {
                if (mNativeTapDisambiguator == 0) return;
                containerView.requestFocus();
                nativeResolveTapDisambiguation(mNativeTapDisambiguator, timeMs, x, y, isLongPress);
            }
        };
        mPopupView = new PopupZoomer(context, containerView, visibilityListener, tapListener);
        mNativeTapDisambiguator = nativeInit(mWebContents);
        ImeAdapterImpl.fromWebContents(mWebContents).addEventObserver(this);
        PopupController.register(mWebContents, this);
        mInitialized = true;
    }

    private boolean initialized() {
        return mInitialized;
    }

    // ImeEventObserver

    @Override
    public void onImeEvent() {
        hidePopup(true);
    }

    // HideablePopup

    @Override
    public void hide() {
        hidePopup(false);
    }

    /**
     * Returns true if the view is currently being shown (or is animating).
     */
    public boolean isShowing() {
        return mPopupView.isShowing();
    }

    /**
     * Sets the last touch point (on the unzoomed view).
     */
    public void setLastTouch(float x, float y) {
        mPopupView.setLastTouch(x, y);
    }

    /**
     * Show the TapDisambiguator view with given target bounds.
     */
    public void showPopup(Rect rect) {
        mPopupView.show(rect);
    }

    /**
     * Hide the TapDisambiguator view because of some external event such as focus
     * change, JS-originating scroll, etc.
     * @param animation true if hide with animation.
     */
    public void hidePopup(boolean animation) {
        mPopupView.hide(animation);
    }

    /**
     * Called when back button is pressed.
     */
    public void backButtonPressed() {
        mPopupView.backButtonPressed();
    }

    @CalledByNative
    private void destroy() {
        mNativeTapDisambiguator = 0;
    }

    @CalledByNative
    private void showPopup(Rect targetRect, Bitmap zoomedBitmap) {
        mPopupView.setBitmap(zoomedBitmap);
        mPopupView.show(targetRect);
    }

    @CalledByNative
    private void hidePopup() {
        hidePopup(false);
    }

    @CalledByNative
    private static Rect createRect(int x, int y, int right, int bottom) {
        return new Rect(x, y, right, bottom);
    }

    @VisibleForTesting
    void setPopupZoomerForTest(PopupZoomer view) {
        mPopupView = view;
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeResolveTapDisambiguation(
            long nativeTapDisambiguator, long timeMs, float x, float y, boolean isLongPress);
}
