// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.accessibility.captioning;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.WebContents;

/**
 *  Sends notification when platform closed caption settings have changed.
 */
@JNINamespace("content")
public class CaptioningController implements SystemCaptioningBridge.SystemCaptioningBridgeListener {
    private SystemCaptioningBridge mSystemCaptioningBridge;
    private long mNativeCaptioningController;

    public CaptioningController(WebContents webContents, Context context) {
        mSystemCaptioningBridge = CaptioningBridgeFactory.getSystemCaptioningBridge(context);
        mNativeCaptioningController = nativeInit(webContents);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onDestroy() {
        mNativeCaptioningController = 0;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onRenderProcessChange() {
        // Immediately sync closed caption settings to the new render process.
        mSystemCaptioningBridge.syncToListener(this);
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    @Override
    public void onSystemCaptioningChanged(TextTrackSettings settings) {
        if (mNativeCaptioningController == 0) return;
        nativeSetTextTrackSettings(mNativeCaptioningController, settings.getTextTracksEnabled(),
                settings.getTextTrackBackgroundColor(), settings.getTextTrackFontFamily(),
                settings.getTextTrackFontStyle(), settings.getTextTrackFontVariant(),
                settings.getTextTrackTextColor(), settings.getTextTrackTextShadow(),
                settings.getTextTrackTextSize());
    }

    public void startListening() {
        mSystemCaptioningBridge.addListener(this);
    }

    public void stopListening() {
        mSystemCaptioningBridge.removeListener(this);
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeSetTextTrackSettings(long nativeCaptioningController,
            boolean textTracksEnabled, String textTrackBackgroundColor, String textTrackFontFamily,
            String textTrackFontStyle, String textTrackFontVariant, String textTrackTextColor,
            String textTrackTextShadow, String textTrackTextSize);
}
