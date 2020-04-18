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

import android.view.View;

import com.android.setupwizardlib.view.NavigationBar;

/**
 * Add this helper to require the scroll view to be scrolled to the bottom, making sure that the
 * user sees all content on the screen. This will change the navigation bar to show the more button
 * instead of the next button when there is more content to be seen. When the more button is
 * clicked, the scroll view will be scrolled one page down.
 */
public abstract class AbstractRequireScrollHelper implements View.OnClickListener {

    private final NavigationBar mNavigationBar;

    private boolean mScrollNeeded;
    // Whether the user have seen the more button yet.
    private boolean mScrollNotified = false;

    protected AbstractRequireScrollHelper(NavigationBar navigationBar) {
        mNavigationBar = navigationBar;
    }

    protected void requireScroll() {
        mNavigationBar.getMoreButton().setOnClickListener(this);
    }

    protected void notifyScrolledToBottom() {
        if (mScrollNeeded) {
            mNavigationBar.post(new Runnable() {
                @Override
                public void run() {
                    mNavigationBar.getNextButton().setVisibility(View.VISIBLE);
                    mNavigationBar.getMoreButton().setVisibility(View.GONE);
                }
            });
            mScrollNeeded = false;
            mScrollNotified = true;
        }
    }

    protected void notifyRequiresScroll() {
        if (!mScrollNeeded && !mScrollNotified) {
            mNavigationBar.post(new Runnable() {
                @Override
                public void run() {
                    mNavigationBar.getNextButton().setVisibility(View.GONE);
                    mNavigationBar.getMoreButton().setVisibility(View.VISIBLE);
                }
            });
            mScrollNeeded = true;
        }
    }

    @Override
    public void onClick(View view) {
        pageScrollDown();
    }

    protected abstract void pageScrollDown();
}
