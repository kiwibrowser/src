// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.shell.DrawGL;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.WebContents;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * A View used for testing the AwContents internals.
 *
 * This class takes the place android.webkit.WebView would have in the production configuration.
 */
public class AwTestContainerView extends FrameLayout {
    private AwContents mAwContents;
    private AwContents.InternalAccessDelegate mInternalAccessDelegate;

    private HardwareView mHardwareView;
    private boolean mAttachedContents;

    private class HardwareView extends GLSurfaceView {
        private static final int MODE_DRAW = 0;
        private static final int MODE_PROCESS = 1;
        private static final int MODE_PROCESS_NO_CONTEXT = 2;
        private static final int MODE_SYNC = 3;

        // mSyncLock is used to synchronized requestRender on the UI thread
        // and drawGL on the rendering thread. The variables following
        // are protected by it.
        private final Object mSyncLock = new Object();
        private boolean mFunctorAttached;
        private boolean mNeedsProcessGL;
        private boolean mNeedsDrawGL;
        private boolean mWaitForCompletion;
        private int mLastScrollX;
        private int mLastScrollY;

        // Only used by drawGL on render thread to store the value of scroll offsets at most recent
        // sync for subsequent draws.
        private int mCommittedScrollX;
        private int mCommittedScrollY;

        private boolean mHaveSurface;
        private Runnable mReadyToRenderCallback;
        private Runnable mReadyToDetachCallback;

        private long mDrawGL;
        private long mViewContext;

        public HardwareView(Context context) {
            super(context);
            setEGLContextClientVersion(2); // GLES2
            getHolder().setFormat(PixelFormat.OPAQUE);
            setPreserveEGLContextOnPause(true);
            setRenderer(new Renderer() {
                private int mWidth;
                private int mHeight;

                @Override
                public void onDrawFrame(GL10 gl) {
                    HardwareView.this.drawGL(mWidth, mHeight);
                }

                @Override
                public void onSurfaceChanged(GL10 gl, int width, int height) {
                    gl.glViewport(0, 0, width, height);
                    gl.glScissor(0, 0, width, height);
                    mWidth = width;
                    mHeight = height;
                }

                @Override
                public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                }
            });

            setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        }

        public void initialize(long drawGL) {
            mDrawGL = drawGL;
            mViewContext = 0;
        }

        public boolean isReadyToRender() {
            return mHaveSurface;
        }

        public void setReadyToRenderCallback(Runnable runner) {
            assert !isReadyToRender() || runner == null;
            mReadyToRenderCallback = runner;
        }

        public void setReadyToDetachCallback(Runnable runner) {
            mReadyToDetachCallback = runner;
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mHaveSurface = true;
            if (mReadyToRenderCallback != null) {
                mReadyToRenderCallback.run();
                mReadyToRenderCallback = null;
            }
            super.surfaceCreated(holder);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            mHaveSurface = false;
            if (mReadyToDetachCallback != null) {
                mReadyToDetachCallback.run();
                mReadyToDetachCallback = null;
            }
            super.surfaceDestroyed(holder);
        }

        public void updateScroll(int x, int y) {
            synchronized (mSyncLock) {
                mLastScrollX = x;
                mLastScrollY = y;
            }
        }

        public void detachGLFunctor() {
            synchronized (mSyncLock) {
                mFunctorAttached = false;
                mNeedsProcessGL = false;
                mNeedsDrawGL = false;
                mWaitForCompletion = false;
            }
        }

        public void requestRender(long viewContext, Canvas canvas, boolean waitForCompletion) {
            synchronized (mSyncLock) {
                assert viewContext != 0;
                mViewContext = viewContext;
                super.requestRender();
                mFunctorAttached = true;
                mWaitForCompletion = waitForCompletion;
                if (canvas == null) {
                    mNeedsProcessGL = true;
                } else {
                    mNeedsDrawGL = true;
                    if (!waitForCompletion) {
                        // Wait until SYNC is complete only.
                        // Do this every time there was a new frame.
                        try {
                            while (mNeedsDrawGL) {
                                mSyncLock.wait();
                            }
                        } catch (InterruptedException e) {
                            // ...
                        }
                    }
                }
                if (waitForCompletion) {
                    try {
                        while (mWaitForCompletion) {
                            mSyncLock.wait();
                        }
                    } catch (InterruptedException e) {
                        // ...
                    }
                }
            }
        }

