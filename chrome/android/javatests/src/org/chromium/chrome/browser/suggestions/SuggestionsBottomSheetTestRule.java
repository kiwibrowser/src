// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;

import android.support.v7.widget.RecyclerView;

import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.NewTabPageAdapter;
import org.chromium.chrome.test.BottomSheetTestRule;
import org.chromium.chrome.test.util.browser.RecyclerViewTestUtils;
import org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils;

/**
 * Junit4 rule for testing suggestions in the Chrome Home "Home" sheet.
 */
public class SuggestionsBottomSheetTestRule extends BottomSheetTestRule {
    /**
     * @return The {@link SuggestionsRecyclerView} for this sheet.
     */
    public SuggestionsRecyclerView getRecyclerView() {
        SuggestionsRecyclerView recyclerView =
                getBottomSheetContent().getContentView().findViewById(
                        org.chromium.chrome.R.id.recycler_view);
        assertNotNull(recyclerView);
        return recyclerView;
    }

    /**
     * @return The {@link NewTabPageAdapter} for this sheet.
     */
    public NewTabPageAdapter getAdapter() {
        return getRecyclerView().getNewTabPageAdapter();
    }

    /**
     * @param itemViewType The type of item to find.
     * @return The position of the first item of the given type.
     */
    public int getFirstPositionForType(@ItemViewType int itemViewType) {
        return getAdapter().getFirstPositionForType(itemViewType);
    }

    /**
     * Finds and scrolls to the first item of the given type.
     * @param type The type of item to find and scroll to.
     * @return the ViewHolder for the given {@code position}.
     */
    public RecyclerView.ViewHolder scrollToFirstItemOfType(@ItemViewType int type) {
        int position = getFirstPositionForType(type);
        assertNotEquals("Scroll target of type " + type + " not found\n"
                        + ContentSuggestionsTestUtils.stringify(getAdapter().getRootForTesting()),
                RecyclerView.NO_POSITION, position);

        return RecyclerViewTestUtils.scrollToView(getRecyclerView(), position);
    }
}
