// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.app.Activity;
import android.graphics.drawable.Drawable;
import android.os.StrictMode;
import android.text.TextUtils;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.Space;
import android.widget.TextView;

import org.chromium.base.Callback;
import org.chromium.chrome.R;

import java.util.List;

/**
 * Takes a list of {@link ContextMenuItem} and puts them in an adapter meant to be used within a
 * list view.
 */
class TabularContextMenuListAdapter extends BaseAdapter {
    private final List<ContextMenuItem> mMenuItems;
    private final Activity mActivity;
    private final Callback<Boolean> mOnDirectShare;

    /**
     * Adapter for the tabular context menu UI
     * @param menuItems The list of items to display in the view.
     * @param activity Used to inflate the layout.
     * @param onDirectShare Callback to handle direct share.
     */
    TabularContextMenuListAdapter(
            List<ContextMenuItem> menuItems, Activity activity, Callback<Boolean> onDirectShare) {
        mMenuItems = menuItems;
        mActivity = activity;
        mOnDirectShare = onDirectShare;
    }

    @Override
    public int getCount() {
        return mMenuItems.size();
    }

    @Override
    public Object getItem(int position) {
        return mMenuItems.get(position);
    }

    @Override
    public long getItemId(int position) {
        return mMenuItems.get(position).getMenuId();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final ContextMenuItem menuItem = mMenuItems.get(position);
        ViewHolderItem viewHolder;

        if (convertView == null) {
            LayoutInflater inflater = LayoutInflater.from(mActivity);
            convertView = inflater.inflate(R.layout.tabular_context_menu_row, null);

            viewHolder = new ViewHolderItem();
            viewHolder.mIcon = (ImageView) convertView.findViewById(R.id.context_menu_icon);
            viewHolder.mText = (TextView) convertView.findViewById(R.id.context_menu_text);
            if (viewHolder.mText == null) {
                throw new IllegalStateException("Context text not found in new view inflation");
            }
            viewHolder.mRightPadding =
                    (Space) convertView.findViewById(R.id.context_menu_right_padding);

            convertView.setTag(viewHolder);
        } else {
            viewHolder = (ViewHolderItem) convertView.getTag();
            if (viewHolder.mText == null) {
                throw new IllegalStateException("Context text not found in view resuse");
            }
        }

        final String titleText = menuItem.getTitle(mActivity);
        viewHolder.mText.setText(titleText);

        if (menuItem instanceof ShareContextMenuItem) {
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            try {
                final Pair<Drawable, CharSequence> shareInfo =
                        ((ShareContextMenuItem) menuItem).getShareInfo();
                if (shareInfo.first != null) {
                    viewHolder.mIcon.setImageDrawable(shareInfo.first);
                    viewHolder.mIcon.setVisibility(View.VISIBLE);
                    viewHolder.mIcon.setContentDescription(mActivity.getString(
                            R.string.accessibility_menu_share_via, shareInfo.second));
                    viewHolder.mIcon.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            mOnDirectShare.onResult(
                                    ((ShareContextMenuItem) menuItem).isShareLink());
                        }
                    });
                    viewHolder.mRightPadding.setVisibility(View.GONE);
                }
            } finally {
                StrictMode.setThreadPolicy(oldPolicy);
            }
        } else {
            viewHolder.mIcon.setVisibility(View.GONE);
            viewHolder.mIcon.setImageDrawable(null);
            viewHolder.mIcon.setContentDescription(null);
            viewHolder.mIcon.setOnClickListener(null);
            viewHolder.mRightPadding.setVisibility(View.VISIBLE);

            Callback<Drawable> callback = drawable -> {
                // If the current title does not match the title when triggering the callback,
                // assume the View now represents a different view and do not update the icon.
                if (!TextUtils.equals(titleText, viewHolder.mText.getText())) return;

                if (drawable != null) {
                    viewHolder.mIcon.setVisibility(View.VISIBLE);
                    viewHolder.mIcon.setImageDrawable(drawable);
                    viewHolder.mRightPadding.setVisibility(View.GONE);
                }
            };
            menuItem.getDrawableAsync(mActivity, callback);
        }

        return convertView;
    }

    private static class ViewHolderItem {
        ImageView mIcon;
        TextView mText;
        Space mRightPadding;
    }
}
