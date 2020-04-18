// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.home.filter;

import android.os.Handler;

import org.chromium.base.ObserverList;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.home.filter.Filters.FilterType;
import org.chromium.chrome.browser.download.home.filter.chips.Chip;
import org.chromium.chrome.browser.download.home.filter.chips.ChipsProvider;
import org.chromium.components.offline_items_collection.OfflineItem;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * A {@link ChipsProvider} implementation that wraps a subset of {@link Filters} to be used as a
 * chip selector for filtering downloads.
 */
public class FilterChipsProvider implements ChipsProvider, OfflineItemFilterObserver {
    private static final int INVALID_INDEX = -1;

    /** A delegate responsible for handling UI actions like selecting filters. */
    public interface Delegate {
        /** Called when the selected filter has changed. */
        void onFilterSelected(@FilterType int filterType);
    }

    private final Delegate mDelegate;
    private final OfflineItemFilterSource mSource;

    private final Handler mHandler = new Handler();
    private final ObserverList<Observer> mObservers = new ObserverList<ChipsProvider.Observer>();
    private final List<Chip> mSortedChips = new ArrayList<>();

    /** Builds a new FilterChipsBackend. */
    public FilterChipsProvider(Delegate delegate, OfflineItemFilterSource source) {
        mDelegate = delegate;
        mSource = source;

        Chip noneChip = new Chip(Filters.NONE, R.string.download_manager_ui_all_downloads,
                R.string.download_manager_ui_all_downloads, R.drawable.ic_play_arrow_white_24dp,
                () -> onChipSelected(Filters.NONE));
        Chip videosChip = new Chip(Filters.VIDEOS, R.string.download_manager_ui_video,
                R.string.download_manager_ui_video, R.drawable.ic_play_arrow_white_24dp,
                () -> onChipSelected(Filters.VIDEOS));
        Chip musicChip = new Chip(Filters.MUSIC, R.string.download_manager_ui_audio,
                R.string.download_manager_ui_audio, R.drawable.ic_play_arrow_white_24dp,
                () -> onChipSelected(Filters.MUSIC));
        Chip imagesChip = new Chip(Filters.IMAGES, R.string.download_manager_ui_images,
                R.string.download_manager_ui_images, R.drawable.ic_play_arrow_white_24dp,
                () -> onChipSelected(Filters.IMAGES));
        Chip sitesChip = new Chip(Filters.SITES, R.string.download_manager_ui_pages,
                R.string.download_manager_ui_pages, R.drawable.ic_play_arrow_white_24dp,
                () -> onChipSelected(Filters.SITES));
        Chip otherChip = new Chip(Filters.OTHER, R.string.download_manager_ui_other,
                R.string.download_manager_ui_other, R.drawable.ic_play_arrow_white_24dp,
                () -> onChipSelected(Filters.OTHER));

        mSortedChips.add(noneChip);
        mSortedChips.add(videosChip);
        mSortedChips.add(musicChip);
        mSortedChips.add(imagesChip);
        mSortedChips.add(sitesChip);
        mSortedChips.add(otherChip);

        mSource.addObserver(this);
        generateFilterStates();
    }

    /**
     * Sets whether or not a filter is enabled.
     * @param type    The type of filter to enable.
     * @param enabled Whether or not that filter is enabled.
     */
    public void setFilterEnabled(@FilterType int type, boolean enabled) {
        int chipIndex = getChipIndex(type);
        if (chipIndex == INVALID_INDEX) return;
        Chip chip = mSortedChips.get(chipIndex);

        if (enabled == chip.enabled) return;
        chip.enabled = enabled;
        for (Observer observer : mObservers) observer.onChipChanged(chipIndex, chip);
    }

    /**
     * Sets the filter that is currently selected.
     * @param type The type of filter to select.
     */
    public void setFilterSelected(@FilterType int type) {
        for (int i = 0; i < mSortedChips.size(); i++) {
            Chip chip = mSortedChips.get(i);
            boolean willSelect = chip.id == type;

            // Early out if we're already selecting the appropriate Chip type.
            if (chip.selected && willSelect) return;
            if (chip.selected == willSelect) continue;
            chip.selected = willSelect;

            for (Observer observer : mObservers) observer.onChipChanged(i, chip);
        }
    }

    /**
     * @return The {@link FilterType} of the selected filter or {@link Filters#NONE} if no filter
     * is selected.
     */
    public @FilterType int getSelectedFilter() {
        for (Chip chip : mSortedChips) {
            if (chip.selected) return chip.id;
        }

        return Filters.NONE;
    }

    // ChipsProvider implementation.
    @Override
    public void addObserver(Observer observer) {
        mObservers.addObserver(observer);
    }

    @Override
    public void removeObserver(Observer observer) {
        mObservers.removeObserver(observer);
    }

    @Override
    public List<Chip> getChips() {
        return mSortedChips;
    }

    // OfflineItemFilterObserver implementation.
    // Post calls to generateFilterStates() avoid re-entrancy.
    @Override
    public void onItemsAdded(Collection<OfflineItem> items) {
        mHandler.post(() -> generateFilterStates());
    }

    @Override
    public void onItemsRemoved(Collection<OfflineItem> items) {
        mHandler.post(() -> generateFilterStates());
    }

    @Override
    public void onItemUpdated(OfflineItem oldItem, OfflineItem item) {
        if (oldItem.filter == item.filter) return;
        mHandler.post(() -> generateFilterStates());
    }

    private void generateFilterStates() {
        // Build a set of all filter types in our data set.
        Set</* @FilterType */ Integer> filters = new HashSet<>();
        filters.add(Filters.NONE);
        for (OfflineItem item : mSource.getItems()) {
            filters.add(Filters.fromOfflineItem(item.filter));
        }

        // Set the enabled states correctly for all filter types.
        for (Chip chip : mSortedChips) setFilterEnabled(chip.id, filters.contains(chip.id));

        // Validate that selection is on a valid type.
        for (Chip chip : mSortedChips) {
            if (chip.selected && !chip.enabled) {
                onChipSelected(Filters.NONE);
                break;
            }
        }
    }

    private void onChipSelected(@FilterType int id) {
        setFilterSelected(id);
        mDelegate.onFilterSelected(id);
    }

    private int getChipIndex(@FilterType int type) {
        for (int i = 0; i < mSortedChips.size(); i++) {
            if (mSortedChips.get(i).id == type) return i;
        }

        return INVALID_INDEX;
    }
}