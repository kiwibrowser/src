// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.view.MotionEvent;
import android.view.View;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.MotionEventSynthesizer;

/**
 * Forwards events for Java native UI pages to MotionEventSynthesizer.
 */
@JNINamespace("vr")
public class AndroidUiGestureTarget {
    private final MotionEventSynthesizer mMotionEventSynthesizer;
    private final long mNativePointer;

    public AndroidUiGestureTarget(
            View target, float scaleFactor, float scrollRatio, int touchSlop) {
        mMotionEventSynthesizer = MotionEventSynthesizer.create(target);
        mNativePointer = nativeInit(scaleFactor, scrollRatio, touchSlop);
    }

    @CalledByNative
    private void inject(int action, long timeInMs) {
        mMotionEventSynthesizer.inject(action, 1 /* pointerCount */, timeInMs);
    }

    @CalledByNative
    private void setPointer(int x, int y) {
        mMotionEventSynthesizer.setPointer(
                0 /* index */, x, y, 0 /* id */, MotionEvent.TOOL_TYPE_STYLUS);
    }

    @CalledByNative
    private long getNativeObject() {
        return mNativePointer;
    }

    private native long nativeInit(float scaleFactor, float scrollRatio, int touchSlop);
}
