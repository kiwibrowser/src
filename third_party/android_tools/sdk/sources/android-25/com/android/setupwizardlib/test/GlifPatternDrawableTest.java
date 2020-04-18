/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.setupwizardlib.test;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.os.Debug;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import com.android.setupwizardlib.GlifPatternDrawable;

import junit.framework.AssertionFailedError;

public class GlifPatternDrawableTest extends AndroidTestCase {

    private static final String TAG = "GlifPatternDrawableTest";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        GlifPatternDrawable.invalidatePattern();
    }

    @SmallTest
    public void testDraw() {
        final Bitmap bitmap = Bitmap.createBitmap(1366, 768, Bitmap.Config.ARGB_8888);
        final Canvas canvas = new Canvas(bitmap);

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 1366, 768);
        drawable.draw(canvas);

        assertSameColor("Top left pixel should be #e61a1a", 0xffe61a1a, bitmap.getPixel(0, 0));
        assertSameColor("Center pixel should be #d90d0d", 0xffd90d0d, bitmap.getPixel(683, 384));
        assertSameColor("Bottom right pixel should be #d40808", 0xffd40808,
                bitmap.getPixel(1365, 767));
    }

    @SmallTest
    public void testDrawTwice() {
        // Test that the second time the drawable is drawn is also correct, to make sure caching is
        // done correctly.

        final Bitmap bitmap = Bitmap.createBitmap(1366, 768, Bitmap.Config.ARGB_8888);
        final Canvas canvas = new Canvas(bitmap);

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 1366, 768);
        drawable.draw(canvas);

        Paint paint = new Paint();
        paint.setColor(Color.WHITE);
        canvas.drawRect(0, 0, 1366, 768, paint);  // Erase the entire canvas

        drawable.draw(canvas);

        assertSameColor("Top left pixel should be #e61a1a", 0xffe61a1a, bitmap.getPixel(0, 0));
        assertSameColor("Center pixel should be #d90d0d", 0xffd90d0d, bitmap.getPixel(683, 384));
        assertSameColor("Bottom right pixel should be #d40808", 0xffd40808,
                bitmap.getPixel(1365, 767));
    }

    @SmallTest
    public void testScaleToCanvasSquare() {
        final Canvas canvas = new Canvas();
        Matrix expected = new Matrix(canvas.getMatrix());

        Bitmap mockBitmapCache = Bitmap.createBitmap(1366, 768, Bitmap.Config.ALPHA_8);

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 683, 384);  // half each side of the view box
        drawable.scaleCanvasToBounds(canvas, mockBitmapCache, drawable.getBounds());

        expected.postScale(0.5f, 0.5f);

        assertEquals("Matrices should match", expected, canvas.getMatrix());
    }

    @SmallTest
    public void testScaleToCanvasTall() {
        final Canvas canvas = new Canvas();
        final Matrix expected = new Matrix(canvas.getMatrix());

        Bitmap mockBitmapCache = Bitmap.createBitmap(1366, 768, Bitmap.Config.ALPHA_8);

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 683, 768);  // half the width only
        drawable.scaleCanvasToBounds(canvas, mockBitmapCache, drawable.getBounds());

        expected.postScale(1f, 1f);
        expected.postTranslate(-99.718f, 0f);

        assertEquals("Matrices should match", expected, canvas.getMatrix());
    }

    @SmallTest
    public void testScaleToCanvasWide() {
        final Canvas canvas = new Canvas();
        final Matrix expected = new Matrix(canvas.getMatrix());

        Bitmap mockBitmapCache = Bitmap.createBitmap(1366, 768, Bitmap.Config.ALPHA_8);

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 1366, 384);  // half the height only
        drawable.scaleCanvasToBounds(canvas, mockBitmapCache, drawable.getBounds());

        expected.postScale(1f, 1f);
        expected.postTranslate(0f, -87.552f);

        assertEquals("Matrices should match", expected, canvas.getMatrix());
    }

    @SmallTest
    public void testScaleToCanvasMaxSize() {
        final Canvas canvas = new Canvas();
        final Matrix expected = new Matrix(canvas.getMatrix());

        Bitmap mockBitmapCache = Bitmap.createBitmap(2049, 1152, Bitmap.Config.ALPHA_8);

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 1366, 768);  // original viewbox size
        drawable.scaleCanvasToBounds(canvas, mockBitmapCache, drawable.getBounds());

        expected.postScale(1 / 1.5f, 1 / 1.5f);
        expected.postTranslate(0f, 0f);

        assertEquals("Matrices should match", expected, canvas.getMatrix());
    }

    @SmallTest
    public void testMemoryAllocation() {
        Debug.MemoryInfo memoryInfo = new Debug.MemoryInfo();
        Debug.getMemoryInfo(memoryInfo);
        final long memoryBefore = memoryInfo.getTotalPss();  // Get memory usage in KB

        final GlifPatternDrawable drawable = new GlifPatternDrawable(Color.RED);
        drawable.setBounds(0, 0, 1366, 768);
        drawable.createBitmapCache(2049, 1152);

        Debug.getMemoryInfo(memoryInfo);
        final long memoryAfter = memoryInfo.getTotalPss();
        Log.i(TAG, "Memory allocated for bitmap cache: " + (memoryAfter - memoryBefore));
        assertTrue("Memory allocation should not exceed 5MB", memoryAfter < memoryBefore + 5000);
    }

    private void assertSameColor(String message, int expected, int actual) {
        try {
            assertEquals(expected, actual);
        } catch (AssertionFailedError e) {
            throw new AssertionFailedError(message + " expected <#" + Integer.toHexString(expected)
                    + "> but found <#" + Integer.toHexString(actual) + "> instead");
        }
    }
}
