/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.support.rastermill;

import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.os.SystemClock;

public class FrameSequenceDrawable extends Drawable implements Animatable, Runnable {
    /**
     * These constants are chosen to imitate common browser behavior for WebP/GIF.
     * If other decoders are added, this behavior should be moved into the WebP/GIF decoders.
     *
     * Note that 0 delay is undefined behavior in the GIF standard.
     */
    private static final long MIN_DELAY_MS = 20;
    private static final long DEFAULT_DELAY_MS = 100;

    private static final Object sLock = new Object();
    private static HandlerThread sDecodingThread;
    private static Handler sDecodingThreadHandler;
    private static void initializeDecodingThread() {
        synchronized (sLock) {
            if (sDecodingThread != null) return;

            sDecodingThread = new HandlerThread("FrameSequence decoding thread",
                    Process.THREAD_PRIORITY_BACKGROUND);
            sDecodingThread.start();
            sDecodingThreadHandler = new Handler(sDecodingThread.getLooper());
        }
    }

    public static interface OnFinishedListener {
        /**
         * Called when a FrameSequenceDrawable has finished looping.
         *
         * Note that this is will not be called if the drawable is explicitly
         * stopped, or marked invisible.
         */
        public abstract void onFinished(FrameSequenceDrawable drawable);
    }

    public static interface BitmapProvider {
        /**
         * Called by FrameSequenceDrawable to aquire an 8888 Bitmap with minimum dimensions.
         */
        public abstract Bitmap acquireBitmap(int minWidth, int minHeight);

        /**
         * Called by FrameSequenceDrawable to release a Bitmap it no longer needs. The Bitmap
         * will no longer be used at all by the drawable, so it is safe to reuse elsewhere.
         *
         * This method may be called by FrameSequenceDrawable on any thread.
         */
        public abstract void releaseBitmap(Bitmap bitmap);
    }

    private static BitmapProvider sAllocatingBitmapProvider = new BitmapProvider() {
        @Override
        public Bitmap acquireBitmap(int minWidth, int minHeight) {
            return Bitmap.createBitmap(minWidth, minHeight, Bitmap.Config.ARGB_8888);
        }

        @Override
        public void releaseBitmap(Bitmap bitmap) {}
    };

    /**
     * Register a callback to be invoked when a FrameSequenceDrawable finishes looping.
     *
     * @see #setLoopBehavior(int)
     */
    public void setOnFinishedListener(OnFinishedListener onFinishedListener) {
        mOnFinishedListener = onFinishedListener;
    }

    /**
     * Loop only once.
     */
    public static final int LOOP_ONCE = 1;

    /**
     * Loop continuously. The OnFinishedListener will never be called.
     */
    public static final int LOOP_INF = 2;

    /**
     * Use loop count stored in source data, or LOOP_ONCE if not present.
     */
    public static final int LOOP_DEFAULT = 3;

    /**
     * Define looping behavior of frame sequence.
     *
     * Must be one of LOOP_ONCE, LOOP_INF, or LOOP_DEFAULT
     */
    public void setLoopBehavior(int loopBehavior) {
        mLoopBehavior = loopBehavior;
    }

    private final FrameSequence mFrameSequence;
    private final FrameSequence.State mFrameSequenceState;

    private final Paint mPaint;
    private BitmapShader mFrontBitmapShader;
    private BitmapShader mBackBitmapShader;
     private final Rect mSrcRect;
    private boolean mCircleMaskEnabled;

    //Protects the fields below
    private final Object mLock = new Object();

    private final BitmapProvider mBitmapProvider;
    private boolean mDestroyed = false;
    private Bitmap mFrontBitmap;
    private Bitmap mBackBitmap;

    private static final int STATE_SCHEDULED = 1;
    private static final int STATE_DECODING = 2;
    private static final int STATE_WAITING_TO_SWAP = 3;
    private static final int STATE_READY_TO_SWAP = 4;

    private int mState;
    private int mCurrentLoop;
    private int mLoopBehavior = LOOP_DEFAULT;

    private long mLastSwap;
    private long mNextSwap;
    private int mNextFrameToDecode;
    private OnFinishedListener mOnFinishedListener;

