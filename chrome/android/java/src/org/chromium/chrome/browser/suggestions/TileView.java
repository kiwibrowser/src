// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.content.res.Resources;
import android.support.annotation.IntDef;
import android.util.AttributeSet;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.TitleUtil;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * The view for a site suggestion tile. Displays the title of the site beneath a large icon. If a
 * large icon isn't available, displays a rounded rectangle with a single letter in its place.
 */
public class TileView extends FrameLayout {
    @IntDef({Style.CLASSIC, Style.CLASSIC_CONDENSED, Style.MODERN, Style.MODERN_CONDENSED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Style {
        int CLASSIC = 0;
        int CLASSIC_CONDENSED = 1;
        int MODERN = 2;
        int MODERN_CONDENSED = 3;
    }

    /** The url currently associated to this tile. */
    private SiteSuggestion mSiteData;

    private TextView mTitleView;
    private ImageView mIconView;
    private ImageView mBadgeView;

    /**
     * Constructor for inflating from XML.
     */
    public TileView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mTitleView = findViewById(R.id.tile_view_title);
        mIconView = findViewById(R.id.tile_view_icon);
        mBadgeView = findViewById(R.id.offline_badge);
    }

    /**
     * Initializes the view using the data held by {@code tile}. This should be called immediately
     * after inflation.
     * @param tile The tile that holds the data to populate this view.
     * @param titleLines The number of text lines to use for the tile title.
     * @param tileStyle The visual style of the tile.
     */
    public void initialize(Tile tile, int titleLines, @Style int tileStyle) {
        mTitleView.setLines(titleLines);
        mSiteData = tile.getData();
        mTitleView.setText(TitleUtil.getTitleForDisplay(tile.getTitle(), tile.getUrl()));
        renderOfflineBadge(tile);
        renderIcon(tile);
    }

    /** @return The url associated with this view. */
    public String getUrl() {
        return mSiteData.url;
    }

    public SiteSuggestion getData() {
        return mSiteData;
    }

    /** @return The {@link TileSource} of the tile represented by this TileView */
    public int getTileSource() {
        return mSiteData.source;
    }

    /**
     * Renders the icon held by the {@link Tile} or clears it from the view if the icon is null.
     */
    public void renderIcon(Tile tile) {
        mIconView.setImageDrawable(tile.getIcon());
        if (!SuggestionsConfig.useModernLayout()) return;

        // Slightly enlarge the monogram in the modern layout.
        MarginLayoutParams params = (MarginLayoutParams) mIconView.getLayoutParams();
        Resources resources = getResources();
        if (tile.getType() == TileVisualType.ICON_COLOR
                || tile.getType() == TileVisualType.ICON_DEFAULT) {
            params.width = resources.getDimensionPixelSize(R.dimen.tile_view_monogram_size_modern);
            params.height = resources.getDimensionPixelSize(R.dimen.tile_view_monogram_size_modern);
            params.topMargin =
                    resources.getDimensionPixelSize(R.dimen.tile_view_monogram_margin_top_modern);
        } else {
            params.width = resources.getDimensionPixelSize(R.dimen.tile_view_icon_size_modern);
            params.height = resources.getDimensionPixelSize(R.dimen.tile_view_icon_size_modern);
            params.topMargin =
                    resources.getDimensionPixelSize(R.dimen.tile_view_icon_margin_top_modern);
        }
        mIconView.setLayoutParams(params);
    }

    /** Shows or hides the offline badge to reflect the offline availability of the {@link Tile}. */
    public void renderOfflineBadge(Tile tile) {
        mBadgeView.setVisibility(tile.isOfflineAvailable() ? VISIBLE : GONE);
    }
}
