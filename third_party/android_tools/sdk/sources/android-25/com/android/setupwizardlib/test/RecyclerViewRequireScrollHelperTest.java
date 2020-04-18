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

import android.content.Context;
import android.os.Build;
import android.support.v7.widget.RecyclerView;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.View;

import com.android.setupwizardlib.util.RecyclerViewRequireScrollHelper;
import com.android.setupwizardlib.view.NavigationBar;

public class RecyclerViewRequireScrollHelperTest extends AndroidTestCase {

    private TestRecyclerView mRecyclerView;
    private NavigationBar mNavigationBar;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mRecyclerView = new TestRecyclerView(getContext());
        mNavigationBar = new TestNavigationBar(getContext());

        mRecyclerView.layout(0, 0, 50, 50);
    }

    @SmallTest
    public void testRequireScroll() {
        RecyclerViewRequireScrollHelper.requireScroll(mNavigationBar, mRecyclerView);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
            assertEquals("More button should be shown initially", View.VISIBLE,
                    mNavigationBar.getMoreButton().getVisibility());
            assertEquals("Next button should be gone initially", View.GONE,
                    mNavigationBar.getNextButton().getVisibility());
        }
    }

    @SmallTest
    public void testScrolledToBottom() {
        RecyclerViewRequireScrollHelper.requireScroll(mNavigationBar, mRecyclerView);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
            assertEquals("More button should be shown when scroll is required", View.VISIBLE,
                    mNavigationBar.getMoreButton().getVisibility());
            assertEquals("Next button should not be shown when scroll is required", View.GONE,
                    mNavigationBar.getNextButton().getVisibility());

            mRecyclerView.scrollOffset = 20;
            mRecyclerView.listener.onScrolled(mRecyclerView, 0, 20);
            assertEquals("More button should be hidden when scrolled to bottom", View.GONE,
                    mNavigationBar.getMoreButton().getVisibility());
            assertEquals("Next button should be shown when scrolled to bottom", View.VISIBLE,
                    mNavigationBar.getNextButton().getVisibility());
        }
    }

    @SmallTest
    public void testClickScrollButton() {
        RecyclerViewRequireScrollHelper.requireScroll(mNavigationBar, mRecyclerView);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
            assertEquals("ScrollView page should be initially 0", 0, mRecyclerView.scrollDistance);
            mNavigationBar.getMoreButton().performClick();
            assertEquals("ScrollView page should be scrolled by 50px",
                    50, mRecyclerView.scrollDistance);
        }
    }

    private static class TestRecyclerView extends RecyclerView {

        public int scrollOffset = 0;
        public int scrollRange = 20;
        public int scrollExtent = 0;

        public int scrollDistance = 0;

        public OnScrollListener listener;

        public TestRecyclerView(Context context) {
            super(context);
        }

        @Override
        public void addOnScrollListener(OnScrollListener listener) {
            super.addOnScrollListener(listener);
            this.listener = listener;
        }

        @Override
        public int computeVerticalScrollOffset() {
            return scrollOffset;
        }

        @Override
        public int computeVerticalScrollRange() {
            return scrollRange;
        }

        @Override
        public int computeVerticalScrollExtent() {
            return scrollExtent;
        }

        @Override
        public void smoothScrollBy(int dx, int dy) {
            super.smoothScrollBy(dx, dy);
            scrollDistance += dy;
        }
    }

    private static class TestNavigationBar extends NavigationBar {

        public TestNavigationBar(Context context) {
            super(context);
        }

        @Override
        public boolean post(Runnable action) {
            action.run();
            return true;
        }
    }
}
