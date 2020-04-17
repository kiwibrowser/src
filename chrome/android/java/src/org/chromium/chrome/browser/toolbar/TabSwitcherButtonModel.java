// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;

import org.chromium.chrome.browser.modelutil.PropertyObservable;

/**
 * All of the state for the tab switcher, updated by the {@link TabSwitcherButtonController}.
 */
public class TabSwitcherButtonModel extends PropertyObservable<TabSwitcherButtonModel.PropertyKey> {
    /** The different properties that can change on the tab switcher. */
    public static class PropertyKey {
        public static final PropertyKey NUMBER_OF_TABS = new PropertyKey();
        public static final PropertyKey ON_CLICK_LISTENER = new PropertyKey();
        public static final PropertyKey ON_LONG_CLICK_LISTENER = new PropertyKey();

        private PropertyKey() {}
    }

    /** The number of tabs. */
    private int mNumberOfTabs;

    /** The click listener for the tab switcher button */
    private OnClickListener mOnClickListener;

    /** The long click listener for the tab switcher button */
    private OnLongClickListener mOnLongClickListener;

    /** Default constructor. */
    public TabSwitcherButtonModel() {}

    /**
     * @param tabCount The current number of tabs.
     */
    public void setNumberOfTabs(int numberOfTabs) {
        mNumberOfTabs = numberOfTabs;
        notifyPropertyChanged(PropertyKey.NUMBER_OF_TABS);
    }

    /**
     * @return The current number of tabs.
     */
    public int getNumberOfTabs() {
        return mNumberOfTabs;
    }

    /**
     * @param listener The click listener for the tab switcher button.
     */
    public void setOnClickListener(OnClickListener listener) {
        mOnClickListener = listener;
        notifyPropertyChanged(PropertyKey.ON_CLICK_LISTENER);
    }

    /**
     * @return The click listener for the tab switcher button.
     */
    public OnClickListener getOnClickListener() {
        return mOnClickListener;
    }

    /**
     * @param listener The long click listener for the tab switcher button.
     */
    public void setOnLongClickListener(OnLongClickListener listener) {
        mOnLongClickListener = listener;
        notifyPropertyChanged(PropertyKey.ON_LONG_CLICK_LISTENER);
    }

    /**
     * @return The long click listener for the tab switcher button.
     */
    public OnLongClickListener getOnLongClickListener() {
        return mOnLongClickListener;
    }
}
