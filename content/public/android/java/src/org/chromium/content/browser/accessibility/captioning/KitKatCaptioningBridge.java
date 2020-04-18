// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.accessibility.captioning;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.view.accessibility.CaptioningManager;

import java.util.Locale;

/**
 * This is the implementation of SystemCaptioningBridge that uses CaptioningManager
 * on KitKat+ systems.
 */
@TargetApi(Build.VERSION_CODES.KITKAT)
public class KitKatCaptioningBridge implements SystemCaptioningBridge {
    private final CaptioningManager.CaptioningChangeListener mCaptioningChangeListener =
            new KitKatCaptioningChangeListener();

    private final CaptioningChangeDelegate mCaptioningChangeDelegate;
    private final CaptioningManager mCaptioningManager;
    private static KitKatCaptioningBridge sKitKatCaptioningBridge;

    /**
     * Bridge listener to inform the mCaptioningChangeDelegate when the mCaptioningManager
     * broadcasts any changes.
     */
    private class KitKatCaptioningChangeListener extends
            CaptioningManager.CaptioningChangeListener {
        @Override
        public void onEnabledChanged(boolean enabled) {
            mCaptioningChangeDelegate.onEnabledChanged(enabled);
        }

        @Override
        public void onFontScaleChanged(float fontScale) {
            mCaptioningChangeDelegate.onFontScaleChanged(fontScale);
        }

        @Override
        public void onLocaleChanged(Locale locale) {
            mCaptioningChangeDelegate.onLocaleChanged(locale);
        }

        @Override
        public void onUserStyleChanged(CaptioningManager.CaptionStyle userStyle) {
            final CaptioningStyle captioningStyle = getCaptioningStyleFrom(userStyle);
            mCaptioningChangeDelegate.onUserStyleChanged(captioningStyle);
        }
    }

    /**
     * Return the singleton instance of the captioning bridge for Kitkat+
     *
     * @param context the Context to associate with this bridge.
     * @return the singleton instance of KitKatCaptioningBridge.
     */
    public static KitKatCaptioningBridge getInstance(Context context) {
        if (sKitKatCaptioningBridge == null) {
            sKitKatCaptioningBridge = new KitKatCaptioningBridge(context);
        }
        return sKitKatCaptioningBridge;
    }

    /**
     * Construct a new KitKat+ captioning bridge
     *
     * @param context the Context to associate with this bridge.
     */
    private KitKatCaptioningBridge(Context context) {
        mCaptioningChangeDelegate = new CaptioningChangeDelegate();
        mCaptioningManager =
                (CaptioningManager) context.getApplicationContext().getSystemService(
                        Context.CAPTIONING_SERVICE);
    }

    /**
     * Force-sync the current closed caption settings to the delegate
     */
    private void syncToDelegate() {
        mCaptioningChangeDelegate.onEnabledChanged(mCaptioningManager.isEnabled());
        mCaptioningChangeDelegate.onFontScaleChanged(mCaptioningManager.getFontScale());
        mCaptioningChangeDelegate.onLocaleChanged(mCaptioningManager.getLocale());
        mCaptioningChangeDelegate.onUserStyleChanged(
                getCaptioningStyleFrom(mCaptioningManager.getUserStyle()));
    }

    @Override
    public void syncToListener(SystemCaptioningBridge.SystemCaptioningBridgeListener listener) {
        if (!mCaptioningChangeDelegate.hasActiveListener()) {
            syncToDelegate();
        }
        mCaptioningChangeDelegate.notifyListener(listener);
    }

    @Override
    public void addListener(SystemCaptioningBridge.SystemCaptioningBridgeListener listener) {
        if (!mCaptioningChangeDelegate.hasActiveListener()) {
            mCaptioningManager.addCaptioningChangeListener(mCaptioningChangeListener);
            syncToDelegate();
        }
        mCaptioningChangeDelegate.addListener(listener);
        mCaptioningChangeDelegate.notifyListener(listener);
    }

    @Override
    public void removeListener(SystemCaptioningBridge.SystemCaptioningBridgeListener listener) {
        mCaptioningChangeDelegate.removeListener(listener);
        if (!mCaptioningChangeDelegate.hasActiveListener()) {
            mCaptioningManager.removeCaptioningChangeListener(mCaptioningChangeListener);
        }
    }

    /**
     * Create a Chromium CaptioningStyle from a platform CaptionStyle
     *
     * @param userStyle the platform CaptionStyle
     * @return a Chromium CaptioningStyle
     */
    private CaptioningStyle getCaptioningStyleFrom(CaptioningManager.CaptionStyle userStyle) {
        return CaptioningStyle.createFrom(userStyle);
    }
}
