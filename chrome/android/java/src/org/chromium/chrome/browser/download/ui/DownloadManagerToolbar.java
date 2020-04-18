// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MenuItem;
import android.view.View;
import android.widget.Spinner;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.ui.DownloadManagerUi.DownloadUiObserver;
import org.chromium.chrome.browser.widget.selection.SelectableListToolbar;

import java.util.List;

/**
 * Handles toolbar functionality for the {@link DownloadManagerUi}.
 */
public class DownloadManagerToolbar extends SelectableListToolbar<DownloadHistoryItemWrapper>
        implements DownloadUiObserver {
    private Spinner mSpinner;
    private DownloadManagerUi mManager;

    private int mInfoMenuItemId;

    public DownloadManagerToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);
        inflateMenu(R.menu.download_manager_menu);
    }

    /**
     * @param manager The {@link DownloadManagerUi} associated with this toolbar.
     */
    public void setManager(DownloadManagerUi manager) {
        mManager = manager;
    }

    /**
     * Initializes UI elements in download toolbar.
     * @param adapter The adapter associated with the spinner.
     */
    public void initialize(FilterAdapter adapter) {
        // Initialize the spinner.
        mSpinner = findViewById(R.id.spinner);
        mSpinner.setAdapter(adapter);
        mSpinner.setOnItemSelectedListener(adapter);
    }

    /**
     * Removes the close button from the toolbar.
     */
    public void removeCloseButton() {
        getMenu().removeItem(R.id.close_menu_id);
    }

    @Override
    public void onFilterChanged(int filter) {
        mSpinner.setSelection(filter);
    }

    @Override
    public void onSelectionStateChange(List<DownloadHistoryItemWrapper> selectedItems) {
        boolean wasSelectionEnabled = mIsSelectionEnabled;
        super.onSelectionStateChange(selectedItems);

        mSpinner.setVisibility((mIsSelectionEnabled || mIsSearching) ? GONE : VISIBLE);
        if (mIsSelectionEnabled) {
            int numSelected = mSelectionDelegate.getSelectedItems().size();

            // If the share or delete menu items are shown in the overflow menu instead of as an
            // action, there may not be views associated with them.
            View shareButton = findViewById(R.id.selection_mode_share_menu_id);
            if (shareButton != null) {
                shareButton.setContentDescription(getResources().getQuantityString(
                        R.plurals.accessibility_share_selected_items,
                                numSelected, numSelected));
            }

            View deleteButton = findViewById(R.id.selection_mode_delete_menu_id);
            if (deleteButton != null) {
                deleteButton.setContentDescription(getResources().getQuantityString(
                        R.plurals.accessibility_remove_selected_items,
                        numSelected, numSelected));
            }

            if (!wasSelectionEnabled) {
                RecordUserAction.record("Android.DownloadManager.SelectionEstablished");
            }
        }
    }

    @Override
    protected void onDataChanged(int numItems) {
        super.onDataChanged(numItems);
        MenuItem item = getMenu().findItem(mInfoMenuItemId);
        if (item != null) item.setVisible(!mIsSearching && !mIsSelectionEnabled && numItems > 0);
    }

    @Override
    public void onManagerDestroyed() {
        mSpinner.setAdapter(null);
    }

    @Override
    protected void showNormalView() {
        super.showNormalView();
        mManager.updateInfoButtonVisibility();
    }

    @Override
    public void showSearchView() {
        super.showSearchView();
        mSpinner.setVisibility(GONE);
    }

    @Override
    public void hideSearchView() {
        super.hideSearchView();
        mSpinner.setVisibility(VISIBLE);
    }

    @Override
    public void setInfoMenuItem(int infoMenuItemId) {
        super.setInfoMenuItem(infoMenuItemId);
        mInfoMenuItemId = infoMenuItemId;
    }

    /** Returns the {@link Spinner}. */
    @VisibleForTesting
    public Spinner getSpinnerForTests() {
        return mSpinner;
    }
}
