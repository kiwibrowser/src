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

import static com.google.common.truth.Truth.assertThat;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import java.lang.ref.WeakReference;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link RoundedCornerImageView}. */
@RunWith(RobolectricTestRunner.class)
public class RoundedCornerImageViewTest {

  private RoundedCornerImageView view;

  @Before
  public void setUp() {
    view = new RoundedCornerImageView(Robolectric.setupActivity(Activity.class));
  }

  @Test
  public void testSetImageDrawable_withColor() {
    ColorDrawable drawable = new ColorDrawable(Color.RED);

    view.setImageDrawable(drawable);

    view.setRoundedCorners(
        RoundedCornerViewHelper.TOP_CORNERS | RoundedCornerViewHelper.BOTTOM_CORNERS);
    view.setCornerRadius(5);
    assertThat(view.roundedRectangle).isNotNull();

    // No shader should be created
    assertThat(view.shader).isNull();
  }

  @Test
  public void testSetImageDrawable_invalidDrawable() {
    Drawable drawable = new InsetDrawable(new ColorDrawable(Color.RED), 4);

    view.setImageDrawable(drawable);

    view.setRoundedCorners(
        RoundedCornerViewHelper.TOP_CORNERS | RoundedCornerViewHelper.BOTTOM_CORNERS);
    view.setCornerRadius(5);
    assertThat(view.roundedRectangle).isNotNull();

    // No shader should be created
    assertThat(view.shader).isNull();
  }

  @Test
  public void testSetImageDrawable_getBitmapReturnsNullDoesNotNPE() {
    BitmapDrawable drawable = new BitmapDrawable();

    view.setImageDrawable(drawable);
    view.setRoundedCorners(
        RoundedCornerViewHelper.TOP_CORNERS | RoundedCornerViewHelper.BOTTOM_CORNERS);
    view.setCornerRadius(5);
    assertThat(view.roundedRectangle).isNotNull();

    // No shader should be created
    assertThat(view.shader).isNull();
  }

  @Test
  public void testSetImageDrawable_nullHoldsNoResources() {
    BitmapDrawable drawable = new BitmapDrawable(Bitmap.createBitmap(4, 4, Bitmap.Config.RGB_565));
    WeakReference<Drawable> weakDrawable = new WeakReference<>(drawable);
    view.setImageDrawable(drawable);

    view.setRoundedCorners(
        RoundedCornerViewHelper.TOP_CORNERS | RoundedCornerViewHelper.BOTTOM_CORNERS);
    view.setCornerRadius(5);
    assertThat(view.shader).isNotNull();
    view.setImageDrawable(null);
    drawable = null;

    System.gc();
    assertThat(view.shader).isNull();
    assertThat(view.getDrawable()).isNull();
    assertThat(weakDrawable.get()).isNull();
  }
}
