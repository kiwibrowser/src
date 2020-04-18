// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.selection;

import android.os.Build;
import android.support.annotation.IntDef;
import android.text.TextUtils;
import android.view.textclassifier.TextClassifier;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionMetricsLogger;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * A class that controls Smart Text selection. Smart Text selection automatically augments the
 * selected boundaries and classifies the selected text based on the context.
 * This class requests the selection together with its surrounding text from the focused frame and
 * sends it to SmartSelectionProvider which does the classification itself.
 */
@JNINamespace("content")
public class SmartSelectionClient implements SelectionClient {
    @IntDef({CLASSIFY, SUGGEST_AND_CLASSIFY})
    @Retention(RetentionPolicy.SOURCE)
    private @interface RequestType {}

    // Request to obtain the type (e.g. phone number, e-mail address) and the most
    // appropriate operation for the selected text.
    private static final int CLASSIFY = 0;

    // Request to obtain the type (e.g. phone number, e-mail address), the most
    // appropriate operation for the selected text and a better selection boundaries.
    private static final int SUGGEST_AND_CLASSIFY = 1;

    // The maximal number of characters on the left and on the right from the current selection.
    // Used for surrounding text request.
    private static final int NUM_EXTRA_CHARS = 240;

    private long mNativeSmartSelectionClient;
    private SmartSelectionProvider mProvider;
    private ResultCallback mCallback;
    private SmartSelectionMetricsLogger mSmartSelectionMetricLogger;

    /**
     * Creates the SmartSelectionClient. Returns null in case SmartSelectionProvider does not exist
     * in the system.
     */
    public static SmartSelectionClient create(ResultCallback callback, WebContents webContents) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || windowAndroid == null) return null;

        return new SmartSelectionClient(callback, webContents, windowAndroid);
    }

    private SmartSelectionClient(
            ResultCallback callback, WebContents webContents, WindowAndroid windowAndroid) {
        assert Build.VERSION.SDK_INT >= Build.VERSION_CODES.O;
        mProvider = new SmartSelectionProvider(callback, windowAndroid);
        mCallback = callback;
        mSmartSelectionMetricLogger =
                SmartSelectionMetricsLogger.create(windowAndroid.getContext().get());
        mNativeSmartSelectionClient = nativeInit(webContents);
    }

    @CalledByNative
    private void onNativeSideDestroyed(long nativeSmartSelectionClient) {
        assert nativeSmartSelectionClient == mNativeSmartSelectionClient;
        mNativeSmartSelectionClient = 0;
        mProvider.cancelAllRequests();
    }

    // SelectionClient implementation
    @Override
    public void onSelectionChanged(String selection) {}

    @Override
    public void onSelectionEvent(int eventType, float posXPix, float posYPix) {}

    @Override
    public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {}

    @Override
    public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
        requestSurroundingText(shouldSuggest ? SUGGEST_AND_CLASSIFY : CLASSIFY);
        return true;
    }

    @Override
    public void cancelAllRequests() {
        if (mNativeSmartSelectionClient != 0) {
            nativeCancelAllRequests(mNativeSmartSelectionClient);
        }

        mProvider.cancelAllRequests();
    }

    @Override
    public SelectionMetricsLogger getSelectionMetricsLogger() {
        return mSmartSelectionMetricLogger;
    }

    @Override
    public void setTextClassifier(TextClassifier textClassifier) {
        mProvider.setTextClassifier(textClassifier);
    }

    @Override
    public TextClassifier getTextClassifier() {
        return mProvider.getTextClassifier();
    }

    @Override
    public TextClassifier getCustomTextClassifier() {
        return mProvider.getCustomTextClassifier();
    }

    private void requestSurroundingText(@RequestType int callbackData) {
        if (mNativeSmartSelectionClient == 0) {
            onSurroundingTextReceived(callbackData, "", 0, 0);
            return;
        }

        nativeRequestSurroundingText(mNativeSmartSelectionClient, NUM_EXTRA_CHARS, callbackData);
    }

    @CalledByNative
    private void onSurroundingTextReceived(
            @RequestType int callbackData, String text, int start, int end) {
        if (!textHasValidSelection(text, start, end)) {
            mCallback.onClassified(new Result());
            return;
        }

        switch (callbackData) {
            case SUGGEST_AND_CLASSIFY:
                mProvider.sendSuggestAndClassifyRequest(text, start, end, null);
                break;

            case CLASSIFY:
                mProvider.sendClassifyRequest(text, start, end, null);
                break;

            default:
                assert false : "Unexpected callback data";
                break;
        }
    }

    private boolean textHasValidSelection(String text, int start, int end) {
        return !TextUtils.isEmpty(text) && 0 <= start && start < end && end <= text.length();
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeRequestSurroundingText(
            long nativeSmartSelectionClient, int numExtraCharacters, int callbackData);
    private native void nativeCancelAllRequests(long nativeSmartSelectionClient);
}
