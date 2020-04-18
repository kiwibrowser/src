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

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.SystemClock;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import com.android.setupwizardlib.test.util.MockWindow;
import com.android.setupwizardlib.util.SystemBarHelper;

public class SystemBarHelperTest extends AndroidTestCase {

    private static final int STATUS_BAR_DISABLE_BACK = 0x00400000;

    @SuppressLint("InlinedApi")
    private static final int DEFAULT_IMMERSIVE_FLAGS =
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;

    @SuppressLint("InlinedApi")
    private static final int DIALOG_IMMERSIVE_FLAGS =
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

    @SmallTest
    public void testAddVisibilityFlagView() {
        final View view = createViewWithSystemUiVisibility(0x456);
        SystemBarHelper.addVisibilityFlag(view, 0x1400);
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            // Check that result is 0x1456, because 0x1400 | 0x456 = 0x1456.
            assertEquals("View visibility should be 0x1456", 0x1456, view.getSystemUiVisibility());
        }
    }

    @SmallTest
    public void testRemoveVisibilityFlagView() {
        final View view = createViewWithSystemUiVisibility(0x456);
        SystemBarHelper.removeVisibilityFlag(view, 0x1400);
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            // Check that result is 0x56, because 0x456 & ~0x1400 = 0x56.
            assertEquals("View visibility should be 0x56", 0x56, view.getSystemUiVisibility());
        }
    }

    @SmallTest
    public void testAddVisibilityFlagWindow() {
        final Window window = createWindowWithSystemUiVisibility(0x456);
        SystemBarHelper.addVisibilityFlag(window, 0x1400);
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            // Check that result is 0x1456 = 0x1400 | 0x456.
            assertEquals("View visibility should be 0x1456", 0x1456,
                    window.getAttributes().systemUiVisibility);
        }
    }

    @SmallTest
    public void testRemoveVisibilityFlagWindow() {
        final Window window = createWindowWithSystemUiVisibility(0x456);
        SystemBarHelper.removeVisibilityFlag(window, 0x1400);
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            // Check that result is 0x56 = 0x456 & ~0x1400.
            assertEquals("View visibility should be 0x56", 0x56,
                    window.getAttributes().systemUiVisibility);
        }
    }

    @SmallTest
    public void testHideSystemBarsWindow() {
        final Window window = createWindowWithSystemUiVisibility(0x456);
        SystemBarHelper.hideSystemBars(window);
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            assertEquals("DEFAULT_IMMERSIVE_FLAGS should be added to window's systemUiVisibility",
                    DEFAULT_IMMERSIVE_FLAGS | 0x456,
                    window.getAttributes().systemUiVisibility);
            assertEquals(
                    "DEFAULT_IMMERSIVE_FLAGS should be added to decorView's systemUiVisibility",
                    DEFAULT_IMMERSIVE_FLAGS | 0x456,
                    window.getDecorView().getSystemUiVisibility());
            assertEquals("Navigation bar should be transparent", window.getNavigationBarColor(), 0);
            assertEquals("Status bar should be transparent", window.getStatusBarColor(), 0);
        }
    }

    @SmallTest
    public void testShowSystemBarsWindow() {
        final Window window = createWindowWithSystemUiVisibility(0x456);
        SystemBarHelper.showSystemBars(window, getContext());
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            assertEquals(
                    "DEFAULT_IMMERSIVE_FLAGS should be removed from window's systemUiVisibility",
                    0x456 & ~DEFAULT_IMMERSIVE_FLAGS,
                    window.getAttributes().systemUiVisibility);
            assertEquals(
                    "DEFAULT_IMMERSIVE_FLAGS should be removed from decorView's systemUiVisibility",
                    0x456 & ~DEFAULT_IMMERSIVE_FLAGS,
                    window.getDecorView().getSystemUiVisibility());
            assertEquals("Navigation bar should not be transparent",
                    window.getNavigationBarColor(), 0xff000000);
            assertEquals("Status bar should not be transparent",
                    window.getStatusBarColor(), 0xff000000);
        }
    }

    @SmallTest
    public void testHideSystemBarsNoInfiniteLoop() throws InterruptedException {
        final TestWindow window = new TestWindow(getContext(), null);
        final HandlerThread thread = new HandlerThread("SystemBarHelperTest");
        thread.start();
        final Handler handler = new Handler(thread.getLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                SystemBarHelper.hideSystemBars(window);
            }
        });
        SystemClock.sleep(500);  // Wait for the looper to drain all the messages
        thread.quit();
        // Initial peek + 3 retries = 4 tries total
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            assertEquals("Peek decor view should give up after 4 tries", 4,
                    window.peekDecorViewCount);
        }
    }

    @SmallTest
    public void testHideSystemBarsDialog() {
        final Dialog dialog = new Dialog(mContext);
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            final WindowManager.LayoutParams attrs = dialog.getWindow().getAttributes();
            attrs.systemUiVisibility = 0x456;
            dialog.getWindow().setAttributes(attrs);
        }

        SystemBarHelper.hideSystemBars(dialog);
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            assertEquals("DIALOG_IMMERSIVE_FLAGS should be added to window's systemUiVisibility",
                    DIALOG_IMMERSIVE_FLAGS | 0x456,
                    dialog.getWindow().getAttributes().systemUiVisibility);
        }
    }

    @SmallTest
    public void testSetBackButtonVisibleTrue() {
        final Window window = createWindowWithSystemUiVisibility(0x456);
        SystemBarHelper.setBackButtonVisible(window, true);
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            assertEquals("View visibility should be 0x456", 0x456,
                    window.getAttributes().systemUiVisibility);
        }
    }

    @SmallTest
    public void testSetBackButtonVisibleFalse() {
        final Window window = createWindowWithSystemUiVisibility(0x456);
        SystemBarHelper.setBackButtonVisible(window, false);
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            assertEquals("STATUS_BAR_DISABLE_BACK should be added to systemUiVisibility",
                    0x456 | STATUS_BAR_DISABLE_BACK, window.getAttributes().systemUiVisibility);
        }
    }

    private View createViewWithSystemUiVisibility(int vis) {
        final View view = new View(getContext());
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            view.setSystemUiVisibility(vis);
        }
        return view;
    }

    private Window createWindowWithSystemUiVisibility(int vis) {
        final Window window = new TestWindow(getContext(), createViewWithSystemUiVisibility(vis));
        if (VERSION.SDK_INT >= VERSION_CODES.HONEYCOMB) {
            WindowManager.LayoutParams attrs = window.getAttributes();
            attrs.systemUiVisibility = vis;
            window.setAttributes(attrs);
        }
        return window;
    }

    private static class TestWindow extends MockWindow {

        private View mDecorView;
        public int peekDecorViewCount = 0;

        private int mNavigationBarColor = -1;
        private int mStatusBarColor = -1;

        public TestWindow(Context context, View decorView) {
            super(context);
            mDecorView = decorView;
        }

        @Override
        public View getDecorView() {
            return mDecorView;
        }

        @Override
        public View peekDecorView() {
            peekDecorViewCount++;
            return mDecorView;
        }

        @Override
        public void setNavigationBarColor(int i) {
            mNavigationBarColor = i;
        }

        @Override
        public int getNavigationBarColor() {
            return mNavigationBarColor;
        }

        @Override
        public void setStatusBarColor(int i) {
            mStatusBarColor = i;
        }

        @Override
        public int getStatusBarColor() {
            return mStatusBarColor;
        }
    }
}