    /**
     * Runs on decoding thread, only modifies mBackBitmap's pixels
     */
    private Runnable mDecodeRunnable = new Runnable() {
        @Override
        public void run() {
            int nextFrame;
            Bitmap bitmap;
            synchronized (mLock) {
                if (mDestroyed) return;

                nextFrame = mNextFrameToDecode;
                if (nextFrame < 0) {
                    return;
                }
                bitmap = mBackBitmap;
                mState = STATE_DECODING;
            }
            int lastFrame = nextFrame - 2;
            long invalidateTimeMs = mFrameSequenceState.getFrame(nextFrame, bitmap, lastFrame);

            if (invalidateTimeMs < MIN_DELAY_MS) {
                invalidateTimeMs = DEFAULT_DELAY_MS;
            }

            boolean schedule = false;
            Bitmap bitmapToRelease = null;
            synchronized (mLock) {
                if (mDestroyed) {
                    bitmapToRelease = mBackBitmap;
                    mBackBitmap = null;
                } else if (mNextFrameToDecode >= 0 && mState == STATE_DECODING) {
                    schedule = true;
                    mNextSwap = invalidateTimeMs + mLastSwap;
                    mState = STATE_WAITING_TO_SWAP;
                }
            }
            if (schedule) {
                scheduleSelf(FrameSequenceDrawable.this, mNextSwap);
            }
            if (bitmapToRelease != null) {
                // destroy the bitmap here, since there's no safe way to get back to
                // drawable thread - drawable is likely detached, so schedule is noop.
                mBitmapProvider.releaseBitmap(bitmapToRelease);
            }
        }
    };

    private Runnable mCallbackRunnable = new Runnable() {
        @Override
        public void run() {
            if (mOnFinishedListener != null) {
                mOnFinishedListener.onFinished(FrameSequenceDrawable.this);
            }
        }
    };

    private static Bitmap acquireAndValidateBitmap(BitmapProvider bitmapProvider,
            int minWidth, int minHeight) {
        Bitmap bitmap = bitmapProvider.acquireBitmap(minWidth, minHeight);

        if (bitmap.getWidth() < minWidth
                || bitmap.getHeight() < minHeight
                || bitmap.getConfig() != Bitmap.Config.ARGB_8888) {
            throw new IllegalArgumentException("Invalid bitmap provided");
        }

        return bitmap;
    }

    public FrameSequenceDrawable(FrameSequence frameSequence) {
        this(frameSequence, sAllocatingBitmapProvider);
    }

    public FrameSequenceDrawable(FrameSequence frameSequence, BitmapProvider bitmapProvider) {
        if (frameSequence == null || bitmapProvider == null) throw new IllegalArgumentException();

        mFrameSequence = frameSequence;
        mFrameSequenceState = frameSequence.createState();
        final int width = frameSequence.getWidth();
        final int height = frameSequence.getHeight();

        mBitmapProvider = bitmapProvider;
        mFrontBitmap = acquireAndValidateBitmap(bitmapProvider, width, height);
        mBackBitmap = acquireAndValidateBitmap(bitmapProvider, width, height);
        mSrcRect = new Rect(0, 0, width, height);
        mPaint = new Paint();
        mPaint.setFilterBitmap(true);

        mFrontBitmapShader
            = new BitmapShader(mFrontBitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP);
        mBackBitmapShader
            = new BitmapShader(mBackBitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP);

        mLastSwap = 0;

        mNextFrameToDecode = -1;
        mFrameSequenceState.getFrame(0, mFrontBitmap, -1);
        initializeDecodingThread();
    }

    /**
     * Pass true to mask the shape of the animated drawing content to a circle.
     *
     * <p> The masking circle will be the largest circle contained in the Drawable's bounds.
     * Masking is done with BitmapShader, incurring minimal additional draw cost.
     */
    public final void setCircleMaskEnabled(boolean circleMaskEnabled) {
        mCircleMaskEnabled = circleMaskEnabled;
        // Anti alias only necessary when using circular mask
        mPaint.setAntiAlias(circleMaskEnabled);
    }

    private void checkDestroyedLocked() {
        if (mDestroyed) {
            throw new IllegalStateException("Cannot perform operation on recycled drawable");
        }
    }

    public boolean isDestroyed() {
        synchronized (mLock) {
            return mDestroyed;
        }
    }

