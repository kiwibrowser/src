// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Path.Direction;
import android.graphics.PointF;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region.Op;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.SystemClock;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Interpolator;
import android.view.animation.OvershootInterpolator;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.content.R;

/**
 * {@link View} class that implements tap disambiguation UI.
 */
public class PopupZoomer extends View {
    private static final String TAG = "cr.PopupZoomer";

    // The padding between the edges of the view and the popup. Note that there is a mirror
    // constant in content/renderer/render_view_impl.cc which should be kept in sync if
    // this is changed.
    private static final int ZOOM_BOUNDS_MARGIN = 25;
    // Time it takes for the animation to finish in ms.
    private static final long ANIMATION_DURATION = 300;

    // Note that these values should be cross-checked against
    // tools/metrics/histograms/histograms.xml.  Values should only be appended,
    // not changed or removed.
    private static final String UMA_TAPDISAMBIGUATION = "Touchscreen.TapDisambiguation";
    private static final int UMA_TAPDISAMBIGUATION_OTHER = 0;
    private static final int UMA_TAPDISAMBIGUATION_BACKBUTTON = 1;
    private static final int UMA_TAPDISAMBIGUATION_TAPPEDOUTSIDE = 2;
    private static final int UMA_TAPDISAMBIGUATION_TAPPEDINSIDE_DEPRECATED = 3;
    private static final int UMA_TAPDISAMBIGUATION_TAPPEDINSIDE_SAMENODE = 4;
    private static final int UMA_TAPDISAMBIGUATION_TAPPEDINSIDE_DIFFERENTNODE = 5;
    private static final int UMA_TAPDISAMBIGUATION_COUNT = 6;

    private void recordHistogram(int value) {
        RecordHistogram.recordEnumeratedHistogram(
                UMA_TAPDISAMBIGUATION, value, UMA_TAPDISAMBIGUATION_COUNT);
    }

    /**
     * Interface to be implemented to listen for touch events inside the zoomed area.
     */
    public static interface OnTapListener {
        public void onResolveTapDisambiguation(long timeMs, float x, float y, boolean isLongPress);
    }

    private final OnTapListener mOnTapListener;

    /**
     * Interface to be implemented to add and remove PopupZoomer to/from the view hierarchy.
     */
    public static interface OnVisibilityChangedListener {
        public void onPopupZoomerShown(PopupZoomer zoomer);
        public void onPopupZoomerHidden(PopupZoomer zoomer);
    }

    private final OnVisibilityChangedListener mOnVisibilityChangedListener;

    // Cached drawable used to frame the zooming popup.
    // TODO(tonyg): This should be marked purgeable so that if the system wants to recover this
    // memory, we can just reload it from the resource ID next time it is needed.
    // See android.graphics.BitmapFactory.Options#inPurgeable
    private static Drawable sOverlayDrawable;
    // The padding used for drawing the overlay around the content, instead of directly above it.
    private static Rect sOverlayPadding;
    // The radius of the overlay bubble, used for rounding the bitmap to draw underneath it.
    private static float sOverlayCornerRadius;

    private final Interpolator mShowInterpolator = new OvershootInterpolator();
    private final Interpolator mHideInterpolator = new ReverseInterpolator(mShowInterpolator);

    private boolean mAnimating;
    private boolean mShowing;
    private long mAnimationStartTime;

    // The time that was left for the outwards animation to finish.
    // This is used in the case that the zoomer is cancelled while it is still animating outwards,
    // to avoid having it jump to full size then animate closed.
    private long mTimeLeft;

    // initDimensions() needs to be called in onDraw().
    private boolean mNeedsToInitDimensions;

    // Available view area after accounting for ZOOM_BOUNDS_MARGIN.
    private RectF mViewClipRect;

    // The target rect to be zoomed.
    private Rect mTargetBounds;

    // The bitmap to hold the zoomed view.
    private Bitmap mZoomedBitmap;

    // How far to shift the canvas after all zooming is done, to keep it inside the bounds of the
    // view (including margin).
    private float mShiftX, mShiftY;
    // The magnification factor of the popup. It is recomputed once we have mTargetBounds and
    // mZoomedBitmap.
    private float mScale = 1.0f;
    // The bounds representing the actual zoomed popup.
    private RectF mClipRect;
    // The extrusion values are how far the zoomed area (mClipRect) extends from the touch point.
    // These values to used to animate the popup.
    private float mLeftExtrusion, mTopExtrusion, mRightExtrusion, mBottomExtrusion;
    // The last touch point, where the animation will start from.
    private final PointF mTouch = new PointF();