        public void drawGL(int width, int height) {
            final boolean draw;
            final boolean process;
            final boolean waitForCompletion;
            final long viewContext;

            synchronized (mSyncLock) {
                if (!mFunctorAttached) {
                    mSyncLock.notifyAll();
                    return;
                }

                draw = mNeedsDrawGL;
                process = mNeedsProcessGL;
                waitForCompletion = mWaitForCompletion;
                viewContext = mViewContext;
                if (draw) {
                    DrawGL.drawGL(mDrawGL, viewContext, width, height, 0, 0, MODE_SYNC);
                    mCommittedScrollX = mLastScrollX;
                    mCommittedScrollY = mLastScrollY;
                }
                mNeedsDrawGL = false;
                mNeedsProcessGL = false;
                if (!waitForCompletion) {
                    mSyncLock.notifyAll();
                }
            }
            if (process) {
                DrawGL.drawGL(mDrawGL, viewContext, width, height, 0, 0, MODE_PROCESS);
            }
            if (process || draw) {
                DrawGL.drawGL(mDrawGL, viewContext, width, height, mCommittedScrollX,
                        mCommittedScrollY, MODE_DRAW);
            }

            if (waitForCompletion) {
                synchronized (mSyncLock) {
                    mWaitForCompletion = false;
                    mSyncLock.notifyAll();
                }
            }
        }
    }

    private static boolean sCreatedOnce;
    private HardwareView createHardwareViewOnlyOnce(Context context) {
        if (sCreatedOnce) return null;
        sCreatedOnce = true;
        return new HardwareView(context);
    }

    public AwTestContainerView(Context context, boolean allowHardwareAcceleration) {
        super(context);
        if (allowHardwareAcceleration) {
            mHardwareView = createHardwareViewOnlyOnce(context);
        }
        if (isBackedByHardwareView()) {
            addView(mHardwareView,
                    new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));
        } else {
            setLayerType(LAYER_TYPE_SOFTWARE, null);
        }
        mInternalAccessDelegate = new InternalAccessAdapter();
        setOverScrollMode(View.OVER_SCROLL_ALWAYS);
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    public void initialize(AwContents awContents) {
        mAwContents = awContents;
        if (isBackedByHardwareView()) {
            mHardwareView.initialize(AwContents.getAwDrawGLFunction());
        }
    }

    public boolean isBackedByHardwareView() {
        return mHardwareView != null;
    }

    public ContentViewCore getContentViewCore() {
        return mAwContents.getContentViewCore();
    }

    public WebContents getWebContents() {
        return mAwContents.getWebContents();
    }

    public AwContents getAwContents() {
        return mAwContents;
    }

    public AwContents.NativeDrawGLFunctorFactory getNativeDrawGLFunctorFactory() {
        return new NativeDrawGLFunctorFactory();
    }

    public AwContents.InternalAccessDelegate getInternalAccessDelegate() {
        return mInternalAccessDelegate;
    }

    public void destroy() {
        mAwContents.destroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mAwContents.onConfigurationChanged(newConfig);
    }

    private void attachedContentsInternal() {
        assert !mAttachedContents;
        mAwContents.onAttachedToWindow();
        mAttachedContents = true;
    }

    private void detachedContentsInternal() {
        assert mAttachedContents;
        mAwContents.onDetachedFromWindow();
        mAttachedContents = false;
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (mHardwareView == null || mHardwareView.isReadyToRender()) {
            attachedContentsInternal();
        } else {
            mHardwareView.setReadyToRenderCallback(() -> attachedContentsInternal());
        }

        if (mHardwareView != null) {
            mHardwareView.setReadyToDetachCallback(() -> detachedContentsInternal());
        }
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (mHardwareView == null || mHardwareView.isReadyToRender()) {
            detachedContentsInternal();

            if (mHardwareView != null) {
                mHardwareView.setReadyToRenderCallback(null);
                mHardwareView.setReadyToDetachCallback(null);
            }
        }
    }

    @Override
    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
        mAwContents.onFocusChanged(focused, direction, previouslyFocusedRect);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mAwContents.onCreateInputConnection(outAttrs);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return mAwContents.onKeyUp(keyCode, event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return mAwContents.dispatchKeyEvent(event);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        mAwContents.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    public void onSizeChanged(int w, int h, int ow, int oh) {
        super.onSizeChanged(w, h, ow, oh);
        mAwContents.onSizeChanged(w, h, ow, oh);
    }

    @Override
    public void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        mAwContents.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);
    }

    @Override
    public void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);
        if (mAwContents != null) {
            mAwContents.onContainerViewScrollChanged(l, t, oldl, oldt);
        }
    }

    @Override
    public void computeScroll() {
        mAwContents.computeScroll();
    }

    @Override
    public void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        mAwContents.onVisibilityChanged(changedView, visibility);
    }

    @Override
    public void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        mAwContents.onWindowVisibilityChanged(visibility);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        super.onTouchEvent(ev);
        return mAwContents.onTouchEvent(ev);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent ev) {
        super.onGenericMotionEvent(ev);
        return mAwContents.onGenericMotionEvent(ev);
    }

    @Override
    public boolean onHoverEvent(MotionEvent ev) {
        super.onHoverEvent(ev);
        return mAwContents.onHoverEvent(ev);
    }

    @Override
    public void onDraw(Canvas canvas) {
        if (isBackedByHardwareView()) {
            mHardwareView.updateScroll(getScrollX(), getScrollY());
        }
        mAwContents.onDraw(canvas);
        super.onDraw(canvas);
    }

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        AccessibilityNodeProvider provider =
                mAwContents.getAccessibilityNodeProvider();
        return provider == null ? super.getAccessibilityNodeProvider() : provider;
    }

    @Override
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        return mAwContents.performAccessibilityAction(action, arguments);
    }

    private class NativeDrawGLFunctorFactory implements AwContents.NativeDrawGLFunctorFactory {
        @Override
        public NativeDrawGLFunctor createFunctor(long context) {
            return new NativeDrawGLFunctor(context);
        }
    }

    private static final class NativeDrawGLFunctorDestroyRunnable implements Runnable {
        public long mContext;
        NativeDrawGLFunctorDestroyRunnable(long context) {
            mContext = context;
        }
        @Override
        public void run() {
            mContext = 0;
        }
    }

    private class NativeDrawGLFunctor implements AwContents.NativeDrawGLFunctor {
        private NativeDrawGLFunctorDestroyRunnable mDestroyRunnable;

        NativeDrawGLFunctor(long context) {
            mDestroyRunnable = new NativeDrawGLFunctorDestroyRunnable(context);
        }

        @Override
        public boolean supportsDrawGLFunctorReleasedCallback() {
            return false;
        }

        @Override
        public boolean requestDrawGL(Canvas canvas, Runnable releasedRunnable) {
            assert releasedRunnable == null;
            if (!isBackedByHardwareView()) return false;
            mHardwareView.requestRender(mDestroyRunnable.mContext, canvas, false);
            return true;
        }

        @Override
        public boolean requestInvokeGL(View containerView, boolean waitForCompletion) {
            if (!isBackedByHardwareView()) return false;
            mHardwareView.requestRender(mDestroyRunnable.mContext, null, waitForCompletion);
            return true;
        }

        @Override
        public void detach(View containerView) {
            if (isBackedByHardwareView()) mHardwareView.detachGLFunctor();
        }

        @Override
        public Runnable getDestroyRunnable() {
            return mDestroyRunnable;
        }
    }

    // TODO: AwContents could define a generic class that holds an implementation similar to
    // the one below.
    private class InternalAccessAdapter implements AwContents.InternalAccessDelegate {

        @Override
        public boolean super_onKeyUp(int keyCode, KeyEvent event) {
            return AwTestContainerView.super.onKeyUp(keyCode, event);
        }

        @Override
        public boolean super_dispatchKeyEvent(KeyEvent event) {
            return AwTestContainerView.super.dispatchKeyEvent(event);
        }

        @Override
        public boolean super_onGenericMotionEvent(MotionEvent event) {
            return AwTestContainerView.super.onGenericMotionEvent(event);
        }

        @Override
        public void super_onConfigurationChanged(Configuration newConfig) {
            AwTestContainerView.super.onConfigurationChanged(newConfig);
        }

        @Override
        public void super_scrollTo(int scrollX, int scrollY) {
            // We're intentionally not calling super.scrollTo here to make testing easier.
            AwTestContainerView.this.scrollTo(scrollX, scrollY);
            if (isBackedByHardwareView()) {
                // Undo the scroll that will be applied because of mHardwareView
                // being a child of |this|.
                mHardwareView.setTranslationX(scrollX);
                mHardwareView.setTranslationY(scrollY);
            }
        }

        @Override
        public void overScrollBy(int deltaX, int deltaY,
                int scrollX, int scrollY,
                int scrollRangeX, int scrollRangeY,
                int maxOverScrollX, int maxOverScrollY,
                boolean isTouchEvent) {
            // We're intentionally not calling super.scrollTo here to make testing easier.
            AwTestContainerView.this.overScrollBy(deltaX, deltaY, scrollX, scrollY,
                     scrollRangeX, scrollRangeY, maxOverScrollX, maxOverScrollY, isTouchEvent);
        }

        @Override
        public void onScrollChanged(int l, int t, int oldl, int oldt) {
            AwTestContainerView.super.onScrollChanged(l, t, oldl, oldt);
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            AwTestContainerView.super.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        @Override
        public int super_getScrollBarStyle() {
            return AwTestContainerView.super.getScrollBarStyle();
        }

        @Override
        public void super_startActivityForResult(Intent intent, int requestCode) {}
    }
}
