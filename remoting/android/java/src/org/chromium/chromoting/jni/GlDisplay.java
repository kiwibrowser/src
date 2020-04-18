// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting.jni;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.view.Surface;
import android.view.SurfaceHolder;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chromoting.DesktopView;
import org.chromium.chromoting.Event;
import org.chromium.chromoting.InputFeedbackRadiusMapper;
import org.chromium.chromoting.RenderStub;
import org.chromium.chromoting.SizeChangedEventParameter;

/**
 * This class is a RenderStub implementation that uses the OpenGL renderer in native code to render
 * the remote desktop. The lifetime of this class is managed by the native JniGlDisplayHandler.
 *
 * This class works entirely on the UI thread:
 *  All functions, including callbacks from native code are called only on UI thread.
 */
@JNINamespace("remoting")
public class GlDisplay implements SurfaceHolder.Callback, RenderStub {
    private final Event.Raisable<SizeChangedEventParameter> mOnClientSizeChanged =
            new Event.Raisable<>();
    private final Event.Raisable<SizeChangedEventParameter> mOnHostSizeChanged =
            new Event.Raisable<>();
    private final Event.Raisable<Void> mOnCanvasRendered = new Event.Raisable<>();

    private long mNativeJniGlDisplay;
    private InputFeedbackRadiusMapper mFeedbackRadiusMapper;
    private float mScaleFactor = 0;

    private GlDisplay(long nativeJniGlDisplay) {
        mNativeJniGlDisplay = nativeJniGlDisplay;
    }

    /**
     * Invalidates this object and disconnects from the native display handler.
     */
    @CalledByNative
    private void invalidate() {
        mNativeJniGlDisplay = 0;
    }

    /**
     * Notifies the OpenGL renderer that a surface for OpenGL to draw is created.
     * @param holder the surface holder that holds the surface.
     */
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mNativeJniGlDisplay != 0) {
            nativeOnSurfaceCreated(mNativeJniGlDisplay, holder.getSurface());
        }
    }

    /**
     * Notifies the OpenGL renderer the size of the surface. Should be called after surfaceCreated()
     * and before surfaceDestroyed().
     * @param width the width of the surface
     * @param height the height of the surface
     */
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mOnClientSizeChanged.raise(new SizeChangedEventParameter(width, height));
        if (mNativeJniGlDisplay != 0) {
            nativeOnSurfaceChanged(mNativeJniGlDisplay, width, height);
        }
    }

    /**
     * Notifies the OpenGL renderer that the current surface being used is about to be destroyed.
     */
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (mNativeJniGlDisplay != 0) {
            nativeOnSurfaceDestroyed(mNativeJniGlDisplay);
        }
    }

    @Override
    public void setDesktopView(DesktopView view) {
        view.getHolder().addCallback(this);
        mFeedbackRadiusMapper = new InputFeedbackRadiusMapper(view);
    }

    /**
     * Sets the transformation matrix (in pixel coordinates).
     * @param matrix the transformation matrix
     */
    @Override
    public void setTransformation(Matrix matrix) {
        if (mNativeJniGlDisplay != 0) {
            float[] matrixArray = new float[9];
            matrix.getValues(matrixArray);
            nativeOnPixelTransformationChanged(mNativeJniGlDisplay, matrixArray);
            mScaleFactor = matrix.mapRadius(1);
        }
    }

    /** Moves the cursor to the corresponding location on the desktop. */
    @Override
    public void moveCursor(PointF position) {
        if (mNativeJniGlDisplay != 0) {
            nativeOnCursorPixelPositionChanged(mNativeJniGlDisplay, position.x, position.y);
        }
    }

    /**
     * Decides whether the cursor should be shown on the canvas.
     */
    @Override
    public void setCursorVisibility(boolean visible) {
        if (mNativeJniGlDisplay != 0) {
            nativeOnCursorVisibilityChanged(mNativeJniGlDisplay, visible);
        }
    }

    /**
     * Shows the cursor input feedback animation with the given diameter at the given desktop
     * location.
     */
    @Override
    public void showInputFeedback(InputFeedbackType feedbackToShow, PointF pos) {
        if (mNativeJniGlDisplay == 0 || feedbackToShow.equals(InputFeedbackType.NONE)) {
            return;
        }
        float diameter = mFeedbackRadiusMapper
                .getFeedbackRadius(feedbackToShow, mScaleFactor) * 2.0f;
        if (diameter <= 0.0f) {
            return;
        }
        nativeOnCursorInputFeedback(mNativeJniGlDisplay, pos.x, pos.y, diameter);
    }

    @Override
    public Event<SizeChangedEventParameter> onClientSizeChanged() {
        return mOnClientSizeChanged;
    }

    @Override
    public Event<SizeChangedEventParameter> onHostSizeChanged() {
        return mOnHostSizeChanged;
    }

    @Override
    public Event<Void> onCanvasRendered() {
        return mOnCanvasRendered;
    }

    /**
     * Called by native code to notify GlDisplay that the size of the canvas (=size of desktop) has
     * changed.
     * @param width width of the canvas
     * @param height height of the canvas
     */
    @CalledByNative
    private void changeCanvasSize(int width, int height) {
        mOnHostSizeChanged.raise(new SizeChangedEventParameter(width, height));
    }

    /**
     * Called by native code when a render request has been done by the OpenGL renderer. This
     * will only be called when the render event callback is enabled.
     */
    @CalledByNative
    private void canvasRendered() {
        mOnCanvasRendered.raise(null);
    }

    @CalledByNative
    private void initializeClient(Client client) {
        client.setRenderStub(this);
    }

    @CalledByNative
    private static GlDisplay createJavaDisplayObject(long nativeDisplayHandler) {
        return new GlDisplay(nativeDisplayHandler);
    }

    private native void nativeOnSurfaceCreated(long nativeJniGlDisplayHandler, Surface surface);
    private native void nativeOnSurfaceChanged(long nativeJniGlDisplayHandler,
                                               int width, int height);
    private native void nativeOnSurfaceDestroyed(long nativeJniGlDisplayHandler);
    private native void nativeOnPixelTransformationChanged(long nativeJniGlDisplayHandler,
                                                           float[] matrix);
    private native void nativeOnCursorPixelPositionChanged(long nativeJniGlDisplayHandler,
                                                           float x, float y);
    private native void nativeOnCursorInputFeedback(long nativeJniGlDisplayHandler,
                                                    float x, float y, float diameter);
    private native void nativeOnCursorVisibilityChanged(long nativeJniGlDisplayHandler,
                                                        boolean visible);
}