    /**
     * Marks the drawable as permanently recycled (and thus unusable), and releases any owned
     * Bitmaps drawable to its BitmapProvider, if attached.
     *
     * If no BitmapProvider is attached to the drawable, recycle() is called on the Bitmaps.
     */
    public void destroy() {
        if (mBitmapProvider == null) {
            throw new IllegalStateException("BitmapProvider must be non-null");
        }

        Bitmap bitmapToReleaseA;
        Bitmap bitmapToReleaseB = null;
        synchronized (mLock) {
            checkDestroyedLocked();

            bitmapToReleaseA = mFrontBitmap;
            mFrontBitmap = null;

            if (mState != STATE_DECODING) {
                bitmapToReleaseB = mBackBitmap;
                mBackBitmap = null;
            }

            mDestroyed = true;
        }

        // For simplicity and safety, we don't destroy the state object here
        mBitmapProvider.releaseBitmap(bitmapToReleaseA);
        if (bitmapToReleaseB != null) {
            mBitmapProvider.releaseBitmap(bitmapToReleaseB);
        }
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            mFrameSequenceState.destroy();
        } finally {
            super.finalize();
        }
    }

    @Override
    public void draw(Canvas canvas) {
        synchronized (mLock) {
            checkDestroyedLocked();
            if (mState == STATE_WAITING_TO_SWAP) {
                // may have failed to schedule mark ready runnable,
                // so go ahead and swap if swapping is due
                if (mNextSwap - SystemClock.uptimeMillis() <= 0) {
                    mState = STATE_READY_TO_SWAP;
                }
            }

            if (isRunning() && mState == STATE_READY_TO_SWAP) {
                // Because draw has occurred, the view system is guaranteed to no longer hold a
                // reference to the old mFrontBitmap, so we now use it to produce the next frame
                Bitmap tmp = mBackBitmap;
                mBackBitmap = mFrontBitmap;
                mFrontBitmap = tmp;

                BitmapShader tmpShader = mBackBitmapShader;
                mBackBitmapShader = mFrontBitmapShader;
                mFrontBitmapShader = tmpShader;

                mLastSwap = SystemClock.uptimeMillis();

                boolean continueLooping = true;
                if (mNextFrameToDecode == mFrameSequence.getFrameCount() - 1) {
                    mCurrentLoop++;
                    if ((mLoopBehavior == LOOP_ONCE && mCurrentLoop == 1) ||
                            (mLoopBehavior == LOOP_DEFAULT && mCurrentLoop == mFrameSequence.getDefaultLoopCount())) {
                        continueLooping = false;
                    }
                }

                if (continueLooping) {
                    scheduleDecodeLocked();
                } else {
                    scheduleSelf(mCallbackRunnable, 0);
                }
            }
        }

        if (mCircleMaskEnabled) {
            Rect bounds = getBounds();
            mPaint.setShader(mFrontBitmapShader);
            float width = bounds.width();
            float height = bounds.height();
            float circleRadius = (Math.min(width, height)) / 2f;
            canvas.drawCircle(width / 2f, height / 2f, circleRadius, mPaint);
        } else {
            mPaint.setShader(null);
            canvas.drawBitmap(mFrontBitmap, mSrcRect, getBounds(), mPaint);
        }
    }

    private void scheduleDecodeLocked() {
        mState = STATE_SCHEDULED;
        mNextFrameToDecode = (mNextFrameToDecode + 1) % mFrameSequence.getFrameCount();
        sDecodingThreadHandler.post(mDecodeRunnable);
    }

    @Override
    public void run() {
        // set ready to swap as necessary
        boolean invalidate = false;
        synchronized (mLock) {
            if (mNextFrameToDecode >= 0 && mState == STATE_WAITING_TO_SWAP) {
                mState = STATE_READY_TO_SWAP;
                invalidate = true;
            }
        }
        if (invalidate) {
            invalidateSelf();
        }
    }

    @Override
    public void start() {
        if (!isRunning()) {
            synchronized (mLock) {
                checkDestroyedLocked();
                if (mState == STATE_SCHEDULED) return; // already scheduled
                mCurrentLoop = 0;
                scheduleDecodeLocked();
            }
        }
    }

    @Override
    public void stop() {
        if (isRunning()) {
            unscheduleSelf(this);
        }
    }

    @Override
    public boolean isRunning() {
        synchronized (mLock) {
            return mNextFrameToDecode > -1 && !mDestroyed;
        }
    }

    @Override
    public void unscheduleSelf(Runnable what) {
        synchronized (mLock) {
            mNextFrameToDecode = -1;
            mState = 0;
        }
        super.unscheduleSelf(what);
    }

    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        boolean changed = super.setVisible(visible, restart);

        if (!visible) {
            stop();
        } else if (restart || changed) {
            stop();
            start();
        }

        return changed;
    }

    // drawing properties

    @Override
    public void setFilterBitmap(boolean filter) {
        mPaint.setFilterBitmap(filter);
    }

    @Override
    public void setAlpha(int alpha) {
        mPaint.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(ColorFilter colorFilter) {
        mPaint.setColorFilter(colorFilter);
    }

    @Override
    public int getIntrinsicWidth() {
        return mFrameSequence.getWidth();
    }

    @Override
    public int getIntrinsicHeight() {
        return mFrameSequence.getHeight();
    }

    @Override
    public int getOpacity() {
        return mFrameSequence.isOpaque() ? PixelFormat.OPAQUE : PixelFormat.TRANSPARENT;
    }
}
