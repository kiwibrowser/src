// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromoting;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chromoting.help.HelpContext;

/**
 * Describes the appearance and behavior of the navigation menu. This also implements
 * AdapterView.OnItemClickListener so it can be used as the ListView's onItemClickListener.
 */
public class NavigationMenuAdapter extends ArrayAdapter<NavigationMenuAdapter.NavigationMenuItem>
        implements AdapterView.OnItemClickListener {
    /**
     * Defines a menu item.
     */
    public static class NavigationMenuItem {
        private String mText;
        private Drawable mIcon;
        private Runnable mCallback;

        public NavigationMenuItem(String text, Drawable icon, Runnable callback) {
            mText = text;
            mIcon = icon;
            mCallback = callback;
        }
    }

    public static ListView createNavigationMenu(final Chromoting chromoting) {
        ListView navigationMenu =
                (ListView) chromoting.getLayoutInflater().inflate(R.layout.navigation_list, null);

        NavigationMenuItem feedbackItem = new NavigationMenuItem(
                chromoting.getResources().getString(R.string.actionbar_send_feedback),
                getIcon(chromoting, R.drawable.ic_announcement), new Runnable() {
                    @Override
                    public void run() {
                        chromoting.launchFeedback();
                    }
                });

        NavigationMenuItem helpItem = new NavigationMenuItem(
                chromoting.getResources().getString(R.string.actionbar_help),
                getIcon(chromoting, R.drawable.ic_help), new Runnable() {
                    @Override
                    public void run() {
                        chromoting.launchHelp(HelpContext.HOST_LIST);
                    }
                });

        NavigationMenuItem[] navigationMenuItems = { feedbackItem, helpItem };
        NavigationMenuAdapter adapter = new NavigationMenuAdapter(chromoting, navigationMenuItems);
        navigationMenu.setAdapter(adapter);
        navigationMenu.setOnItemClickListener(adapter);
        return navigationMenu;
    }

    /** Returns the drawable of |drawableId| that can be used to draw icon for a navigation item. */
    private static Drawable getIcon(Activity activity, int drawableId) {
        Drawable drawable = ApiCompatibilityUtils.getDrawable(activity.getResources(), drawableId);
        drawable.setBounds(0, 0, drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight());
        return drawable;
    }

    private NavigationMenuAdapter(Context context, NavigationMenuItem[] objects) {
        super(context, -1, objects);
    }

    /** Generates a View corresponding to the particular navigation item. */
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            LayoutInflater inflater =
                    (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(R.layout.navigation_list_item, parent, false);
        }
        TextView textView = (TextView) convertView.findViewById(R.id.navigation_item_label);
        NavigationMenuItem item = getItem(position);
        textView.setCompoundDrawables(item.mIcon, null, null, null);
        textView.setText(item.mText);
        return convertView;
    }


    /** AdapterView.OnItemClickListener override. */
    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        getItem(position).mCallback.run();
    }
}
