// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.keyboard_accessory;

import org.chromium.chrome.browser.autofill.keyboard_accessory.KeyboardAccessoryData.Tab;
import org.chromium.chrome.browser.modelutil.PropertyObservable;
import org.chromium.chrome.browser.modelutil.SimpleListObservable;

/**
 * This model holds all view state of the accessory sheet.
 * It is updated by the {@link AccessorySheetMediator} and emits notification on which observers
 * like the view binder react.
 */
class AccessorySheetModel extends PropertyObservable<AccessorySheetModel.PropertyKey> {
    public static class PropertyKey {
        public static final PropertyKey ACTIVE_TAB_INDEX = new PropertyKey();
        public static final PropertyKey VISIBLE = new PropertyKey();
    }

    public static final int NO_ACTIVE_TAB = -1;

    private int mActiveTabIndex = NO_ACTIVE_TAB;
    private boolean mVisible;
    private final SimpleListObservable<Tab> mTabList = new SimpleListObservable<>();

    SimpleListObservable<Tab> getTabList() {
        return mTabList;
    }

    void setVisible(boolean visible) {
        if (mVisible == visible) return; // Nothing to do here: same value.
        mVisible = visible;
        notifyPropertyChanged(PropertyKey.VISIBLE);
    }

    boolean isVisible() {
        return mVisible;
    }

    int getActiveTabIndex() {
        return mActiveTabIndex;
    }

    void setActiveTabIndex(int activeTabPosition) {
        if (mActiveTabIndex == activeTabPosition) return;
        assert((activeTabPosition >= 0 && activeTabPosition < mTabList.getItemCount())
                || activeTabPosition == NO_ACTIVE_TAB)
            : "Tried to set invalid index '" + activeTabPosition + "' as active tab!";
        mActiveTabIndex = activeTabPosition;
        notifyPropertyChanged(PropertyKey.ACTIVE_TAB_INDEX);
    }
}