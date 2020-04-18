// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.support.annotation.IntDef;
import android.util.AttributeSet;
import android.widget.ScrollView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * An extension of the ScrollView that supports edge boundaries coming in.
 */
public class FadingEdgeScrollView extends ScrollView {
    /** Draw no lines at all. */
    public static final int DRAW_NO_EDGE = 0;

    /** Draw an edge that fades in, depending on how much is left to scroll. */
    public static final int DRAW_FADING_EDGE = 1;

    /** Draw either no line (if there is nothing to scroll) or a fully opaque line. */
    public static final int DRAW_HARD_EDGE = 2;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({DRAW_NO_EDGE, DRAW_FADING_EDGE, DRAW_HARD_EDGE})
    public @interface EdgeType {}

    private static final int POSITION_TOP = 0;
    private static final int POSITION_BOTTOM = 1;

    private final Paint mSeparatorPaint = new Paint();
    private final int mSeparatorColor;
    private final int mSeparatorHeight;

    @EdgeType
    private int mDrawTopEdge = DRAW_FADING_EDGE;
    @EdgeType
    private int mDrawBottomEdge = DRAW_FADING_EDGE;

    public FadingEdgeScrollView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mSeparatorColor =
                ApiCompatibilityUtils.getColor(getResources(), R.color.toolbar_shadow_color);
        mSeparatorHeight = getResources().getDimensionPixelSize(R.dimen.separator_height);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        super.dispatchDraw(canvas);
        setVerticalFadingEdgeEnabled(true);
        float topEdgeStrength = getTopFadingEdgeStrength();
        float bottomEdgeStrength = getBottomFadingEdgeStrength();
        setVerticalFadingEdgeEnabled(false);

        drawBoundaryLine(canvas, POSITION_TOP, topEdgeStrength, mDrawTopEdge);
        drawBoundaryLine(canvas, POSITION_BOTTOM, bottomEdgeStrength, mDrawBottomEdge);
    }

    /**
     * Sets which edge should be drawn.
     * @param topEdgeType    Whether to draw the edge on the top part of the view.
     * @param bottomEdgeType Whether to draw the edge on the bottom part of the view.
     */
    public void setEdgeVisibility(@EdgeType int topEdgeType, @EdgeType int bottomEdgeType) {
        mDrawTopEdge = topEdgeType;
        mDrawBottomEdge = bottomEdgeType;
        invalidate();
    }

    /**
     * Draws a line at the top or bottom of the view. This should be called from dispatchDraw() so
     * it gets drawn on top of the View's children.
     *
     * @param canvas       The canvas on which to draw.
     * @param position     Where to draw the line: either POSITION_TOP or POSITION_BOTTOM.
     * @param edgeStrength A value between 0 and 1 indicating the relative size of the line. 0
     *                     means no line at all. 1 means a fully opaque line.
     * @param edgeType     How to draw the line.
     */
    private void drawBoundaryLine(
            Canvas canvas, int position, float edgeStrength, @EdgeType int edgeType) {
        if (edgeType == DRAW_NO_EDGE) {
            return;
        } else if (edgeType == DRAW_FADING_EDGE) {
            edgeStrength = Math.max(0.0f, Math.min(1.0f, edgeStrength));
        } else {
            edgeStrength = 1.0f;
        }
        if (edgeStrength <= 0.0f) return;

        int adjustedA = (int) (Color.alpha(mSeparatorColor) * edgeStrength);
        int adjustedR = (int) (Color.red(mSeparatorColor) * edgeStrength);
        int adjustedG = (int) (Color.green(mSeparatorColor) * edgeStrength);
        int adjustedB = (int) (Color.blue(mSeparatorColor) * edgeStrength);
        mSeparatorPaint.setColor(Color.argb(adjustedA, adjustedR, adjustedG, adjustedB));

        int left = getScrollX();
        int right = left + getRight();

        if (position == POSITION_BOTTOM) {
            int bottom = getScrollY() + getBottom() - getTop();
            canvas.drawRect(left, bottom - mSeparatorHeight, right, bottom, mSeparatorPaint);
        } else if (position == POSITION_TOP) {
            int top = getScrollY();
            canvas.drawRect(left, top, right, top + mSeparatorHeight, mSeparatorPaint);
        }
    }
}
