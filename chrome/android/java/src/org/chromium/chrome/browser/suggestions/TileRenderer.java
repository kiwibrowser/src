// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.os.AsyncTask;
import android.support.annotation.LayoutRes;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.favicon.IconType;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.RoundedIconGenerator;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Utility class that renders {@link Tile}s into a provided {@link ViewGroup}, creating and
 * manipulating the views as needed.
 */
public class TileRenderer {
    private static final String TAG = "TileRenderer";

    private static final int ICON_CORNER_RADIUS_DP = 4;
    private static final int ICON_TEXT_SIZE_DP = 20;
    private static final int ICON_MIN_SIZE_PX = 48;
    private static final int ICON_DECREASED_MIN_SIZE_PX = 24;

    private final Resources mResources;
    private final ImageFetcher mImageFetcher;
    private final RoundedIconGenerator mIconGenerator;

    @TileView.Style
    private final int mStyle;
    private final int mTitleLinesCount;
    private final int mDesiredIconSize;
    private final int mMinIconSize;

    @LayoutRes
    private final int mLayout;

    public TileRenderer(
            Context context, @TileView.Style int style, int titleLines, ImageFetcher imageFetcher) {
        mImageFetcher = imageFetcher;
        mStyle = style;
        mTitleLinesCount = titleLines;

        mResources = context.getResources();
        mDesiredIconSize = mResources.getDimensionPixelSize(R.dimen.tile_view_icon_size);
        int desiredIconSizeDp =
                Math.round(mDesiredIconSize / mResources.getDisplayMetrics().density);

        // On ldpi devices, mDesiredIconSize could be even smaller than the global limit.
        mMinIconSize = Math.min(mDesiredIconSize,
                useDecreasedMinSize() ? ICON_DECREASED_MIN_SIZE_PX : ICON_MIN_SIZE_PX);

        mLayout = getLayout();

        int cornerRadiusDp;
        if (style == TileView.Style.MODERN || style == TileView.Style.MODERN_CONDENSED) {
            cornerRadiusDp = desiredIconSizeDp / 2;
        } else {
            cornerRadiusDp = ICON_CORNER_RADIUS_DP;
        }

        int iconColor = ApiCompatibilityUtils.getColor(
                mResources, R.color.default_favicon_background_color);
        mIconGenerator = new RoundedIconGenerator(mResources, desiredIconSizeDp, desiredIconSizeDp,
                cornerRadiusDp, iconColor, ICON_TEXT_SIZE_DP);
    }

    private static boolean useDecreasedMinSize() {
        return FeatureUtilities.isChromeModernDesignEnabled();
    }

    /**
     * Renders tile views in the given {@link ViewGroup}, reusing existing tile views where
     * possible because view inflation and icon loading are slow.
     * @param parent The layout to render the tile views into.
     * @param sectionTiles Tiles to render.
     * @param setupDelegate Delegate used to setup callbacks and listeners for the new views.
     */
    public void renderTileSection(
            List<Tile> sectionTiles, ViewGroup parent, TileGroup.TileSetupDelegate setupDelegate) {
        // Map the old tile views by url so they can be reused later.
        Map<SiteSuggestion, TileView> oldTileViews = new HashMap<>();
        int childCount = parent.getChildCount();
        for (int i = 0; i < childCount; i++) {
            TileView tileView = (TileView) parent.getChildAt(i);
            oldTileViews.put(tileView.getData(), tileView);
        }

        // Remove all views from the layout because even if they are reused later they'll have to be
        // added back in the correct order.
        parent.removeAllViews();

        for (Tile tile : sectionTiles) {
            TileView tileView = oldTileViews.get(tile.getData());
            if (tileView == null) {
                tileView = buildTileView(tile, parent, setupDelegate);
            }

            parent.addView(tileView);
        }
    }

