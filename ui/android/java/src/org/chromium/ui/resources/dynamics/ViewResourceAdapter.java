// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.resources.dynamics;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.view.View;
import android.view.View.OnLayoutChangeListener;

import org.chromium.base.TraceEvent;
import org.chromium.ui.resources.Resource;
import org.chromium.ui.resources.ResourceFactory;
import org.chromium.ui.resources.statics.NinePatchData;

/**
 * An adapter that exposes a {@link View} as a {@link DynamicResource}. In order to properly use
 * this adapter {@link ViewResourceAdapter#invalidate(Rect)} must be called when parts of the
 * {@link View} are invalidated.  For {@link ViewGroup}s the easiest way to do this is to override
 * {@link View#invalidateChildInParent(int[], Rect)}.
 */
public class ViewResourceAdapter implements DynamicResource, OnLayoutChangeListener {
    private final View mView;
    private final Rect mDirtyRect = new Rect();

    private Bitmap mBitmap;
    private Rect mBitmapSize = new Rect();

    /**
     * Builds a {@link ViewResourceAdapter} instance around {@code view}.
     * @param view The {@link View} to expose as a {@link Resource}.
     */
    public ViewResourceAdapter(View view) {
        mView = view;
        mView.addOnLayoutChangeListener(this);
    }

    /**
     * If this resource is not dirty ({@link #isDirty()} returned {@code false}), this will return
     * the last {@link Bitmap} built from the {@link View}.  Otherwise it will recapture a
     * {@link Bitmap} of the {@link View}.
     * @see {@link DynamicResource#getBitmap()}.
     * @return A {@link Bitmap} representing the {@link View}.
     */
    @Override
    public Bitmap getBitmap() {
        if (!isDirty()) return mBitmap;
        TraceEvent.begin("ViewResourceAdapter:getBitmap");
        if (validateBitmap()) {
            Canvas canvas = new Canvas(mBitmap);

            onCaptureStart(canvas, mDirtyRect.isEmpty() ? null : mDirtyRect);

            if (!mDirtyRect.isEmpty()) canvas.clipRect(mDirtyRect);
            capture(canvas);

            onCaptureEnd();
        } else {
            assert mBitmap.getWidth() == 1 && mBitmap.getHeight() == 1;
            mBitmap.setPixel(0, 0, Color.TRANSPARENT);
        }

        mDirtyRect.setEmpty();
        TraceEvent.end("ViewResourceAdapter:getBitmap");
        return mBitmap;
    }

    @Override
    public Rect getBitmapSize() {
        return mBitmapSize;
    }

    /**
     * Override this method to create the native resource type for the generated bitmap.
     */
    @Override
    public long createNativeResource() {
        return ResourceFactory.createBitmapResource(null);
    }

    @Override
    public final NinePatchData getNinePatchData() {
        return null;
    }

    @Override
    public boolean isDirty() {
        if (mBitmap == null) mDirtyRect.set(0, 0, mView.getWidth(), mView.getHeight());

        return !mDirtyRect.isEmpty();
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
            int oldTop, int oldRight, int oldBottom) {
        final int width = right - left;
        final int height = bottom - top;
        final int oldWidth = oldRight - oldLeft;
        final int oldHeight = oldBottom - oldTop;

        if (width != oldWidth || height != oldHeight) mDirtyRect.set(0, 0, width, height);
    }

    /**
     * Invalidates a particular region of the {@link View} that needs to be repainted.
     * @param dirtyRect The region to invalidate, or {@code null} if the entire {@code Bitmap}
     *                  should be redrawn.
     */
    public void invalidate(Rect dirtyRect) {
        if (dirtyRect == null) {
            mDirtyRect.set(0, 0, mView.getWidth(), mView.getHeight());
        } else {
            mDirtyRect.union(dirtyRect);
        }
    }

    /**
     * Drops the cached bitmap to free up memory.
     */
    public void dropCachedBitmap() {
        mBitmap = null;
    }

    /**
     * @return Dirty rect that will be drawn on capture.
     */
    protected Rect getDirtyRect() {
        return mDirtyRect;
    }

    /**
     * Called before {@link #capture(Canvas)} is called.
     * @param canvas    The {@link Canvas} that will be drawn to.
     * @param dirtyRect The dirty {@link Rect} or {@code null} if the entire area is being redrawn.
     */
    protected void onCaptureStart(Canvas canvas, Rect dirtyRect) {
    }

    /**
     * Called to draw the {@link View}'s contents into the passed in {@link Canvas}.
     * @param canvas The {@link Canvas} that will be drawn to.
     */
    protected void capture(Canvas canvas) {
        mView.draw(canvas);
    }

    /**
     * Called after {@link #capture(Canvas)}.
     */
    protected void onCaptureEnd() {
    }

    /**
     * @return Whether |mBitmap| is corresponding to |mView| or not.
     */
    private boolean validateBitmap() {
        int viewWidth = mView.getWidth();
        int viewHeight = mView.getHeight();
        boolean isEmpty = viewWidth == 0 || viewHeight == 0;
        if (isEmpty) {
            viewWidth = 1;
            viewHeight = 1;
        }
        if (mBitmap != null
                && (mBitmap.getWidth() != viewWidth || mBitmap.getHeight() != viewHeight)) {
            mBitmap.recycle();
            mBitmap = null;
        }

        if (mBitmap == null) {
            mBitmap = Bitmap.createBitmap(viewWidth, viewHeight, Bitmap.Config.ARGB_8888);
            mBitmap.setHasAlpha(true);
            mDirtyRect.set(0, 0, viewWidth, viewHeight);
            mBitmapSize.set(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
        }

        return !isEmpty;
    }
}
