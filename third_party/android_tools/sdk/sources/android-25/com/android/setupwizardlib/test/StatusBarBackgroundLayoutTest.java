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
import android.graphics.drawable.ShapeDrawable;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.setupwizardlib.view.StatusBarBackgroundLayout;

public class StatusBarBackgroundLayoutTest extends AndroidTestCase {

    @SmallTest
    public void testSetStatusBarBackground() {
        final StatusBarBackgroundLayout layout = new StatusBarBackgroundLayout(getContext());
        final ShapeDrawable drawable = new ShapeDrawable();
        layout.setStatusBarBackground(drawable);
        assertSame("Status bar background drawable should be same as set",
                drawable, layout.getStatusBarBackground());
    }

    @SmallTest
    public void testAttachedToWindow() {
        // Attaching to window should request apply window inset
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
            final TestStatusBarBackgroundLayout layout =
                    new TestStatusBarBackgroundLayout(getContext());
            layout.mRequestApplyInsets = false;
            layout.onAttachedToWindow();

            assertTrue("Attaching to window should apply window inset", layout.mRequestApplyInsets);
        }
    }

    private static class TestStatusBarBackgroundLayout extends StatusBarBackgroundLayout {

        boolean mRequestApplyInsets = false;

        TestStatusBarBackgroundLayout(Context context) {
            super(context);
        }

        @Override
        public void onAttachedToWindow() {
            super.onAttachedToWindow();
        }

        @Override
        public void requestApplyInsets() {
            super.requestApplyInsets();
            mRequestApplyInsets = true;
        }
    }
}
