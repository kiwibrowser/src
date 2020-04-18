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

package com.android.setupwizardlib.util;

import android.widget.ScrollView;

import com.android.setupwizardlib.view.BottomScrollView;
import com.android.setupwizardlib.view.NavigationBar;

/**
 * Add this helper to require the scroll view to be scrolled to the bottom, making sure that the
 * user sees all content on the screen. This will change the navigation bar to show the more button
 * instead of the next button when there is more content to be seen. When the more button is
 * clicked, the scroll view will be scrolled one page down.
 */
public class RequireScrollHelper extends AbstractRequireScrollHelper
            implements BottomScrollView.BottomScrollListener {

    public static void requireScroll(NavigationBar navigationBar, BottomScrollView scrollView) {
        new RequireScrollHelper(navigationBar, scrollView).requireScroll();
    }

    private final BottomScrollView mScrollView;

    private RequireScrollHelper(NavigationBar navigationBar, BottomScrollView scrollView) {
        super(navigationBar);
        mScrollView = scrollView;
    }

    @Override
    protected void requireScroll() {
        super.requireScroll();
        mScrollView.setBottomScrollListener(this);
    }

    @Override
    protected void pageScrollDown() {
        mScrollView.pageScroll(ScrollView.FOCUS_DOWN);
    }

    @Override
    public void onScrolledToBottom() {
        notifyScrolledToBottom();
    }

    @Override
    public void onRequiresScroll() {
        notifyRequiresScroll();
    }
}
