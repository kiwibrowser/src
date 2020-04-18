// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import org.chromium.base.ObserverList;
import org.chromium.chrome.browser.download.ui.DownloadHistoryAdapter.SubsectionHeader;
import org.chromium.chrome.browser.widget.selection.SelectionDelegate;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Separately maintains the selected state of the {@link SubsectionHeader}s in addition to the other
 * selected items. Notifies the {@link OfflineGroupHeaderView}s about any change in the selected
 * state of the headers.
 */
public class DownloadItemSelectionDelegate extends SelectionDelegate<DownloadHistoryItemWrapper> {
    /** Observer interface to be notified of selection changes for {@link SubsectionHeader}s. */
    public interface SubsectionHeaderSelectionObserver {
        /**
         * Called when the set of selected subsection headers has changed.
         * @param selectedHeaders The set of currently selected headers.
         */
        public void onSubsectionHeaderSelectionStateChanged(Set<SubsectionHeader> selectedHeaders);
    }

    private final Set<SubsectionHeader> mSelectedHeaders = new HashSet<>();
    private final ObserverList<SubsectionHeaderSelectionObserver> mObservers = new ObserverList<>();
    private DownloadHistoryAdapter mAdapter;

    /**
     * Adds an observer to be notified of selection changes for subsection headers.
     * @param observer The observer to add.
     */
    public void addObserver(SubsectionHeaderSelectionObserver observer) {
        mObservers.addObserver(observer);
    }

    /**
     * Removes an observer.
     * @param observer The observer to remove.
     */
    public void removeObserver(SubsectionHeaderSelectionObserver observer) {
        mObservers.removeObserver(observer);
    }

    /**
     * Initializes the selection delegate with the required dependencies. Registers an observer to
     * be notified of selection changes for the download items of the adapter.
     * @param adapter The associated download history adapter.
     */
    public void initialize(DownloadHistoryAdapter adapter) {
        mAdapter = adapter;
        addObserver(new SelectionObserver<DownloadHistoryItemWrapper>() {
            @Override
            public void onSelectionStateChange(List<DownloadHistoryItemWrapper> selectedItems) {
                for (SubsectionHeader header : mAdapter.getSubsectionHeaders()) {
                    boolean isChecked = true;

                    // A header should get selected if all the associated items are selected
                    // irrespective of whether it is currently expanded or collapsed.
                    for (DownloadHistoryItemWrapper subItem : header.getItems()) {
                        if (!selectedItems.contains(subItem)) {
                            isChecked = false;
                            break;
                        }
                    }
                    setSelectionForHeader(header, isChecked);
                }

                notifySubsectionHeaderSelectionStateChanged();
            }
        });
    }

    /**
     * True if the header is currently selected. False otherwise.
     * @param header The given header.
     * @return Whether the header is selected.
     */
    public boolean isHeaderSelected(SubsectionHeader header) {
        return mSelectedHeaders.contains(header);
    }

    /**
     * Toggles selection for a given subsection and sets the associated items to the correct
     * selection state.
     * @param header The header for the subsection being toggled.
     * @return True if the header is selected after the toggle, false otherwise.
     */
    public boolean toggleSelectionForSubsection(SubsectionHeader header) {
        boolean newSelectedState = !isHeaderSelected(header);
        setSelectionForHeader(header, newSelectedState);

        for (DownloadHistoryItemWrapper item : header.getItems()) {
            if (newSelectedState != isItemSelected(item)) {
                toggleSelectionForItem(item);
            }
        }

        return isHeaderSelected(header);
    }

    /**
     * Sets the selection state for a given header. Doesn't affect the associated items. This method
     * is supposed to be called if the header needs to be toggled as a result of selecting or
     * deselecting one of the associated items. In case of a long press on the header, don't call
     * this method, call {@link #toggleSelectionForSubsection(SubsectionHeader)} instead directly.
     * @param header The given {@link SubsectionHeader}.
     * @param selected The new selected state.
     */
    public void setSelectionForHeader(SubsectionHeader header, boolean selected) {
        if (selected) {
            mSelectedHeaders.add(header);
        } else {
            mSelectedHeaders.remove(header);
        }
    }

    private void notifySubsectionHeaderSelectionStateChanged() {
        for (SubsectionHeaderSelectionObserver observer : mObservers) {
            observer.onSubsectionHeaderSelectionStateChanged(mSelectedHeaders);
        }
    }
}
