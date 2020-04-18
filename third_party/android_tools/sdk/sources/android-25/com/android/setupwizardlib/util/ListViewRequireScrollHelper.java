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

import android.os.Build;
import android.widget.AbsListView;
import android.widget.ListAdapter;
import android.widget.ListView;

import com.android.setupwizardlib.view.NavigationBar;

/**
 * Add this helper to require the list view to be scrolled to the bottom, making sure that the
 * user sees all content on the screen. This will change the navigation bar to show the more button
 * instead of the next button when there is more content to be seen. When the more button is
 * clicked, the list view will be scrolled one page down.
 */
public class ListViewRequireScrollHelper extends AbstractRequireScrollHelper
        implements AbsListView.OnScrollListener {

    public static void requireScroll(NavigationBar navigationBar, ListView listView) {
        new ListViewRequireScrollHelper(navigationBar, listView).requireScroll();
    }

    private final ListView mListView;

    private ListViewRequireScrollHelper(NavigationBar navigationBar, ListView listView) {
        super(navigationBar);
        mListView = listView;
    }

    @Override
    protected void requireScroll() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
            // APIs to scroll a list only exists on Froyo or above.
            super.requireScroll();
            mListView.setOnScrollListener(this);

            final ListAdapter adapter = mListView.getAdapter();
            if (mListView.getLastVisiblePosition() < adapter.getCount()) {
                notifyRequiresScroll();
            }
        }
    }

    @Override
    protected void pageScrollDown() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.FROYO) {
            final int height = mListView.getHeight();
            mListView.smoothScrollBy(height, 500);
        }
    }

    @Override
    public void onScrollStateChanged(AbsListView view, int scrollState) {
    }

    @Override
    public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
            int totalItemCount) {
        if (firstVisibleItem + visibleItemCount >= totalItemCount) {
            notifyScrolledToBottom();
        } else {
            notifyRequiresScroll();
        }
    }
}
