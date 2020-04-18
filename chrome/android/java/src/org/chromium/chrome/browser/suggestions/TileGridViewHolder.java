// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.Resources;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.displaystyle.MarginResizer;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

import java.util.List;
/**
 * A {@link SiteSectionViewHolder} specialised in displaying sites as a simple grid of tiles,
 * through
 * {@link TileGridLayout}.
 */
public class TileGridViewHolder extends SiteSectionViewHolder {
    private final TileGridLayout mSectionView;
    private final MarginResizer mMarginResizer;

    public TileGridViewHolder(ViewGroup view, int maxRows, int maxColumns, UiConfig uiConfig) {
        super(view);

        mSectionView = (TileGridLayout) itemView;
        mSectionView.setMaxRows(maxRows);
        mSectionView.setMaxColumns(maxColumns);

        if (SuggestionsConfig.useModernLayout()) {
            Resources res = itemView.getResources();
            int defaultLateralMargin =
                    res.getDimensionPixelSize(R.dimen.tile_grid_layout_padding_start);
            int wideLateralMargin =
                    res.getDimensionPixelSize(R.dimen.ntp_wide_card_lateral_margins);
            mMarginResizer =
                    new MarginResizer(itemView, uiConfig, defaultLateralMargin, wideLateralMargin);
        } else {
            mMarginResizer = null;
        }
    }

    @Override
    public void refreshData() {
        assert mTileGroup.getTileSections().size() == 1;
        List<Tile> tiles = mTileGroup.getTileSections().get(TileSectionType.PERSONALIZED);
        assert tiles != null;

        mTileRenderer.renderTileSection(tiles, mSectionView, mTileGroup.getTileSetupDelegate());
        mTileGroup.notifyTilesRendered();
    }

    @Override
    protected TileView findTileView(SiteSuggestion data) {
        return mSectionView.getTileView(data);
    }

    @Override
    public void bindDataSource(TileGroup tileGroup, TileRenderer tileRenderer) {
        super.bindDataSource(tileGroup, tileRenderer);
        if (mMarginResizer != null) mMarginResizer.attach();
    }

    @Override
    public void recycle() {
        super.recycle();
        if (mMarginResizer != null) mMarginResizer.detach();
    }
}
