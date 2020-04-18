// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import android.content.Context;
import android.support.annotation.IntDef;
import android.view.View;

import org.chromium.base.ObserverList;
import org.chromium.chrome.browser.download.home.filter.Filters.FilterType;
import org.chromium.chrome.browser.download.home.filter.chips.ChipsCoordinator;
import org.chromium.chrome.browser.modelutil.PropertyModelChangeProcessor;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/** A Coordinator responsible for showing the tab filter selection UI for downloads home. */
public class FilterCoordinator {
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({TAB_FILES, TAB_PREFETCH})
    public @interface TabType {}
    public static final int TAB_FILES = 0;
    public static final int TAB_PREFETCH = 1;

    /** An Observer to notify when the selected tab has changed. */
    public interface Observer {
        /** Called when the selected tab has changed. */
        void onFilterChanged(@FilterType int selectedTab);
    }

    private final ObserverList<Observer> mObserverList = new ObserverList<>();
    private final FilterModel mModel;
    private final FilterViewBinder mViewBinder;
    private final FilterView mView;

    private final ChipsCoordinator mChipsCoordinator;
    private final FilterChipsProvider mChipsProvider;

    /**
     * Builds a new FilterCoordinator.
     * @param context The context to build the views and pull parameters from.
     */
    public FilterCoordinator(Context context, OfflineItemFilterSource chipFilterSource) {
        mChipsProvider = new FilterChipsProvider(type -> handleChipSelected(), chipFilterSource);
        mChipsCoordinator = new ChipsCoordinator(context, mChipsProvider);

        mModel = new FilterModel();
        mViewBinder = new FilterViewBinder();
        mView = new FilterView(context);
        mModel.addObserver(new PropertyModelChangeProcessor<>(mModel, mView, mViewBinder));

        mModel.setChangeListener(selectedTab -> handleTabSelected(selectedTab));
        selectTab(TAB_FILES);
    }

    /** @return The {@link View} representing this widget. */
    public View getView() {
        return mView.getView();
    }

    /** Registers {@code observer} to be notified of tab selection changes. */
    public void addObserver(Observer observer) {
        mObserverList.addObserver(observer);
    }

    /** Unregisters {@code observer} from tab selection changes. */
    public void removeObserver(Observer observer) {
        mObserverList.removeObserver(observer);
    }

    /** Selects the tab identified by {@code selectedTab}. */
    public void selectTab(@TabType int selectedTab) {
        mModel.setSelectedTab(selectedTab);

        if (selectedTab == TAB_FILES) {
            mModel.setContentView(mChipsCoordinator.getView());
        } else if (selectedTab == TAB_PREFETCH) {
            mModel.setContentView(null);
        }
    }

    private void handleTabSelected(@TabType int selectedTab) {
        selectTab(selectedTab);

        @FilterType
        int filterType;
        if (selectedTab == TAB_FILES) {
            filterType = mChipsProvider.getSelectedFilter();
        } else {
            filterType = Filters.PREFETCHED;
        }

        for (Observer observer : mObserverList) observer.onFilterChanged(filterType);
    }

    private void handleChipSelected() {
        handleTabSelected(mModel.getSelectedTab());
    }
}