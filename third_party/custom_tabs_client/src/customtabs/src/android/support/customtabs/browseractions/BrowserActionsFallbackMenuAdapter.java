/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.support.customtabs.browseractions;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.support.customtabs.R;
import android.support.v4.content.res.ResourcesCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.List;

/**
 * The adapter to display the icon and title of custom Browser Actions item.
 */
class BrowserActionsFallbackMenuAdapter extends BaseAdapter {
    private final List<BrowserActionItem> mMenuItems;
    private final Context mContext;

    BrowserActionsFallbackMenuAdapter(List<BrowserActionItem> menuItems, Context context) {
        mMenuItems = menuItems;
        mContext = context;
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
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final BrowserActionItem menuItem = mMenuItems.get(position);
        final ViewHolderItem viewHolder;
        if (convertView == null) {
            convertView = LayoutInflater.from(mContext).inflate(
                    R.layout.browser_actions_context_menu_row, null);
            viewHolder = new ViewHolderItem();
            viewHolder.mIcon =
                    (ImageView) convertView.findViewById(R.id.browser_actions_menu_item_icon);
            viewHolder.mText =
                    (TextView) convertView.findViewById(R.id.browser_actions_menu_item_text);
            convertView.setTag(viewHolder);
        } else {
            viewHolder = (ViewHolderItem) convertView.getTag();
        }

        final String titleText = menuItem.getTitle();
        viewHolder.mText.setText(titleText);
        if (menuItem.getIconId() != 0) {
            Drawable drawable = ResourcesCompat.getDrawable(
                    mContext.getResources(), menuItem.getIconId(), null);
            viewHolder.mIcon.setImageDrawable(drawable);
        } else if (menuItem.getIconUri() != null) {
            BrowserServiceImageReadTask task =
                    new BrowserServiceImageReadTask(mContext.getContentResolver()) {
                        @Override
                        protected void onBitmapFileReady(Bitmap bitmap) {
                            // ViewHolder has been reused by other item.
                            if (!TextUtils.equals(titleText, viewHolder.mText.getText())) return;
                            if (bitmap != null) {
                                viewHolder.mIcon.setVisibility(View.VISIBLE);
                                viewHolder.mIcon.setImageBitmap(bitmap);
                            } else {
                                viewHolder.mIcon.setVisibility(View.INVISIBLE);
                                viewHolder.mIcon.setImageBitmap(null);
                            }
                        }
                    };
            task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, menuItem.getIconUri());
        } else {
            viewHolder.mIcon.setImageBitmap(null);
            viewHolder.mIcon.setVisibility(View.INVISIBLE);
        }
        return convertView;
    }

    private static class ViewHolderItem {
        ImageView mIcon;
        TextView mText;
    }
}
