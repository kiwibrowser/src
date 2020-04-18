// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.piet.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.graphics.drawable.shapes.Shape;
import android.support.annotation.VisibleForTesting;
import android.util.AttributeSet;
import android.widget.ImageView;

/** {@link ImageView} which is able to show colors and bitmaps with rounded off the corners. */
public class RoundedCornerImageView extends ImageView {

  /*@Nullable*/ @VisibleForTesting Shape roundedRectangle;
  /*@Nullable*/ @VisibleForTesting BitmapShader shader;
  /*@Nullable*/ private Paint paint;
  private float radius;
  private int cornerField;

  // Whether or not to apply the shader, if we have one. This might be set to false if the image
  // is smaller than the view and does not need to have the corners rounded.
  private boolean applyShader;

  public RoundedCornerImageView(Context context) {
    this(context, null, 0);
  }

  public RoundedCornerImageView(Context context, /*@Nullable*/ AttributeSet attrs) {
    this(context, attrs, 0);
  }

  // Nullness checker doesn't like null attrs in super call but this is valid.
  @SuppressWarnings("initialization")
  public RoundedCornerImageView(Context context, /*@Nullable*/ AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    roundedRectangle = null;
    paint = null;
  }

  private void createRoundedCornerMask(float radius, int cornerField) {
    // If we don't have any radius values, don't bother creating the mask.
    if (radius > 0) {
      float[] radii = RoundedCornerViewHelper.createRoundedCornerMask(radius, cornerField);
      roundedRectangle = new RoundRectShape(radii, null, null);
      paint = new Paint();
      paint.setAntiAlias(true);
      maybeCreateShader();
    } else {
      roundedRectangle = null;
      paint = null;
    }
  }

  /**
   * Updates the rounded corner, using the radius set in the layout.
   *
   * @param roundedCorners Bitmask from {@link RoundedCorners.getBitmask()}
   */
  public void setRoundedCorners(int roundedCorners) {
    if (cornerField == roundedCorners) {
      return;
    }
    cornerField = roundedCorners;
    createRoundedCornerMask(radius, roundedCorners);

    // Request a re-draw.
    invalidate();
  }

  @Override
  public void setImageDrawable(/*@Nullable*/ Drawable drawable) {
    super.setImageDrawable(drawable);

    // Reset shaders.  We will need to recalculate them.
    shader = null;
    applyShader = false;

    maybeCreateShader();

    updateApplyShader();
  }

  protected void maybeCreateShader() {
    // Only create the shader if we have a rectangle to use as a mask.
    Drawable drawable = getDrawable();
    Bitmap bitmap =
        (drawable instanceof BitmapDrawable) ? ((BitmapDrawable) drawable).getBitmap() : null;
    if (roundedRectangle != null && bitmap != null) {
      shader = new BitmapShader(bitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP);
    }
  }

  @Override
  protected boolean setFrame(int l, int t, int r, int b) {
    boolean changed = super.setFrame(l, t, r, b);
    updateApplyShader();
    return changed;
  }

  @Override
  public void setScaleType(ScaleType scaleType) {
    super.setScaleType(scaleType);
    updateApplyShader();
  }

  /**
   * Updates the flag to tell whether or not to apply the shader that produces the rounded corners.
   * We should not apply the shader if the final image is smaller than the view, because it will try
   * to tile the image, which is not desirable. This should be called when the image is changed, or
   * the view bounds change.
   */
  private void updateApplyShader() {
    Drawable drawable = getDrawable();
    if ((drawable == null)
        || !(drawable instanceof BitmapDrawable)
        || (shader == null)
        || (paint == null)) {
      // In this state we wouldn't use the shader anyway.
      applyShader = false;
      return;
    }

    // Default to using the shader.
    applyShader = true;

    // If the scale type would not scale up the image, and the image is smaller than the
    // view bounds, then just draw it normally so the shader won't have adverse effects.
    // CENTER does not do any scaling, and simply centers the image. In that case we need to
    // check to see if the image is smaller than the view in either dimension, and don't apply
    // the shader if it is. CENTER_INSIDE will only scale down, so we need to calculate the
    // scaled size of the image, and only apply the shader if it happens to match the size of
    // the view.
    // TODO: this won't work with a custom image matrix, but that's probably ok for now
    ScaleType scaleType = getScaleType();
    if (scaleType == ScaleType.CENTER || scaleType == ScaleType.CENTER_INSIDE) {
      int viewWidth = getWidth() - getPaddingRight() - getPaddingLeft();
      int viewHeight = getHeight() - getPaddingTop() - getPaddingBottom();
      int drawableWidth = drawable.getIntrinsicWidth();
      int drawableHeight = drawable.getIntrinsicHeight();
      if (scaleType == ScaleType.CENTER_INSIDE) {
        float scale =
            Math.min(
                (float) viewWidth / (float) drawableWidth,
                (float) viewHeight / (float) drawableHeight);
        drawableWidth = (int) ((scale * drawableWidth) + 0.5f);
        drawableHeight = (int) ((scale * drawableHeight) + 0.5f);
      }
      if ((drawableWidth < viewWidth) || (drawableHeight < viewHeight)) {
        applyShader = false;
      }
    }
  }

  @Override
  public void onDraw(Canvas canvas) {
    Drawable drawable = getDrawable();
    Shape localRoundedRect = roundedRectangle;
    Paint localPaint = paint;
    if (drawable == null || localPaint == null || localRoundedRect == null) {
      super.onDraw(canvas);
      return;
    }

    if (!isSupportedDrawable(drawable)) {
      super.onDraw(canvas);
      return;
    }

    if (drawable instanceof ColorDrawable) {
      ColorDrawable colorDrawable = (ColorDrawable) drawable;
      localPaint.setColor(colorDrawable.getColor());
    }

    if (shader != null && applyShader) {
      shader.setLocalMatrix(getImageMatrix());
      localPaint.setShader(shader);
    }

    // Paint contains the configuration we need.
    // TODO: This does not properly handle padding.  Padding will not be taken into account
    // when rounded corners are used. We will fix this as part of implemented rounded corners on all
    // views.
    localRoundedRect.resize(getWidth(), getHeight());
    localRoundedRect.draw(canvas, localPaint);
  }

  public void setCornerRadius(int radius) {
    if (this.radius == radius) {
      return;
    }
    this.radius = radius;
    createRoundedCornerMask(this.radius, cornerField);
    invalidate();
  }

  public float getCornerRadius() {
    return radius;
  }

  private boolean isSupportedDrawable(Drawable drawable) {
    return (drawable instanceof ColorDrawable) || (drawable instanceof BitmapDrawable);
  }
}