    // Since we sometimes overflow the bounds of the mViewClipRect, we need to allow scrolling.
    // Current scroll position.
    private float mPopupScrollX, mPopupScrollY;
    // Scroll bounds.
    private float mMinScrollX, mMaxScrollX;
    private float mMinScrollY, mMaxScrollY;

    private final GestureDetector mGestureDetector;

    // These bounds are computed and valid for one execution of onDraw.
    // Extracted to a member variable to save unnecessary allocations on each invocation.
    private RectF mDrawRect;

    private static float getOverlayCornerRadius(Context context) {
        if (sOverlayCornerRadius == 0) {
            try {
                sOverlayCornerRadius = context.getResources().getDimension(
                        R.dimen.link_preview_overlay_radius);
            } catch (Resources.NotFoundException e) {
                Log.w(TAG, "No corner radius resource for PopupZoomer overlay found.");
                sOverlayCornerRadius = 1.0f;
            }
        }
        return sOverlayCornerRadius;
    }

    /**
     * Gets the drawable that should be used to frame the zooming popup, loading
     * it from the resource bundle if not already cached.
     */
    private static Drawable getOverlayDrawable(Context context) {
        if (sOverlayDrawable == null) {
            try {
                sOverlayDrawable = ApiCompatibilityUtils.getDrawable(context.getResources(),
                        R.drawable.ondemand_overlay);
            } catch (Resources.NotFoundException e) {
                Log.w(TAG, "No drawable resource for PopupZoomer overlay found.");
                sOverlayDrawable = new ColorDrawable();
            }
            sOverlayPadding = new Rect();
            sOverlayDrawable.getPadding(sOverlayPadding);
        }
        return sOverlayDrawable;
    }

    private static float constrain(float amount, float low, float high) {
        return amount < low ? low : (amount > high ? high : amount);
    }

    private static int constrain(int amount, int low, int high) {
        return amount < low ? low : (amount > high ? high : amount);
    }

    /**
     * Creates Popupzoomer.
     * @param context Context to be used.
     * @param containerView view that popup zoomer gets added to.
     * @param visibilityListener {@link OnVisibilityChangedListener} listener.
     * @param tapListener {@link OnTapListener} listener.
     */
    public PopupZoomer(Context context, ViewGroup containerView,
            OnVisibilityChangedListener visibilityListener, OnTapListener tapListener) {
        super(context);

        mOnVisibilityChangedListener = visibilityListener;
        mOnTapListener = tapListener;

        setVisibility(INVISIBLE);
        setFocusable(true);
        setFocusableInTouchMode(true);

        GestureDetector.SimpleOnGestureListener listener =
                new GestureDetector.SimpleOnGestureListener() {
                    @Override
                    public boolean onScroll(MotionEvent e1, MotionEvent e2,
                            float distanceX, float distanceY) {
                        if (mAnimating) return true;

                        if (isTouchOutsideArea(e1.getX(), e1.getY())) {
                            tappedOutside();
                        } else {
                            scroll(distanceX, distanceY);
                        }
                        return true;
                    }

                    @Override
                    public boolean onSingleTapUp(MotionEvent e) {
                        return handleTapOrPress(e, false);
                    }

                    @Override
                    public void onLongPress(MotionEvent e) {
                        handleTapOrPress(e, true);
                    }

                    private boolean handleTapOrPress(MotionEvent e, boolean isLongPress) {
                        if (mAnimating) return true;

                        float x = e.getX();
                        float y = e.getY();
                        if (isTouchOutsideArea(x, y)) {
                            tappedOutside();
                        } else if (mOnTapListener != null) {
                            PointF converted = convertTouchPoint(x, y);
                            mOnTapListener.onResolveTapDisambiguation(
                                    e.getEventTime(), converted.x, converted.y, isLongPress);
                            tappedInside();
                        }
                        return true;
                    }
                };
        mGestureDetector = new GestureDetector(context, listener);
    }

