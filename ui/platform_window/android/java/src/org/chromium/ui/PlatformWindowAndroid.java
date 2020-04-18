// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui;

import android.app.Activity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Exposes SurfaceView to native code.
 */
@JNINamespace("ui")
public class PlatformWindowAndroid extends SurfaceView {

    private long mNativeMojoViewport;
    private final SurfaceHolder.Callback mSurfaceCallback;
    private final PlatformImeControllerAndroid mImeController;

    @CalledByNative
    public static PlatformWindowAndroid createForActivity(
            long nativeViewport, long nativeImeController) {
        PlatformWindowAndroid rv = new PlatformWindowAndroid(nativeViewport, nativeImeController);
        ((Activity) ContextUtils.getApplicationContext()).setContentView(rv);
        return rv;
    }

    private PlatformWindowAndroid(long nativeViewport, long nativeImeController) {
        super(ContextUtils.getApplicationContext());

        setFocusable(true);
        setFocusableInTouchMode(true);

        mNativeMojoViewport = nativeViewport;
        assert mNativeMojoViewport != 0;

        final float density =
                ContextUtils.getApplicationContext().getResources().getDisplayMetrics().density;

        mSurfaceCallback = new SurfaceHolder.Callback() {
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                assert mNativeMojoViewport != 0;
                nativeSurfaceSetSize(mNativeMojoViewport, width, height, density);
            }

            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                assert mNativeMojoViewport != 0;
                nativeSurfaceCreated(mNativeMojoViewport, holder.getSurface(), density);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                assert mNativeMojoViewport != 0;
                nativeSurfaceDestroyed(mNativeMojoViewport);
            }
        };
        getHolder().addCallback(mSurfaceCallback);

        mImeController = new PlatformImeControllerAndroid(this, nativeImeController);
    }

    @CalledByNative
    public void detach() {
        getHolder().removeCallback(mSurfaceCallback);
        mNativeMojoViewport = 0;
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (visibility == View.VISIBLE) {
            requestFocusFromTouch();
            requestFocus();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int actionMasked = event.getActionMasked();
        if (actionMasked == MotionEvent.ACTION_POINTER_DOWN
                || actionMasked == MotionEvent.ACTION_POINTER_UP) {
            // Up/down events identify a single point.
            return notifyTouchEventAtIndex(event, event.getActionIndex());
        }
        assert event.getPointerCount() != 0;
        // All other types can have more than one point.
        boolean result = false;
        for (int i = 0, count = event.getPointerCount(); i < count; i++) {
            final boolean sub_result = notifyTouchEventAtIndex(event, i);
            result |= sub_result;
        }
        return result;
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return mImeController.isTextEditorType();
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mImeController.onCreateInputConnection(outAttrs);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (privateDispatchKeyEvent(event)) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchKeyEventPreIme(KeyEvent event) {
        if (privateDispatchKeyEvent(event)) {
            return true;
        }
        return super.dispatchKeyEventPreIme(event);
    }

    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event) {
        if (privateDispatchKeyEvent(event)) {
            return true;
        }
        return super.dispatchKeyShortcutEvent(event);
    }

    private boolean notifyTouchEventAtIndex(MotionEvent event, int index) {
        float touchMajor = event.getTouchMajor(index);
        float touchMinor = event.getTouchMinor(index);
        if (touchMajor < touchMinor) {
            float tmp = touchMajor;
            touchMajor = touchMinor;
            touchMinor = tmp;
        }

        return nativeTouchEvent(mNativeMojoViewport, event.getEventTime(), event.getActionMasked(),
                event.getPointerId(index), event.getX(index), event.getY(index),
                event.getPressure(index), touchMajor, touchMinor,
                event.getOrientation(index), event.getAxisValue(MotionEvent.AXIS_HSCROLL, index),
                event.getAxisValue(MotionEvent.AXIS_VSCROLL, index));
    }

    private boolean privateDispatchKeyEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_MULTIPLE) {
            boolean result = false;
            if (event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN && event.getCharacters() != null) {
                String characters = event.getCharacters();
                for (int i = 0; i < characters.length(); ++i) {
                    char c = characters.charAt(i);
                    int codepoint = c;
                    if (codepoint >= Character.MIN_SURROGATE
                            && codepoint < (Character.MAX_SURROGATE + 1)) {
                        i++;
                        char c2 = characters.charAt(i);
                        codepoint = Character.toCodePoint(c, c2);
                    }
                    result |= nativeKeyEvent(mNativeMojoViewport, true, 0, codepoint);
                    result |= nativeKeyEvent(mNativeMojoViewport, false, 0, codepoint);
                }
            } else {
                for (int i = 0; i < event.getRepeatCount(); ++i) {
                    result |= nativeKeyEvent(
                            mNativeMojoViewport, true, event.getKeyCode(), event.getUnicodeChar());
                    result |= nativeKeyEvent(
                            mNativeMojoViewport, false, event.getKeyCode(), event.getUnicodeChar());
                }
            }
            return result;
        } else {
            return nativeKeyEvent(mNativeMojoViewport, event.getAction() == KeyEvent.ACTION_DOWN,
                    event.getKeyCode(), event.getUnicodeChar());
        }
    }

    private static native void nativeDestroy(long nativePlatformWindowAndroid);

    private static native void nativeSurfaceCreated(
            long nativePlatformWindowAndroid, Surface surface, float devicePixelRatio);

    private static native void nativeSurfaceDestroyed(
            long nativePlatformWindowAndroid);

    private static native void nativeSurfaceSetSize(
            long nativePlatformWindowAndroid, int width, int height, float density);

    private static native boolean nativeTouchEvent(long nativePlatformWindowAndroid, long timeMs,
            int maskedAction, int pointerId, float x, float y, float pressure, float touchMajor,
            float touchMinor, float orientation, float hWheel, float vWheel);

    private static native boolean nativeKeyEvent(
            long nativePlatformWindowAndroid, boolean pressed, int keyCode, int unicodeCharacter);
}