    /**
     * Inflates a new tile view, initializes it, and loads an icon for it.
     * @param tile The tile that holds the data to populate the new tile view.
     * @param parentView The parent of the new tile view.
     * @param setupDelegate The delegate used to setup callbacks and listeners for the new view.
     * @return The new tile view.
     */
    @VisibleForTesting
    TileView buildTileView(
            Tile tile, ViewGroup parentView, TileGroup.TileSetupDelegate setupDelegate) {
        TileView tileView = (TileView) LayoutInflater.from(parentView.getContext())
                                    .inflate(mLayout, parentView, false);
        tileView.initialize(tile, mTitleLinesCount, mStyle);

        // Note: It is important that the callbacks below don't keep a reference to the tile or
        // modify them as there is no guarantee that the same tile would be used to update the view.
        fetchIcon(tile.getData(), setupDelegate.createIconLoadCallback(tile));

        TileGroup.TileInteractionDelegate delegate = setupDelegate.createInteractionDelegate(tile);
        tileView.setOnClickListener(delegate);
        tileView.setOnCreateContextMenuListener(delegate);

        return tileView;
    }

    private void fetchIcon(
            final SiteSuggestion siteData, final LargeIconBridge.LargeIconCallback iconCallback) {
        if (siteData.whitelistIconPath.isEmpty()) {
            mImageFetcher.makeLargeIconRequest(siteData.url, mMinIconSize, iconCallback);
            return;
        }

        AsyncTask<Void, Void, Bitmap> task = new AsyncTask<Void, Void, Bitmap>() {
            @Override
            protected Bitmap doInBackground(Void... params) {
                Bitmap bitmap = BitmapFactory.decodeFile(siteData.whitelistIconPath);
                if (bitmap == null) {
                    Log.d(TAG, "Image decoding failed: %s", siteData.whitelistIconPath);
                }
                return bitmap;
            }

            @Override
            protected void onPostExecute(Bitmap icon) {
                if (icon == null) {
                    mImageFetcher.makeLargeIconRequest(siteData.url, mMinIconSize, iconCallback);
                } else {
                    iconCallback.onLargeIconAvailable(icon, Color.BLACK, false, IconType.INVALID);
                }
            }
        };
        task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void updateIcon(
            SiteSuggestion siteData, LargeIconBridge.LargeIconCallback iconCallback) {
        mImageFetcher.makeLargeIconRequest(siteData.url, mMinIconSize, iconCallback);
    }

    public void setTileIconFromBitmap(Tile tile, Bitmap icon) {
        RoundedBitmapDrawable roundedIcon = ViewUtils.createRoundedBitmapDrawable(icon,
                Math.round(ICON_CORNER_RADIUS_DP * mResources.getDisplayMetrics().density
                        * icon.getWidth() / mDesiredIconSize));
        roundedIcon.setAntiAlias(true);
        roundedIcon.setFilterBitmap(true);

        tile.setIcon(roundedIcon);
        tile.setType(TileVisualType.ICON_REAL);
    }

    public void setTileIconFromColor(Tile tile, int fallbackColor, boolean isFallbackColorDefault) {
        mIconGenerator.setBackgroundColor(fallbackColor);
        Bitmap icon = mIconGenerator.generateIconForUrl(tile.getUrl());
        tile.setIcon(new BitmapDrawable(mResources, icon));
        tile.setType(
                isFallbackColorDefault ? TileVisualType.ICON_DEFAULT : TileVisualType.ICON_COLOR);
    }

    @LayoutRes
    private int getLayout() {
        switch (mStyle) {
            case TileView.Style.CLASSIC:
                return R.layout.tile_view_classic;
            case TileView.Style.CLASSIC_CONDENSED:
                return R.layout.tile_view_classic_condensed;
            case TileView.Style.MODERN:
                return R.layout.tile_view_modern;
            case TileView.Style.MODERN_CONDENSED:
                return R.layout.tile_view_modern_condensed;
        }
        assert false;
        return 0;
    }
}
