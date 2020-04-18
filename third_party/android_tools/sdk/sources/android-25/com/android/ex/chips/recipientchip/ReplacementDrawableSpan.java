package com.android.ex.chips.recipientchip;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.text.style.ReplacementSpan;

/**
 * ReplacementSpan that properly draws the drawable that is centered around the text
 * without changing the default text size or layout.
 */
public class ReplacementDrawableSpan extends ReplacementSpan {
    protected static final Paint sWorkPaint = new Paint();

    protected Drawable mDrawable;
    private float mExtraMargin;

    public ReplacementDrawableSpan(Drawable drawable) {
        super();
        mDrawable = drawable;
    }

    public void setExtraMargin(float margin) {
        mExtraMargin = margin;
    }

    private void setupFontMetrics(Paint.FontMetricsInt fm, Paint paint) {
        sWorkPaint.set(paint);
        if (fm != null) {
            sWorkPaint.getFontMetricsInt(fm);

            final Rect bounds = getBounds();
            final int textHeight = fm.descent - fm.ascent;
            final int halfMargin = (int) mExtraMargin / 2;
            fm.ascent = Math.min(fm.top, fm.top + (textHeight - bounds.bottom) / 2) - halfMargin;
            fm.descent = Math.max(fm.bottom, fm.bottom + (bounds.bottom - textHeight) / 2)
                    + halfMargin;
            fm.top = fm.ascent;
            fm.bottom = fm.descent;
        }
    }

    @Override
    public int getSize(Paint paint, CharSequence text, int i, int i2, Paint.FontMetricsInt fm) {
        setupFontMetrics(fm, paint);
        return getBounds().right;
    }

    @Override
    public void draw(Canvas canvas, CharSequence charSequence, int start, int end, float x, int top,
                     int y, int bottom, Paint paint) {
        canvas.save();
        int transY = (bottom - mDrawable.getBounds().bottom + top) / 2;
        canvas.translate(x, transY);
        mDrawable.draw(canvas);
        canvas.restore();
    }

    protected Rect getBounds() {
        return mDrawable.getBounds();
    }
}
