package com.android.ex.chips;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;
import android.widget.ImageView;

/**
 * An ImageView class with a circle mask so that all images are drawn in a
 * circle instead of a square.
 */
public class CircularImageView extends ImageView {
    private static float circularImageBorder = 1f;

    private final Matrix matrix;
    private final RectF source;
    private final RectF destination;
    private final Paint bitmapPaint;
    private final Paint borderPaint;

    public CircularImageView(Context context) {
        this(context, null, 0);
    }

    public CircularImageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CircularImageView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        matrix = new Matrix();
        source = new RectF();
        destination = new RectF();

        bitmapPaint = new Paint();
        bitmapPaint.setAntiAlias(true);
        bitmapPaint.setFilterBitmap(true);
        bitmapPaint.setDither(true);

        borderPaint = new Paint();
        borderPaint.setColor(Color.TRANSPARENT);
        borderPaint.setStyle(Paint.Style.STROKE);
        borderPaint.setStrokeWidth(circularImageBorder);
        borderPaint.setAntiAlias(true);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        Drawable drawable = getDrawable();
        BitmapDrawable bitmapDrawable = null;
        // support state list drawable by getting the current state
        if (drawable instanceof StateListDrawable) {
            if (((StateListDrawable) drawable).getCurrent() != null) {
                bitmapDrawable = (BitmapDrawable) drawable.getCurrent();
            }
        } else {
            bitmapDrawable = (BitmapDrawable) drawable;
        }

        if (bitmapDrawable == null) {
            return;
        }
        Bitmap bitmap = bitmapDrawable.getBitmap();
        if (bitmap == null) {
            return;
        }

        source.set(0, 0, bitmap.getWidth(), bitmap.getHeight());
        destination.set(getPaddingLeft(), getPaddingTop(), getWidth() - getPaddingRight(),
                getHeight() - getPaddingBottom());

        drawBitmapWithCircleOnCanvas(bitmap, canvas, source, destination);
    }

    /**
     * Given the source bitmap and a canvas, draws the bitmap through a circular
     * mask. Only draws a circle with diameter equal to the destination width.
     *
     * @param bitmap The source bitmap to draw.
     * @param canvas The canvas to draw it on.
     * @param source The source bound of the bitmap.
     * @param dest The destination bound on the canvas.
     */
    public void drawBitmapWithCircleOnCanvas(Bitmap bitmap, Canvas canvas,
                                             RectF source, RectF dest) {
        // Draw bitmap through shader first.
        BitmapShader shader = new BitmapShader(bitmap, Shader.TileMode.CLAMP,
                Shader.TileMode.CLAMP);
        matrix.reset();

        // Fit bitmap to bounds.
        matrix.setRectToRect(source, dest, Matrix.ScaleToFit.FILL);

        shader.setLocalMatrix(matrix);
        bitmapPaint.setShader(shader);
        canvas.drawCircle(dest.centerX(), dest.centerY(), dest.width() / 2f,
                bitmapPaint);

        // Then draw the border.
        canvas.drawCircle(dest.centerX(), dest.centerY(),
                dest.width() / 2f - circularImageBorder / 2, borderPaint);
    }
}