    /**
     * Sets the bitmap to be used for the zoomed view.
     */
    @VisibleForTesting
    void setBitmap(Bitmap bitmap) {
        if (mZoomedBitmap != null) {
            mZoomedBitmap.recycle();
            mZoomedBitmap = null;
        }
        mZoomedBitmap = bitmap;

        // Round the corners of the bitmap so it doesn't stick out around the overlay.
        Canvas canvas = new Canvas(mZoomedBitmap);
        Path path = new Path();
        RectF canvasRect = new RectF(0, 0, canvas.getWidth(), canvas.getHeight());
        float overlayCornerRadius = getOverlayCornerRadius(getContext());
        path.addRoundRect(canvasRect, overlayCornerRadius, overlayCornerRadius, Direction.CCW);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            canvas.clipOutPath(path);
        } else {
            canvas.clipPath(path, Op.DIFFERENCE);
        }
        Paint clearPaint = new Paint();
        clearPaint.setXfermode(new PorterDuffXfermode(Mode.SRC));
        clearPaint.setColor(Color.TRANSPARENT);
        canvas.drawPaint(clearPaint);
    }

    private void scroll(float x, float y) {
        mPopupScrollX = constrain(mPopupScrollX - x, mMinScrollX, mMaxScrollX);
        mPopupScrollY = constrain(mPopupScrollY - y, mMinScrollY, mMaxScrollY);
        invalidate();
    }

    private void startAnimation(boolean show) {
        mAnimating = true;
        mShowing = show;
        mTimeLeft = 0;
        if (show) {
            setVisibility(VISIBLE);
            mNeedsToInitDimensions = true;
            if (mOnVisibilityChangedListener != null) {
                mOnVisibilityChangedListener.onPopupZoomerShown(this);
            }
        } else {
            long endTime = mAnimationStartTime + ANIMATION_DURATION;
            mTimeLeft = endTime - SystemClock.uptimeMillis();
            if (mTimeLeft < 0) mTimeLeft = 0;
        }
        mAnimationStartTime = SystemClock.uptimeMillis();
        invalidate();
    }

    private void hideImmediately() {
        mAnimating = false;
        mShowing = false;
        mTimeLeft = 0;
        if (mOnVisibilityChangedListener != null) {
            mOnVisibilityChangedListener.onPopupZoomerHidden(this);
        }
        setVisibility(INVISIBLE);
        mZoomedBitmap.recycle();
        mZoomedBitmap = null;
    }

    /**
     * Returns true if the view is currently being shown (or is animating).
     */
    public boolean isShowing() {
        return mShowing || mAnimating;
    }

    /**
     * Sets the last touch point (on the unzoomed view).
     */
    public void setLastTouch(float x, float y) {
        mTouch.x = x;
        mTouch.y = y;
    }

    private void setTargetBounds(Rect rect) {
        mTargetBounds = rect;
    }

    private void initDimensions() {
        if (mTargetBounds == null || mTouch == null) return;

        // Compute the final zoom scale.
        mScale = (float) mZoomedBitmap.getWidth() / mTargetBounds.width();

        float l = mTouch.x - mScale * (mTouch.x - mTargetBounds.left);
        float t = mTouch.y - mScale * (mTouch.y - mTargetBounds.top);
        float r = l + mZoomedBitmap.getWidth();
        float b = t + mZoomedBitmap.getHeight();
        mClipRect = new RectF(l, t, r, b);
        int width = getWidth();
        int height = getHeight();

        mViewClipRect = new RectF(ZOOM_BOUNDS_MARGIN,
                ZOOM_BOUNDS_MARGIN,
                width - ZOOM_BOUNDS_MARGIN,
                height - ZOOM_BOUNDS_MARGIN);

        // Ensure it stays inside the bounds of the view.  First shift it around to see if it
        // can fully fit in the view, then clip it to the padding section of the view to
        // ensure no overflow.
        mShiftX = 0;
        mShiftY = 0;

        // Right now this has the happy coincidence of showing the leftmost portion
        // of a scaled up bitmap, which usually has the text in it.  When we want to support
        // RTL languages, we can conditionally switch the order of this check to push it
        // to the left instead of right.
        if (mClipRect.left < ZOOM_BOUNDS_MARGIN) {
            mShiftX = ZOOM_BOUNDS_MARGIN - mClipRect.left;
            mClipRect.left += mShiftX;
            mClipRect.right += mShiftX;
        } else if (mClipRect.right > width - ZOOM_BOUNDS_MARGIN) {
            mShiftX = (width - ZOOM_BOUNDS_MARGIN - mClipRect.right);
            mClipRect.right += mShiftX;
            mClipRect.left += mShiftX;
        }
        if (mClipRect.top < ZOOM_BOUNDS_MARGIN) {
            mShiftY = ZOOM_BOUNDS_MARGIN - mClipRect.top;
            mClipRect.top += mShiftY;
            mClipRect.bottom += mShiftY;
        } else if (mClipRect.bottom > height - ZOOM_BOUNDS_MARGIN) {
            mShiftY = height - ZOOM_BOUNDS_MARGIN - mClipRect.bottom;
            mClipRect.bottom += mShiftY;
            mClipRect.top += mShiftY;
        }

        // Allow enough scrolling to get to the entire bitmap that may be clipped inside the
        // bounds of the view.
        mMinScrollX = mMaxScrollX = mMinScrollY = mMaxScrollY = 0;
        if (mViewClipRect.right + mShiftX < mClipRect.right) {
            mMinScrollX = mViewClipRect.right - mClipRect.right;
        }
        if (mViewClipRect.left + mShiftX > mClipRect.left) {
            mMaxScrollX = mViewClipRect.left - mClipRect.left;
        }
        if (mViewClipRect.top + mShiftY > mClipRect.top) {
            mMaxScrollY = mViewClipRect.top - mClipRect.top;
        }
        if (mViewClipRect.bottom + mShiftY < mClipRect.bottom) {
            mMinScrollY = mViewClipRect.bottom - mClipRect.bottom;
        }
        // Now that we know how much we need to scroll, we can intersect with mViewClipRect.
        mClipRect.intersect(mViewClipRect);

        mLeftExtrusion = mTouch.x - mClipRect.left;
        mRightExtrusion = mClipRect.right - mTouch.x;
        mTopExtrusion = mTouch.y - mClipRect.top;
        mBottomExtrusion = mClipRect.bottom - mTouch.y;

        // Set an initial scroll position to take touch point into account.
        float percentX =
                (mTouch.x - mTargetBounds.centerX()) / (mTargetBounds.width() / 2.f) + .5f;
        float percentY =
                (mTouch.y - mTargetBounds.centerY()) / (mTargetBounds.height() / 2.f) + .5f;

        float scrollWidth = mMaxScrollX - mMinScrollX;
        float scrollHeight = mMaxScrollY - mMinScrollY;
        mPopupScrollX = scrollWidth * percentX * -1f;
        mPopupScrollY = scrollHeight * percentY * -1f;
        // Constrain initial scroll position within allowed bounds.
        mPopupScrollX = constrain(mPopupScrollX, mMinScrollX, mMaxScrollX);
        mPopupScrollY = constrain(mPopupScrollY, mMinScrollY, mMaxScrollY);

        // Compute the bounds in onDraw()
        mDrawRect = new RectF();
    }

    /*
     * Tests override it as the PopupZoomer is never attached to the view hierarchy.
     */
    protected boolean acceptZeroSizeView() {
        return false;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (!isShowing() || mZoomedBitmap == null) return;
        if (!acceptZeroSizeView() && (getWidth() == 0 || getHeight() == 0)) return;

        if (mNeedsToInitDimensions) {
            mNeedsToInitDimensions = false;
            initDimensions();
        }

        canvas.save();
        // Calculate the elapsed fraction of animation.
        float time = (SystemClock.uptimeMillis() - mAnimationStartTime + mTimeLeft)
                / ((float) ANIMATION_DURATION);
        time = constrain(time, 0, 1);
        if (time >= 1) {
            mAnimating = false;
            if (!isShowing()) {
                hideImmediately();
                return;
            }
        } else {
            invalidate();
        }

        // Fraction of the animation to actally show.
        float fractionAnimation;
        if (mShowing) {
            fractionAnimation = mShowInterpolator.getInterpolation(time);
        } else {
            fractionAnimation = mHideInterpolator.getInterpolation(time);
        }

        // Draw a faded color over the entire view to fade out the original content, increasing
        // the alpha value as fractionAnimation increases.
        // TODO(nileshagrawal): We should use time here instead of fractionAnimation
        // as fractionAnimaton is interpolated and can go over 1.
        canvas.drawARGB((int) (80 * fractionAnimation), 0, 0, 0);
        canvas.save();

        // Since we want the content to appear directly above its counterpart we need to make
        // sure that it starts out at exactly the same size as it appears in the page,
        // i.e. scale grows from 1/mScale to 1. Note that extrusion values are already zoomed
        // with mScale.
        float scale = fractionAnimation * (mScale - 1.0f) / mScale + 1.0f / mScale;

        // Since we want the content to appear directly above its counterpart on the
        // page, we need to remove the mShiftX/Y effect at the beginning of the animation.
        // The unshifting decreases with the animation.
        float unshiftX = -mShiftX * (1.0f - fractionAnimation) / mScale;
        float unshiftY = -mShiftY * (1.0f - fractionAnimation) / mScale;

        // Compute the |mDrawRect| to show.
        mDrawRect.left = mTouch.x - mLeftExtrusion * scale + unshiftX;
        mDrawRect.top = mTouch.y - mTopExtrusion * scale + unshiftY;
        mDrawRect.right = mTouch.x + mRightExtrusion * scale + unshiftX;
        mDrawRect.bottom = mTouch.y + mBottomExtrusion * scale + unshiftY;
        canvas.clipRect(mDrawRect);

        // Since the canvas transform APIs all pre-concat the transformations, this is done in
        // reverse order. The canvas is first scaled up, then shifted the appropriate amount of
        // pixels.
        canvas.scale(scale, scale, mDrawRect.left, mDrawRect.top);
        canvas.translate(mPopupScrollX, mPopupScrollY);
        canvas.drawBitmap(mZoomedBitmap, mDrawRect.left, mDrawRect.top, null);
        canvas.restore();
        Drawable overlayNineTile = getOverlayDrawable(getContext());
        overlayNineTile.setBounds((int) mDrawRect.left - sOverlayPadding.left,
                (int) mDrawRect.top - sOverlayPadding.top,
                (int) mDrawRect.right + sOverlayPadding.right,
                (int) mDrawRect.bottom + sOverlayPadding.bottom);
        // TODO(nileshagrawal): We should use time here instead of fractionAnimation
        // as fractionAnimaton is interpolated and can go over 1.
        int alpha = constrain((int) (fractionAnimation * 255), 0, 255);
        overlayNineTile.setAlpha(alpha);
        overlayNineTile.draw(canvas);
        canvas.restore();
    }

    /**
     * Show the PopupZoomer view with given target bounds.
     */
    @VisibleForTesting
    void show(Rect rect) {
        if (mShowing || mZoomedBitmap == null) return;

        setTargetBounds(rect);
        startAnimation(true);
    }

    /**
     * Hide the PopupZoomer view because of some external event such as focus
     * change, JS-originating scroll, etc.
     * @param animation true if hide with animation.
     */
    public void hide(boolean animation) {
        if (!mShowing) return;
        recordHistogram(UMA_TAPDISAMBIGUATION_OTHER);

        if (animation) {
            startAnimation(false);
        } else {
            hideImmediately();
        }
    }

    private void tappedInside() {
        if (!mShowing) return;
        // Tapped-inside histogram value is recorded on the renderer side,
        // not here.

        startAnimation(false);
    }

    private void tappedOutside() {
        if (!mShowing) return;
        recordHistogram(UMA_TAPDISAMBIGUATION_TAPPEDOUTSIDE);

        startAnimation(false);
    }

    public void backButtonPressed() {
        if (!mShowing) return;
        recordHistogram(UMA_TAPDISAMBIGUATION_BACKBUTTON);

        startAnimation(false);
    }

    /**
     * Converts the coordinates to a point on the original un-zoomed view.
     */
    private PointF convertTouchPoint(float x, float y) {
        x -= mShiftX;
        y -= mShiftY;
        x = mTouch.x + (x - mTouch.x - mPopupScrollX) / mScale;
        y = mTouch.y + (y - mTouch.y - mPopupScrollY) / mScale;
        return new PointF(x, y);
    }

    /**
     * Returns true if the point is inside the final drawable area for this popup zoomer.
     */
    private boolean isTouchOutsideArea(float x, float y) {
        return !mClipRect.contains(x, y);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        mGestureDetector.onTouchEvent(event);
        return true;
    }

    private static class ReverseInterpolator implements Interpolator {
        private final Interpolator mInterpolator;

        public ReverseInterpolator(Interpolator i) {
            mInterpolator = i;
        }

        @Override
        public float getInterpolation(float input) {
            input = 1.0f - input;
            if (mInterpolator == null) return input;
            return mInterpolator.getInterpolation(input);
        }
    }
}
