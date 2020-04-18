// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home;

import android.content.Context;
import android.view.View;

import org.chromium.chrome.browser.download.home.filter.DeleteUndoOfflineItemFilter;
import org.chromium.chrome.browser.download.home.filter.FilterCoordinator;
import org.chromium.chrome.browser.download.home.filter.SearchOfflineItemFilter;
import org.chromium.chrome.browser.download.home.filter.TypeOfflineItemFilter;
import org.chromium.chrome.browser.download.home.list.DateOrderedListModel;
import org.chromium.chrome.browser.download.home.list.DateOrderedListMutator;
import org.chromium.chrome.browser.download.items.OfflineContentAggregatorFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.offline_items_collection.OfflineContentProvider;

/**
 * The top level coordinator for the download home UI.  This is currently an in progress class and
 * is not fully fleshed out yet.
 */
public class DownloadManagerCoordinatorImpl {
    private final OfflineContentProvider mProvider;
    private final FilterCoordinator mFilterCoordinator;
    private final OfflineItemSource mSource;
    private final DateOrderedListMutator mMutator;
    private final DateOrderedListModel mModel;

    public DownloadManagerCoordinatorImpl(Profile profile, Context context) {
        mProvider = OfflineContentAggregatorFactory.forProfile(profile);
        mSource = new OfflineItemSource(mProvider);

        // TODO(dtrainor): This is just a temporary placeholder to figure out how these pieces
        // hook up.  Much of this probably belongs in the mediator.
        DeleteUndoOfflineItemFilter deleteFilter = new DeleteUndoOfflineItemFilter(mSource);
        TypeOfflineItemFilter typeFilter = new TypeOfflineItemFilter(deleteFilter);
        SearchOfflineItemFilter searchFilter = new SearchOfflineItemFilter(typeFilter);
        mModel = new DateOrderedListModel();
        mMutator = new DateOrderedListMutator(searchFilter, mModel);

        mFilterCoordinator = new FilterCoordinator(context, deleteFilter);
        mFilterCoordinator.addObserver(filter -> typeFilter.onFilterSelected(filter));

        // TODO(dtrainor): mFilteredItems will be used to generate the UI.
    }

    /**
     * @return The {@link View} representing downloads home.
     */
    public View getView() {
        // TODO(dtrainor): Return the real View.
        return mFilterCoordinator.getView();
    }
}