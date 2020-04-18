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

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import android.os.Build;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.View;

import com.android.setupwizardlib.util.DrawableLayoutDirectionHelper;

import java.util.Locale;

public class DrawableLayoutDirectionHelperTest extends AndroidTestCase {

    @SmallTest
    public void testCreateRelativeInsetDrawableLtr() {
        final Drawable drawable = new ColorDrawable(Color.RED);
        final InsetDrawable insetDrawable =
                DrawableLayoutDirectionHelper.createRelativeInsetDrawable(drawable,
                        1 /* start */, 2 /* top */, 3 /* end */, 4 /* bottom */,
                        View.LAYOUT_DIRECTION_LTR);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            assertSame("Drawable from getDrawable() should be same as passed in", drawable,
                    insetDrawable.getDrawable());
        }
        Rect outRect = new Rect();
        insetDrawable.getPadding(outRect);
        assertEquals("InsetDrawable padding should be same as inset", new Rect(1, 2, 3, 4),
                outRect);
    }

    @SmallTest
    public void testCreateRelativeInsetDrawableRtl() {
        final Drawable drawable = new ColorDrawable(Color.RED);
        final InsetDrawable insetDrawable =
                DrawableLayoutDirectionHelper.createRelativeInsetDrawable(drawable,
                        1 /* start */, 2 /* top */, 3 /* end */, 4 /* bottom */,
                        View.LAYOUT_DIRECTION_RTL);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            assertSame("Drawable from getDrawable() should be same as passed in", drawable,
                    insetDrawable.getDrawable());
        }
        Rect outRect = new Rect();
        insetDrawable.getPadding(outRect);
        assertEquals("InsetDrawable padding should be same as inset", new Rect(3, 2, 1, 4),
                outRect);
    }

    @SmallTest
    public void testCreateRelativeInsetDrawableViewRtl() {
        final Drawable drawable = new ColorDrawable(Color.RED);
        final View view = new ForceRtlView(getContext());
        final InsetDrawable insetDrawable =
                DrawableLayoutDirectionHelper.createRelativeInsetDrawable(drawable,
                        1 /* start */, 2 /* top */, 3 /* end */, 4 /* bottom */, view);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            assertSame("Drawable from getDrawable() should be same as passed in", drawable,
                    insetDrawable.getDrawable());
        }
        Rect outRect = new Rect();
        insetDrawable.getPadding(outRect);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            assertEquals("InsetDrawable padding should be same as inset", new Rect(3, 2, 1, 4),
                    outRect);
        } else {
            assertEquals("InsetDrawable padding should be same as inset", new Rect(1, 2, 3, 4),
                    outRect);
        }
    }

    @SmallTest
    public void testCreateRelativeInsetDrawableContextRtl() {
        final Drawable drawable = new ColorDrawable(Color.RED);
        Context context = getContext();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            final Configuration config = new Configuration();
            config.setLayoutDirection(new Locale("fa", "IR"));
            context = getContext().createConfigurationContext(config);
        }
        final InsetDrawable insetDrawable =
                DrawableLayoutDirectionHelper.createRelativeInsetDrawable(drawable,
                        1 /* start */, 2 /* top */, 3 /* end */, 4 /* bottom */, context);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            assertSame("Drawable from getDrawable() should be same as passed in", drawable,
                    insetDrawable.getDrawable());
        }
        Rect outRect = new Rect();
        insetDrawable.getPadding(outRect);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            assertEquals("InsetDrawable padding should be same as inset", new Rect(3, 2, 1, 4),
                    outRect);
        } else {
            assertEquals("InsetDrawable padding should be same as inset", new Rect(1, 2, 3, 4),
                    outRect);
        }
    }

    private static class ForceRtlView extends View {

        public ForceRtlView(Context context) {
            super(context);
        }

        @Override
        public int getLayoutDirection() {
            return View.LAYOUT_DIRECTION_RTL;
        }
    }
}
