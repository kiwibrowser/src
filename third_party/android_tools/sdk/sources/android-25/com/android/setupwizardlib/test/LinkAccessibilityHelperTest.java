/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.graphics.Rect;
import android.os.Bundle;
import android.support.v4.view.accessibility.AccessibilityNodeInfoCompat;
import android.support.v4.widget.ExploreByTouchHelper;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.SpannableStringBuilder;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.accessibility.AccessibilityEvent;
import android.widget.TextView;

import com.android.setupwizardlib.span.LinkSpan;
import com.android.setupwizardlib.util.LinkAccessibilityHelper;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class LinkAccessibilityHelperTest extends AndroidTestCase {

    private TextView mTextView;
    private TestLinkAccessibilityHelper mHelper;
    private LinkSpan mSpan;

    private DisplayMetrics mDisplayMetrics;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mSpan = new LinkSpan("foobar");
        SpannableStringBuilder ssb = new SpannableStringBuilder("Hello world");
        ssb.setSpan(mSpan, 1, 2, 0 /* flags */);

        mTextView = new TextView(getContext());
        mTextView.setText(ssb);
        mTextView.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 15);
        mHelper = new TestLinkAccessibilityHelper(mTextView);

        mTextView.measure(dp2Px(500), dp2Px(500));
        mTextView.layout(dp2Px(0), dp2Px(0), dp2Px(500), dp2Px(500));
    }

    @SmallTest
    public void testGetVirtualViewAt() {
        final int virtualViewId = mHelper.getVirtualViewAt(dp2Px(15), dp2Px(10));
        assertEquals("Virtual view ID should be 1", 1, virtualViewId);
    }

    @SmallTest
    public void testGetVirtualViewAtHost() {
        final int virtualViewId = mHelper.getVirtualViewAt(dp2Px(100), dp2Px(100));
        assertEquals("Virtual view ID should be INVALID_ID",
                ExploreByTouchHelper.INVALID_ID, virtualViewId);
    }

    @SmallTest
    public void testGetVisibleVirtualViews() {
        List<Integer> virtualViewIds = new ArrayList<>();
        mHelper.getVisibleVirtualViews(virtualViewIds);

        assertEquals("VisibleVirtualViews should be [1]",
                Collections.singletonList(1), virtualViewIds);
    }

    @SmallTest
    public void testOnPopulateEventForVirtualView() {
        AccessibilityEvent event = AccessibilityEvent.obtain();
        mHelper.onPopulateEventForVirtualView(1, event);

        // LinkSpan is set on substring(1, 2) of "Hello world" --> "e"
        assertEquals("LinkSpan description should be \"e\"",
                "e", event.getContentDescription().toString());

        event.recycle();
    }

    @SmallTest
    public void testOnPopulateEventForVirtualViewHost() {
        AccessibilityEvent event = AccessibilityEvent.obtain();
        mHelper.onPopulateEventForVirtualView(ExploreByTouchHelper.INVALID_ID, event);

        assertEquals("Host view description should be \"Hello world\"", "Hello world",
                event.getContentDescription().toString());

        event.recycle();
    }

    @SmallTest
    public void testOnPopulateNodeForVirtualView() {
        AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
        mHelper.onPopulateNodeForVirtualView(1, info);

        assertEquals("LinkSpan description should be \"e\"",
                "e", info.getContentDescription().toString());
        assertTrue("LinkSpan should be focusable", info.isFocusable());
        assertTrue("LinkSpan should be clickable", info.isClickable());
        Rect bounds = new Rect();
        info.getBoundsInParent(bounds);
        assertEquals("LinkSpan bounds should be (10.5dp, 0dp, 18.5dp, 20.5dp)",
                new Rect(dp2Px(10.5f), dp2Px(0f), dp2Px(18.5f), dp2Px(20.5f)), bounds);

        info.recycle();
    }

    @SmallTest
    public void testNullLayout() {
        // Setting the padding will cause the layout to be null-ed out.
        mTextView.setPadding(1, 1, 1, 1);

        AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
        mHelper.onPopulateNodeForVirtualView(0, info);

        Rect bounds = new Rect();
        info.getBoundsInParent(bounds);
        assertEquals("LinkSpan bounds should be (0, 0, 1, 1)",
                new Rect(0, 0, 1, 1), bounds);

        info.recycle();
    }

    @SmallTest
    public void testRtlLayout() {
        // Redo setUp with a Hebrew (RTL) string.
        mSpan = new LinkSpan("foobar");
        SpannableStringBuilder ssb = new SpannableStringBuilder("מכונה בתרגום");
        ssb.setSpan(mSpan, 1, 2, 0 /* flags */);

        mTextView = new TextView(getContext());
        mTextView.setText(ssb);
        mTextView.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 15);
        mHelper = new TestLinkAccessibilityHelper(mTextView);

        mTextView.measure(dp2Px(500), dp2Px(500));
        mTextView.layout(dp2Px(0), dp2Px(0), dp2Px(500), dp2Px(500));
        // End redo setup

        AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.obtain();
        mHelper.onPopulateNodeForVirtualView(1, info);

        assertEquals("LinkSpan description should be \"כ\"",
                "כ", info.getContentDescription().toString());
        assertTrue("LinkSpan should be focusable", info.isFocusable());
        assertTrue("LinkSpan should be clickable", info.isClickable());
        Rect bounds = new Rect();
        info.getBoundsInParent(bounds);
        assertEquals("LinkSpan bounds should be (70.5dp, 0dp, 78.5dp, 20.5dp)",
                new Rect(dp2Px(70.5f), dp2Px(0f), dp2Px(78.5f), dp2Px(20.5f)), bounds);

        info.recycle();
    }

    private int dp2Px(float dp) {
        if (mDisplayMetrics == null) {
            mDisplayMetrics = getContext().getResources().getDisplayMetrics();
        }
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, mDisplayMetrics);
    }

    private static class TestLinkAccessibilityHelper extends LinkAccessibilityHelper {

        public TestLinkAccessibilityHelper(TextView view) {
            super(view);
        }

        @Override
        public int getVirtualViewAt(float x, float y) {
            return super.getVirtualViewAt(x, y);
        }

        @Override
        public void getVisibleVirtualViews(List<Integer> virtualViewIds) {
            super.getVisibleVirtualViews(virtualViewIds);
        }

        @Override
        public void onPopulateEventForVirtualView(int virtualViewId, AccessibilityEvent event) {
            super.onPopulateEventForVirtualView(virtualViewId, event);
        }

        @Override
        public void onPopulateNodeForVirtualView(int virtualViewId,
                AccessibilityNodeInfoCompat info) {
            super.onPopulateNodeForVirtualView(virtualViewId, info);
        }

        @Override
        public boolean onPerformActionForVirtualView(int virtualViewId, int action,
                Bundle arguments) {
            return super.onPerformActionForVirtualView(virtualViewId, action, arguments);
        }
    }
}
