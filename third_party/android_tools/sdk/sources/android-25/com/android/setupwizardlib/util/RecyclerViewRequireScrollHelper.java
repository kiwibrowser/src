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

package com.android.setupwizardlib.util;

import android.support.v7.widget.RecyclerView;

import com.android.setupwizardlib.view.NavigationBar;

/**
 * Add this helper to require the recycler view to be scrolled to the bottom, making sure that the
 * user sees all content on the screen. This will change the navigation bar to show the more button
 * instead of the next button when there is more content to be seen. When the more button is
 * clicked, the scroll view will be scrolled one page down.
 */
public class RecyclerViewRequireScrollHelper extends AbstractRequireScrollHelper {

    public static void requireScroll(NavigationBar navigationBar, RecyclerView recyclerView) {
        new RecyclerViewRequireScrollHelper(navigationBar, recyclerView).requireScroll();
    }

    private final RecyclerView mRecyclerView;

    private RecyclerViewRequireScrollHelper(NavigationBar navigationBar,
            RecyclerView recyclerView) {
        super(navigationBar);
        mRecyclerView = recyclerView;
    }

    protected void requireScroll() {
        super.requireScroll();
        mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
            @Override
            public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
                if (!canScrollDown()) {
                    notifyScrolledToBottom();
                } else {
                    notifyRequiresScroll();
                }
            }
        });

        if (canScrollDown()) {
            notifyRequiresScroll();
        }
    }

    private boolean canScrollDown() {
        // Compatibility implementation of View#canScrollVertically
        final int offset = mRecyclerView.computeVerticalScrollOffset();
        final int range = mRecyclerView.computeVerticalScrollRange()
                - mRecyclerView.computeVerticalScrollExtent();
        return range != 0 && offset < range - 1;
    }

    @Override
    protected void pageScrollDown() {
        final int height = mRecyclerView.getHeight();
        mRecyclerView.smoothScrollBy(0, height);
    }
}
