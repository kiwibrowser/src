// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import android.view.View;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.download.home.filter.FilterCoordinator.TabType;
import org.chromium.chrome.browser.modelutil.PropertyObservable;

/**
 * The model responsible for maintaining the visual state of the tab content filter widget.  It also
 * holds callbacks meant to be notified when the tab selection changes.
 */
class FilterModel extends PropertyObservable<FilterModel.PropertyKey> {
    static class PropertyKey {
        static final PropertyKey CONTENT_VIEW = new PropertyKey();
        static final PropertyKey SELECTED_TAB = new PropertyKey();
        static final PropertyKey CHANGE_LISTENER = new PropertyKey();

        private PropertyKey() {}
    }

    private View mContentView;
    private @TabType int mSelectedTab;
    private Callback</* @TabType */ Integer> mChangeListener;

    /** Sets {@code contentView} as the {@link View} to be shown in the content area. */
    public void setContentView(View contentView) {
        if (mContentView == contentView) return;
        mContentView = contentView;
        notifyPropertyChanged(PropertyKey.CONTENT_VIEW);
    }

    /** Sets which tab should be selected. */
    public void setSelectedTab(@TabType int selectedTab) {
        // Note: This does not early-out if selectedTab is the same as mSelectedTab.  This is
        // because default values might prevent us from pushing valid state and causing the UI to
        // refresh.
        mSelectedTab = selectedTab;
        notifyPropertyChanged(PropertyKey.SELECTED_TAB);
    }

    /** Sets the {@link Callback} to call when a tab is selected. */
    public void setChangeListener(Callback</* @TabType */ Integer> changeListener) {
        if (mChangeListener == changeListener) return;
        mChangeListener = changeListener;
        notifyPropertyChanged(PropertyKey.CHANGE_LISTENER);
    }

    /** @return The {@link View} to use in the content area of the tab selection. */
    public View getContentView() {
        return mContentView;
    }

    /** @return The selected tab type. */
    public @TabType int getSelectedTab() {
        return mSelectedTab;
    }

    /** @return The {@link Callback} to call when a tab is selected. */
    public Callback</* @TabType */ Integer> getChangeListener() {
        return mChangeListener;
    }
}