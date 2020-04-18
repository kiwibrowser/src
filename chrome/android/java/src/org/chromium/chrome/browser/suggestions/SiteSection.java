// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.annotation.LayoutRes;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.cards.ItemViewType;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.ntp.cards.NodeVisitor;
import org.chromium.chrome.browser.ntp.cards.OptionalLeaf;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * The model and controller for a group of site suggestions.
 */
public class SiteSection extends OptionalLeaf implements TileGroup.Observer {
    /**
     * The maximum number of tiles to try and fit in a row. On smaller screens, there may not be
     * enough space to fit all of them.
     */
    private static final int MAX_TILE_COLUMNS = 4;

    private final TileGroup mTileGroup;
    private final TileRenderer mTileRenderer;

    public static ViewGroup inflateSiteSection(ViewGroup parent) {
        return (ViewGroup) LayoutInflater.from(parent.getContext())
                .inflate(getLayout(), parent, false);
    }

    public static SiteSectionViewHolder createViewHolder(ViewGroup view, UiConfig uiConfig) {
        return new TileGridViewHolder(view, getMaxTileRows(), MAX_TILE_COLUMNS, uiConfig);
    }

    public SiteSection(SuggestionsUiDelegate uiDelegate, ContextMenuManager contextMenuManager,
            TileGroup.Delegate tileGroupDelegate, OfflinePageBridge offlinePageBridge,
            UiConfig uiConfig) {
        mTileRenderer = new TileRenderer(ContextUtils.getApplicationContext(),
                SuggestionsConfig.getTileStyle(uiConfig), getTileTitleLines(),
                uiDelegate.getImageFetcher());
        mTileGroup = new TileGroup(mTileRenderer, uiDelegate, contextMenuManager, tileGroupDelegate,
                /* observer = */ this, offlinePageBridge);
        mTileGroup.startObserving(MAX_TILE_COLUMNS * getMaxTileRows());
    }

    @Override
    @ItemViewType
    protected int getItemViewType() {
        return ItemViewType.SITE_SECTION;
    }

    @Override
    protected void onBindViewHolder(NewTabPageViewHolder holder) {
        SiteSectionViewHolder siteSectionView = (SiteSectionViewHolder) holder;
        siteSectionView.bindDataSource(mTileGroup, mTileRenderer);
        siteSectionView.refreshData();
    }

    @Override
    protected void visitOptionalItem(NodeVisitor visitor) {
        visitor.visitTileGrid();
    }

    @Override
    public void onTileDataChanged() {
        setVisibilityInternal(!mTileGroup.isEmpty());
        if (!isVisible()) return;
        notifyItemChanged(0, (holder) -> ((SiteSectionViewHolder) holder).refreshData());
    }

    @Override
    public void onTileCountChanged() {
        onTileDataChanged();
    }

    @Override
    public void onTileIconChanged(Tile tile) {
        if (!isVisible()) return;
        notifyItemChanged(0, (holder) -> ((SiteSectionViewHolder) holder).updateIconView(tile));
    }

    @Override
    public void onTileOfflineBadgeVisibilityChanged(Tile tile) {
        if (!isVisible()) return;
        notifyItemChanged(0, (holder) -> ((SiteSectionViewHolder) holder).updateOfflineBadge(tile));
    }

    public TileGroup getTileGroup() {
        return mTileGroup;
    }

    private static int getMaxTileRows() {
        return 2;
    }

    private static int getTileTitleLines() {
        return 1;
    }

    @LayoutRes
    private static int getLayout() {
        if (SuggestionsConfig.useModernLayout()) {
            return R.layout.suggestions_site_tile_grid_modern;
        }
        return R.layout.suggestions_site_tile_grid;
    }
}
