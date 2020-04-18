// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui;

import android.content.Context;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Exposes IME related code to native code.
 */
@JNINamespace("ui")
class PlatformImeControllerAndroid {
    private int mInputType = 0;
    private int mInputFlags = 0;
    private String mText = "";
    private int mSelectionStart = 0;
    private int mSelectionEnd = 0;
    private int mCompositionStart = 0;
    private int mCompositionEnd = 0;

    private final PlatformWindowAndroid mWindow;
    private final long mNativeHandle;
    private final InputMethodManager mInputMethodManager;
    private InputConnection mInputConnection;

    PlatformImeControllerAndroid(PlatformWindowAndroid window, long nativeHandle) {
        mWindow = window;
        mNativeHandle = nativeHandle;
        mInputMethodManager = (InputMethodManager) mWindow.getContext().getSystemService(
                Context.INPUT_METHOD_SERVICE);
        assert mNativeHandle != 0;
        nativeInit(mNativeHandle);
    }

    boolean isTextEditorType() {
        return mInputType != 0;
    }

    InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mInputType == 0) {
            // Although onCheckIsTextEditor will return false in this case, the EditorInfo
            // is still used by the InputMethodService. Need to make sure the IME doesn't
            // enter fullscreen mode.
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;
        }

        // TODO(penghuang): Support full editor.
        final boolean fullEditor = false;
        mInputConnection = new BaseInputConnection(mWindow, fullEditor);
        outAttrs.actionLabel = null;
        // TODO(penghuang): Pass blink text input type to Android framework.
        outAttrs.inputType =
                EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_WEB_EDIT_TEXT;
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN
                | EditorInfo.IME_ACTION_GO;
        return mInputConnection;
    }

    @CalledByNative
    private void updateTextInputState(int textInputType, int textInputFlags, String text,
            int selectionStart, int selectionEnd, int compositionStart, int compositionEnd) {
        mInputType = textInputType;
        mInputFlags = textInputFlags;
        mText = text;
        mSelectionStart = selectionStart;
        mSelectionEnd = selectionEnd;
        mCompositionStart = compositionStart;
        mCompositionEnd = compositionEnd;
        // Update keyboard visibility
        if (mInputType == 0) {
            dismissInput();
        }
    }

    @CalledByNative
    private void setImeVisibility(boolean visible) {
        // The IME is visible only if |mInputType| isn't 0, so we don't need
        // change the visibility if |mInputType| is 0.
        if (mInputType != 0) {
            if (visible) {
                showKeyboard();
            } else {
                dismissInput();
            }
        }
    }

    private void showKeyboard() {
        mInputMethodManager.showSoftInput(mWindow, 0);
    }

    private void dismissInput() {
        mInputMethodManager.hideSoftInputFromWindow(mWindow.getWindowToken(), 0);
    }

    // The generated native method implementation will call
    // PlatformImeControllerAndroid::Init(JNIEnv* env, jobject self)
    private native void nativeInit(long nativePlatformImeControllerAndroid);
}
