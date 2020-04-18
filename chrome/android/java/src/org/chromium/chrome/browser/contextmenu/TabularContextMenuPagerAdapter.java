// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.support.v4.text.TextUtilsCompat;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewCompat;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;
import java.util.Locale;

/**
 * Takes a list of views and strings and creates a wrapper for the ViewPager and Tab adapter.
 */
class TabularContextMenuPagerAdapter extends PagerAdapter {
    private final List<Pair<String, ViewGroup>> mViewList;
    private final boolean mIsRightToLeft;

    /**
     * Used in combination of a TabLayout to create a multi view layout.
     * @param views Thew views to use in the pager Adapter.
     */
    TabularContextMenuPagerAdapter(List<Pair<String, ViewGroup>> views) {
        mViewList = views;
        mIsRightToLeft = TextUtilsCompat.getLayoutDirectionFromLocale(Locale.getDefault())
                == ViewCompat.LAYOUT_DIRECTION_RTL;
    }

    // Addresses the RTL display bug: https://code.google.com/p/android/issues/detail?id=56831
    private int adjustIndexForDirectionality(int index, int count) {
        if (mIsRightToLeft) {
            return count - 1 - index;
        }
        return index;
    }

    @Override
    public Object instantiateItem(ViewGroup container, int position) {
        position = adjustIndexForDirectionality(position, getCount());
        ViewGroup layout = mViewList.get(position).second;
        container.addView(layout);
        return layout;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
        position = adjustIndexForDirectionality(position, getCount());
        container.removeViewAt(position);
    }

    @Override
    public int getCount() {
        return mViewList.size();
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        return view == object;
    }

    @Override
    public CharSequence getPageTitle(int position) {
        position = adjustIndexForDirectionality(position, getCount());
        return mViewList.get(position).first;
    }
}
