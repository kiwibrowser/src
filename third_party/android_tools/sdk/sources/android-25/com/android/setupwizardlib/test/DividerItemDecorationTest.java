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
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.View;
import android.view.ViewGroup;

import com.android.setupwizardlib.DividerItemDecoration;

public class DividerItemDecorationTest extends AndroidTestCase {

    @SmallTest
    public void testDivider() {
        final DividerItemDecoration decoration = new DividerItemDecoration();
        Drawable divider = new ColorDrawable();
        decoration.setDivider(divider);
        assertSame("Divider should be same as set", divider, decoration.getDivider());
    }

    @SmallTest
    public void testDividerHeight() {
        final DividerItemDecoration decoration = new DividerItemDecoration();
        decoration.setDividerHeight(123);
        assertEquals("Divider height should be 123", 123, decoration.getDividerHeight());
    }

    @SmallTest
    public void testShouldDrawDividerBelowWithEitherCondition() {
        // Set up the item decoration, with 1px red divider line
        final DividerItemDecoration decoration = new DividerItemDecoration();
        Drawable divider = new ColorDrawable(Color.RED);
        decoration.setDivider(divider);
        decoration.setDividerHeight(1);

        Bitmap bitmap = drawDecoration(decoration, true, true);

        // Draw the expected result on a bitmap
        Bitmap expectedBitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ARGB_4444);
        Canvas expectedCanvas = new Canvas(expectedBitmap);
        Paint paint = new Paint();
        paint.setColor(Color.RED);
        expectedCanvas.drawRect(0, 5, 20, 6, paint);
        expectedCanvas.drawRect(0, 10, 20, 11, paint);
        expectedCanvas.drawRect(0, 15, 20, 16, paint);
        // Compare the two bitmaps
        assertBitmapEquals(expectedBitmap, bitmap);

        bitmap.recycle();
        bitmap = drawDecoration(decoration, false, true);
        // should still be the same.
        assertBitmapEquals(expectedBitmap, bitmap);

        bitmap.recycle();
        bitmap = drawDecoration(decoration, true, false);
        // last item should not have a divider below it now
        paint.setColor(Color.TRANSPARENT);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        expectedCanvas.drawRect(0, 15, 20, 16, paint);
        assertBitmapEquals(expectedBitmap, bitmap);

        bitmap.recycle();
        bitmap = drawDecoration(decoration, false, false);
        // everything should be transparent now
        expectedCanvas.drawRect(0, 5, 20, 6, paint);
        expectedCanvas.drawRect(0, 10, 20, 11, paint);
        assertBitmapEquals(expectedBitmap, bitmap);

    }

    @SmallTest
    public void testShouldDrawDividerBelowWithBothCondition() {
        // Set up the item decoration, with 1px green divider line
        final DividerItemDecoration decoration = new DividerItemDecoration();
        Drawable divider = new ColorDrawable(Color.GREEN);
        decoration.setDivider(divider);
        decoration.setDividerHeight(1);
        decoration.setDividerCondition(DividerItemDecoration.DIVIDER_CONDITION_BOTH);

        Bitmap bitmap = drawDecoration(decoration, true, true);
        Paint paint = new Paint();
        paint.setColor(Color.GREEN);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.ADD));
        Bitmap expectedBitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ARGB_4444);
        Canvas expectedCanvas = new Canvas(expectedBitmap);
        expectedCanvas.drawRect(0, 5, 20, 6, paint);
        expectedCanvas.drawRect(0, 10, 20, 11, paint);
        expectedCanvas.drawRect(0, 15, 20, 16, paint);
        // Should have all the dividers
        assertBitmapEquals(expectedBitmap, bitmap);

        bitmap.recycle();
        bitmap = drawDecoration(decoration, false, true);
        paint.setColor(Color.TRANSPARENT);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        expectedCanvas.drawRect(0, 5, 20, 6, paint);
        expectedCanvas.drawRect(0, 10, 20, 11, paint);
        assertBitmapEquals(expectedBitmap, bitmap);

        bitmap.recycle();
        bitmap = drawDecoration(decoration, true, false);
        // nothing should be drawn now.
        expectedCanvas.drawRect(0, 15, 20, 16, paint);
        assertBitmapEquals(expectedBitmap, bitmap);

        bitmap.recycle();
        bitmap = drawDecoration(decoration, false, false);
        assertBitmapEquals(expectedBitmap, bitmap);
    }

    private Bitmap drawDecoration(DividerItemDecoration decoration, final boolean allowDividerAbove,
            final boolean allowDividerBelow) {
        // Set up the canvas to be drawn
        Bitmap bitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ARGB_4444);
        Canvas canvas = new Canvas(bitmap);

        // Set up recycler view with vertical linear layout manager
        RecyclerView testRecyclerView = new RecyclerView(getContext());
        testRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));

        // Set up adapter with 3 items, each 5px tall
        testRecyclerView.setAdapter(new RecyclerView.Adapter() {
            @Override
            public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup viewGroup, int i) {
                final View itemView = new View(getContext());
                itemView.setMinimumWidth(20);
                itemView.setMinimumHeight(5);
                return ViewHolder.createInstance(itemView, allowDividerAbove, allowDividerBelow);
            }

            @Override
            public void onBindViewHolder(RecyclerView.ViewHolder viewHolder, int i) {
            }

            @Override
            public int getItemCount() {
                return 3;
            }
        });

        testRecyclerView.layout(0, 0, 20, 20);
        decoration.onDraw(canvas, testRecyclerView, null);
        return bitmap;
    }

    private void assertBitmapEquals(Bitmap expected, Bitmap actual) {
        assertEquals("Width should be the same", expected.getWidth(), actual.getWidth());
        assertEquals("Height should be the same", expected.getHeight(), actual.getHeight());
        for (int x = 0; x < expected.getWidth(); x++) {
            for (int y = 0; y < expected.getHeight(); y++) {
                assertEquals("Pixel at (" + x + ", " + y + ") should be the same",
                        expected.getPixel(x, y), actual.getPixel(x, y));
            }
        }
    }

    private static class ViewHolder extends RecyclerView.ViewHolder
            implements DividerItemDecoration.DividedViewHolder {

        private boolean mAllowDividerAbove;
        private boolean mAllowDividerBelow;

        public static ViewHolder createInstance(View itemView, boolean allowDividerAbove,
                boolean allowDividerBelow) {
            return new ViewHolder(itemView, allowDividerAbove, allowDividerBelow);
        }

        private ViewHolder(View itemView, boolean allowDividerAbove, boolean allowDividerBelow) {
            super(itemView);
            mAllowDividerAbove = allowDividerAbove;
            mAllowDividerBelow = allowDividerBelow;
        }

        @Override
        public boolean isDividerAllowedAbove() {
            return mAllowDividerAbove;
        }

        @Override
        public boolean isDividerAllowedBelow() {
            return mAllowDividerBelow;
        }
    }
}
