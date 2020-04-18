// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import org.chromium.base.ObserverList;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid;
import org.chromium.ui.display.DisplayAndroid.DisplayAndroidObserver;

/**
 * Manages {@link WindowEventObserver} instances used for WebContents.
 */
public final class WindowEventObserverManager implements DisplayAndroidObserver {
    private final ObserverList<WindowEventObserver> mWindowEventObservers = new ObserverList<>();

    private WindowAndroid mWindowAndroid;
    private boolean mAttachedToWindow;

    // The cache of device's current orientation and DIP scale factor.
    private int mRotation;
    private float mDipScale;

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<WindowEventObserverManager> INSTANCE =
                WindowEventObserverManager::new;
    }

    public static WindowEventObserverManager from(WebContents webContents) {
        return webContents.getOrSetUserData(
                WindowEventObserverManager.class, UserDataFactoryLazyHolder.INSTANCE);
    }

    private WindowEventObserverManager(WebContents webContents) {
        WindowAndroid window = webContents.getTopLevelNativeWindow();
        if (window != null) onWindowAndroidChanged(window);
    }

    /**
     * Add {@link WindowEventObserver} object.
     * @param observer Observer instance to add.
     */
    public void addObserver(WindowEventObserver observer) {
        assert !mWindowEventObservers.hasObserver(observer);
        mWindowEventObservers.addObserver(observer);
    }

    /**
     * Remove {@link WindowEventObserver} object.
     * @param observer Observer instance to remove.
     */
    public void removeObserver(WindowEventObserver observer) {
        assert mWindowEventObservers.hasObserver(observer);
        mWindowEventObservers.removeObserver(observer);
    }

    /**
     * @see android.view.View#onAttachedToWindow()
     */
    public void onAttachedToWindow() {
        mAttachedToWindow = true;
        addDisplayAndroidObserverIfNeeded();
        for (WindowEventObserver observer : mWindowEventObservers) observer.onAttachedToWindow();
    }

    /**
     * @see android.view.View#onDetachedFromWindow()
     */
    public void onDetachedFromWindow() {
        mAttachedToWindow = false;
        removeDisplayAndroidObserver();
        for (WindowEventObserver observer : mWindowEventObservers) observer.onDetachedFromWindow();
    }

    /**
     * @see android.view.View#onWindowFocusChanged()
     */
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        for (WindowEventObserver observer : mWindowEventObservers) {
            observer.onWindowFocusChanged(hasWindowFocus);
        }
    }

    /**
     * Called when {@link WindowAndroid} for WebContents is updated.
     * @param windowAndroid A new WindowAndroid object.
     */
    public void onWindowAndroidChanged(WindowAndroid windowAndroid) {
        removeDisplayAndroidObserver();
        mWindowAndroid = windowAndroid;
        addDisplayAndroidObserverIfNeeded();
        for (WindowEventObserver observer : mWindowEventObservers) {
            observer.onWindowAndroidChanged(windowAndroid);
        }
    }

    private void addDisplayAndroidObserverIfNeeded() {
        if (!mAttachedToWindow || mWindowAndroid == null) return;
        DisplayAndroid display = mWindowAndroid.getDisplay();
        display.addObserver(this);
        onRotationChanged(display.getRotation());
        onDIPScaleChanged(display.getDipScale());
    }

    private void removeDisplayAndroidObserver() {
        if (mWindowAndroid != null) mWindowAndroid.getDisplay().removeObserver(this);
    }

    // DisplayAndroidObserver

    @Override
    public void onRotationChanged(int rotation) {
        if (mRotation == rotation) return;
        mRotation = rotation;
        for (WindowEventObserver observer : mWindowEventObservers) {
            observer.onRotationChanged(rotation);
        }
    }

    @Override
    public void onDIPScaleChanged(float dipScale) {
        if (mDipScale == dipScale) return;
        mDipScale = dipScale;
        for (WindowEventObserver observer : mWindowEventObservers) {
            observer.onDIPScaleChanged(dipScale);
        }
    }
}
