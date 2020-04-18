// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;
import android.view.LayoutInflater;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.widget.ImageView;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.ImageViewTinter.ImageViewTinterOwner;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Tests the classes that use ImageViewTinter.
 *
 * In an ideal world, these tests would simply use an XmlPullParser and XML that is defined inside
 * this test, but Android explicitly disallows that because it pre-processes the XML files:
 * https://developer.android.com/reference/android/view/LayoutInflater.html
 *
 * An alternative would be to have test-specific layout directories, but these don't seem to be
 * able to reference the instrumented package's resources.  Instead, the tests reference XML for
 * actual controls used in the production app.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ImageViewTinterTest {
    @Rule
    public UiThreadTestRule mRule = new UiThreadTestRule();

    private Context mContext;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mContext.setTheme(R.style.MainTheme);
    }

    @Test
    @SmallTest
    public void testTintedImageView_attributeParsingExplicitTint() throws Exception {
        // The tint is explicitly set to a blue in the XML.
        int color = ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.blue_mode_tint);
        TintedImageView clearStorageView = (TintedImageView) LayoutInflater.from(mContext).inflate(
                R.layout.clear_storage, null, false);
        Assert.assertNotNull(clearStorageView.getColorFilter());
        Assert.assertTrue(checkIfTintWasApplied(clearStorageView, color));
    }

    @Test
    @SmallTest
    public void testTintedImageButton_attributeParsingExplicitTint() throws Exception {
        // The tint was explicitly set to a color.
        int color = ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.dark_mode_tint);
        TintedImageButton colorTint =
                createImageView(R.layout.search_toolbar, R.id.clear_text_button);
        Assert.assertNotNull(colorTint.getColorFilter());
        Assert.assertTrue(checkIfTintWasApplied(colorTint, color));
    }

    @Test
    @SmallTest
    public void testTintedImageView_attributeParsingNullTint() throws Exception {
        // The tint is explicitly set to null in the XML.
        int color = ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.blue_mode_tint);
        TintedImageView nullTint = createImageView(R.layout.title_button_menu_item, R.id.checkbox);
        Assert.assertNull(nullTint.getColorFilter());
        Assert.assertFalse(checkIfTintWasApplied(nullTint, color));
    }

    @Test
    @SmallTest
    public void testTintedImageButton_attributeParsingNullTint() throws Exception {
        // The tint is explicitly set to null in the XML.  An image resource needs to be set here
        // because the layout doesn't define one by default.
        int color =
                ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.blue_when_enabled);
        TintedImageButton nullTint = createImageView(R.layout.title_button_menu_item, R.id.button);
        Assert.assertNull(nullTint.getColorFilter());
        nullTint.setImageResource(R.drawable.plus);
        Assert.assertFalse(checkIfTintWasApplied(nullTint, color));
    }

    @Test
    @SmallTest
    public void testTintedImageView_setTint() throws Exception {
        // The tint is explicitly set to null for this object in the XML.
        TintedImageView nullTint = createImageView(R.layout.title_button_menu_item, R.id.checkbox);
        checkSetTintWorksCorrectly(nullTint);
    }

    @Test
    @SmallTest
    public void testTintedImageButton_setTint() throws Exception {
        // The tint is explicitly set to null for this object in the XML.
        TintedImageButton nullTint = createImageView(R.layout.title_button_menu_item, R.id.button);
        checkSetTintWorksCorrectly(nullTint);
    }

    private void checkSetTintWorksCorrectly(ImageViewTinterOwner view) {
        ImageView imageView = (ImageView) view;
        int color =
                ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.light_active_color);

        Assert.assertNull(imageView.getColorFilter());
        if (imageView.getDrawable() == null) {
            // An image resource is set here in case the layout does not define one.
            imageView.setImageResource(R.drawable.plus);
        }
        Assert.assertFalse(checkIfTintWasApplied(view, color));

        // Set the tint to one color.
        ColorStateList colorList = ApiCompatibilityUtils.getColorStateList(
                mContext.getResources(), R.color.light_active_color);
        view.setTint(colorList);
        Assert.assertNotNull(imageView.getColorFilter());
        Assert.assertTrue(checkIfTintWasApplied(view, color));

        // Clear it out.
        view.setTint(null);
        Assert.assertNull(imageView.getColorFilter());
        Assert.assertFalse(checkIfTintWasApplied(view, color));

        // Set it to another color.
        int otherColor =
                ApiCompatibilityUtils.getColor(mContext.getResources(), R.color.google_red_700);
        ColorStateList otherColorList = ApiCompatibilityUtils.getColorStateList(
                mContext.getResources(), R.color.google_red_700);
        view.setTint(otherColorList);
        Assert.assertNotNull(imageView.getColorFilter());
        Assert.assertTrue(checkIfTintWasApplied(view, otherColor));
    }

    @SuppressWarnings("unchecked")
    private <T extends ImageView> T createImageView(int layoutId, int ownerId) {
        ViewGroup root = (ViewGroup) LayoutInflater.from(mContext).inflate(layoutId, null, false);
        return (T) root.findViewById(ownerId);
    }

    private boolean checkIfTintWasApplied(
            ImageViewTinterOwner imageViewTinterOwner, int expectedColor) {
        int unspecifiedSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        Assert.assertTrue(imageViewTinterOwner instanceof ImageView);
        ImageView imageView = (ImageView) imageViewTinterOwner;
        imageView.measure(unspecifiedSpec, unspecifiedSpec);
        imageView.layout(0, 0, imageView.getMeasuredWidth(), imageView.getMeasuredHeight());

        // Draw the ImageView into a Canvas so we can check that the tint was applied.
        Drawable drawable = imageView.getDrawable();
        Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        imageViewTinterOwner.onDraw(canvas);

        // Search for any pixel that is of the expected color.
        for (int x = 0; x < bitmap.getWidth(); x++) {
            for (int y = 0; y < bitmap.getHeight(); y++) {
                if (expectedColor == bitmap.getPixel(x, y)) return true;
            }
        }
        return false;
    }
}
