// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.design.widget.TabLayout;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callback;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.ContextMenuDialog;

import java.util.ArrayList;
import java.util.List;

/**
 * A custom dialog that separates each group into separate tabs. It uses a dialog instead.
 */
public class TabularContextMenuUi implements ContextMenuUi, AdapterView.OnItemClickListener {
    private ContextMenuDialog mContextMenuDialog;
    private Callback<Integer> mCallback;
    private int mMenuItemHeight;
    private ImageView mHeaderImageView;
    private Callback<Boolean> mOnShareItemClicked;
    private View mPagerView;
    private float mTopContentOffsetPx;

    public TabularContextMenuUi(Callback<Boolean> onShareItemClicked) {
        mOnShareItemClicked = onShareItemClicked;
    }

    @Override
    public void displayMenu(final Activity activity, ContextMenuParams params,
            List<Pair<Integer, List<ContextMenuItem>>> items, Callback<Integer> onItemClicked,
            final Runnable onMenuShown, final Runnable onMenuClosed) {
        mCallback = onItemClicked;

        float density = Resources.getSystem().getDisplayMetrics().density;
        final float touchPointXPx = params.getTriggeringTouchXDp() * density;
        final float touchPointYPx = params.getTriggeringTouchYDp() * density;

        mContextMenuDialog =
                createContextMenuDialog(activity, params, items, touchPointXPx, touchPointYPx);

        mContextMenuDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialogInterface) {
                onMenuShown.run();
            }
        });

        mContextMenuDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialogInterface) {
                onMenuClosed.run();
            }
        });

        mContextMenuDialog.show();
    }

    /**
     * Returns the fully complete dialog based off the params and the itemGroups.
     *
     * @param activity Used to inflate the dialog.
     * @param params Used to get the header title.
     * @param itemGroups If there is more than one group it will create a paged view.
     * @param touchPointYPx The x-coordinate of the touch that triggered the context menu.
     * @param touchPointXPx The y-coordinate of the touch that triggered the context menu.
     * @return Returns a final dialog that does not have a background can be displayed using
     *         {@link AlertDialog#show()}.
     */
    private ContextMenuDialog createContextMenuDialog(Activity activity, ContextMenuParams params,
            List<Pair<Integer, List<ContextMenuItem>>> itemGroups, float touchPointXPx,
            float touchPointYPx) {
        View view = LayoutInflater.from(activity).inflate(R.layout.tabular_context_menu, null);

        mPagerView = initPagerView(activity, params, itemGroups,
                (TabularContextMenuViewPager) view.findViewById(R.id.custom_pager));

        final ContextMenuDialog dialog = new ContextMenuDialog(activity, R.style.DialogWhenLarge,
                touchPointXPx, touchPointYPx, mTopContentOffsetPx, mPagerView);
        dialog.setContentView(view);

        return dialog;
    }

    /**
     * Creates a ViewPageAdapter based off the given list of views.
     *
     * @param activity Used to inflate the new ViewPager.
     * @param params Used to get the header text.
     * @param itemGroups The list of views to put into the ViewPager. The string is the title of the
     *                   tab.
     * @param viewPager The {@link TabularContextMenuViewPager} to initialize.
     * @return Returns a complete tabular context menu view.
     */
    @VisibleForTesting
    View initPagerView(Activity activity, ContextMenuParams params,
            List<Pair<Integer, List<ContextMenuItem>>> itemGroups,
            TabularContextMenuViewPager viewPager) {
        List<Pair<String, ViewGroup>> viewGroups = new ArrayList<>();
        for (int i = 0; i < itemGroups.size(); i++) {
            Pair<Integer, List<ContextMenuItem>> itemGroup = itemGroups.get(i);
            // TODO(tedchoc): Pass the ContextMenuGroup identifier to determine if it's an image.
            boolean isImageTab = itemGroup.first == R.string.contextmenu_image_title;
            viewGroups.add(new Pair<>(activity.getString(itemGroup.first),
                    createContextMenuPageUi(activity, params, itemGroup.second, isImageTab)));
        }

        viewPager.setAdapter(new TabularContextMenuPagerAdapter(viewGroups));
        TabLayout tabLayout = (TabLayout) viewPager.findViewById(R.id.tab_layout);
        if (itemGroups.size() <= 1) {
            tabLayout.setVisibility(View.GONE);
        } else {
            tabLayout.setBackgroundResource(R.drawable.grey_with_top_rounded_corners);
            tabLayout.setupWithViewPager(viewPager);
        }

        return viewPager;
    }

    /**
     * Creates the view of a context menu. Based off the Context Type, it'll adjust the list of
     * items and display only the ones that'll be on that specific group.
     *
     * @param activity Used to get the resources of an item.
     * @param params used to create the header text.
     * @param items A set of Items to display in a context menu. Filtered based off the type.
     * @param isImage Whether or not the view should have an image layout or not.
     * @param maxCount The maximum amount of {@link ContextMenuItem}s that could exist in this view
     *                 or any other views calculated in the context menu. Used to estimate the size
     *                 of the list.
     * @return Returns a filled LinearLayout with all the context menu items.
     */
    @VisibleForTesting
    ViewGroup createContextMenuPageUi(Activity activity, ContextMenuParams params,
            List<ContextMenuItem> items, boolean isImage) {
        ViewGroup baseLayout = (ViewGroup) LayoutInflater.from(activity).inflate(
                R.layout.tabular_context_menu_page, null);
        ListView listView = (ListView) baseLayout.findViewById(R.id.selectable_items);

        displayHeaderIfVisibleItems(params, baseLayout);
        if (isImage) {
            // #displayHeaderIfVisibleItems() sets these two views to GONE if the header text is
            // empty but they should still be visible because we have an image to display.
            baseLayout.findViewById(R.id.context_header_layout).setVisibility(View.VISIBLE);
            baseLayout.findViewById(R.id.context_divider).setVisibility(View.VISIBLE);
            displayImageHeader(baseLayout, params, activity.getResources());
        }

        // Set the list adapter and get the height to display it appropriately in a dialog.
        Callback<Boolean> onDirectShare = new Callback<Boolean>() {
            @Override
            public void onResult(Boolean isShareLink) {
                mOnShareItemClicked.onResult(isShareLink);
                mContextMenuDialog.dismiss();
            }
        };
        TabularContextMenuListAdapter listAdapter =
                new TabularContextMenuListAdapter(items, activity, onDirectShare);
        ViewGroup.LayoutParams layoutParams = listView.getLayoutParams();
        layoutParams.height = measureApproximateListViewHeight(listView, listAdapter, items.size());
        listView.setLayoutParams(layoutParams);
        listView.setAdapter(listAdapter);
        listView.setOnItemClickListener(this);

        return baseLayout;
    }

    private void displayHeaderIfVisibleItems(ContextMenuParams params, ViewGroup baseLayout) {
        String headerText = ChromeContextMenuPopulator.createHeaderText(params);
        final TextView headerTextView =
                (TextView) baseLayout.findViewById(R.id.context_header_text);
        if (TextUtils.isEmpty(headerText)) {
            MarginLayoutParams marginParams =
                    (MarginLayoutParams) baseLayout.findViewById(R.id.context_header_image)
                            .getLayoutParams();
            marginParams.bottomMargin = marginParams.topMargin;
            return;
        }
        headerTextView.setVisibility(View.VISIBLE);
        headerTextView.setText(headerText);
        headerTextView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (headerTextView.getMaxLines() == Integer.MAX_VALUE) {
                    headerTextView.setMaxLines(1);
                    headerTextView.setEllipsize(TextUtils.TruncateAt.END);
                } else {
                    headerTextView.setMaxLines(Integer.MAX_VALUE);
                    headerTextView.setEllipsize(null);
                }
            }
        });
    }

    private void displayImageHeader(
            ViewGroup baseLayout, ContextMenuParams params, Resources resources) {
        mHeaderImageView = (ImageView) baseLayout.findViewById(R.id.context_header_image);
        TextView headerTextView = (TextView) baseLayout.findViewById(R.id.context_header_text);
        // We'd prefer the header text is the title text instead of the link text for images.
        String headerText = params.getTitleText();
        if (!TextUtils.isEmpty(headerText)) {
            headerTextView.setText(headerText);
        }
        setBackgroundForImageView(mHeaderImageView, resources);
    }

    /**
     * This creates a checkerboard style background displayed before the image is shown.
     */
    private void setBackgroundForImageView(ImageView imageView, Resources resources) {
        Drawable drawable =
                ApiCompatibilityUtils.getDrawable(resources, R.drawable.checkerboard_background);
        Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        BitmapDrawable bm = new BitmapDrawable(resources, bitmap);
        bm.setTileModeXY(Shader.TileMode.REPEAT, Shader.TileMode.REPEAT);
        imageView.setVisibility(View.VISIBLE);
        imageView.setBackground(bm);
    }

    /**
     * To save time measuring the height, this method gets an item if the height has not been
     * previous measured and multiplies it by count of the total amount of items. It is fine if the
     * height too small as the ListView will scroll through the other values.
     *
     * @param listView The ListView to measure the surrounding padding.
     * @param listAdapter The adapter which contains the items within the list.
     * @return Returns the combined height of the padding of the ListView and the approximate height
     *         of the ListView based off the an item.
     */
    private int measureApproximateListViewHeight(
            ListView listView, BaseAdapter listAdapter, int itemCount) {
        int totalHeight = listView.getPaddingTop() + listView.getPaddingBottom();
        if (mMenuItemHeight == 0 && !listAdapter.isEmpty()) {
            View view = listAdapter.getView(0, null, listView);
            view.measure(View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
                    View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
            mMenuItemHeight = view.getMeasuredHeight();
        }
        return totalHeight + mMenuItemHeight * itemCount;
    }

    /**
     * When an thumbnail is retrieved for the header of an image, this will set the header to that
     * particular bitmap.
     */
    public void onImageThumbnailRetrieved(Bitmap bitmap) {
        if (mHeaderImageView != null) {
            if (bitmap != null) {
                mHeaderImageView.setImageBitmap(bitmap);
            } else {
                mHeaderImageView.setImageResource(R.drawable.sad_tab);
            }
        }
    }

    @Override
    public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
        mContextMenuDialog.dismiss();
        mCallback.onResult((int) id);
    }

    /**
     * Set the content offset.
     *
     * This should be set separately ahead of calling {@link displayMenu()}
     * since it cannot be passed to the method.
     * @param topContentOffsetPx y content offset from the top.
     */
    public void setTopContentOffsetY(float topContentOffsetPx) {
        mTopContentOffsetPx = topContentOffsetPx;
    }

    /**
     * Calculates the maximum possible width of the thumbnail in pixels.
     * @param res The resources from where to pull the device dimensions.
     * @return The calculated maximum thumbnail width in pixels.
     */
    public int getMaxThumbnailWidthPx(Resources res) {
        int deviceWidthPx = res.getDisplayMetrics().widthPixels;
        int contextMenuMinimumPaddingPx =
                res.getDimensionPixelSize(R.dimen.context_menu_min_padding);
        int contextMenuWidth = Math.min(deviceWidthPx - 2 * contextMenuMinimumPaddingPx,
                res.getDimensionPixelSize(R.dimen.context_menu_max_width));

        return contextMenuWidth
                - res.getDimensionPixelSize(R.dimen.context_menu_header_image_width_padding) * 2;
    }

    /**
     * Calculates the maximum possible height of the thumbnail in pixels.
     * @param res The resources from where to pull the device dimensions.
     * @return The calculated maximum thumbnail height in pixels.
     */
    public int getMaxThumbnailHeightPx(Resources res) {
        // Use deviceWidthPx instead of deviceHeightPx because we want to make sure that the context
        // menu shows when the device is in its smaller height (i.e. landscape mode).
        int deviceWidthPx = res.getDisplayMetrics().widthPixels;
        int tabLayoutSize = mContextMenuDialog.findViewById(R.id.tab_layout).getHeight();
        int contextMenuMinimumPaddingPx =
                res.getDimensionPixelSize(R.dimen.context_menu_min_padding);

        return Math.min(deviceWidthPx - tabLayoutSize - (2 * contextMenuMinimumPaddingPx)
                        - res.getDimensionPixelSize(R.dimen.context_menu_image_vertical_margin) * 2
                        - res.getDimensionPixelSize(R.dimen.context_menu_selectable_items_min_size),
                res.getDimensionPixelSize(R.dimen.context_menu_header_image_max_height));
    }
}
