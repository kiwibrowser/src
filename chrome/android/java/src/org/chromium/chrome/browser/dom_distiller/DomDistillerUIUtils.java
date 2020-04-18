// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.dom_distiller;

import android.app.Activity;
import android.support.v7.app.AlertDialog;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * Java implementation of dom_distiller::android::DistillerUIHandleAndroid.
 */
@JNINamespace("dom_distiller::android")
public final class DomDistillerUIUtils {
    // Static handle to Reader Mode's manager.
    private static ReaderModeManager sManagerManager;

    /**
     * Set the delegate to the ReaderModeManager.
     * @param manager The class managing Reader Mode.
     */
    public static void setReaderModeManagerDelegate(ReaderModeManager manager) {
        sManagerManager = manager;
    }

    /**
     * A static method for native code to call to open the distiller UI settings.
     * @param webContents The WebContents containing the distilled content.
     */
    @CalledByNative
    public static void openSettings(WebContents webContents) {
        Activity activity = getActivityFromWebContents(webContents);
        if (webContents != null && activity != null) {
            RecordUserAction.record("DomDistiller_DistilledPagePrefsOpened");
            AlertDialog.Builder builder =
                    new AlertDialog.Builder(activity, R.style.AlertDialogTheme);
            builder.setView(DistilledPagePrefsView.create(activity));
            builder.show();
        }
    }

    /**
     * Clear static references to objects.
     * @param manager The manager requesting the destoy. This prevents different managers in
     * document mode from accidentally clearing a reference it doesn't own.
     */
    public static void destroy(ReaderModeManager manager) {
        if (manager != sManagerManager) return;
        sManagerManager = null;
    }

    /**
     * @param webContents The WebContents to get the Activity from.
     * @return The Activity associated with the WebContents.
     */
    private static Activity getActivityFromWebContents(WebContents webContents) {
        if (webContents == null) return null;

        WindowAndroid window = webContents.getTopLevelNativeWindow();
        if (window == null) return null;

        return window.getActivity().get();
    }

    private DomDistillerUIUtils() {}
}
