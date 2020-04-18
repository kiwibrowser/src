// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.content.Context;
import android.content.SharedPreferences;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DirectoryOption;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;

/**
 * Class used to provide data shown in the download location preference in download settings page.
 */
public class DownloadLocationPreferenceAdapter
        extends DownloadDirectoryAdapter implements OnClickListener {
    private DownloadLocationPreference mPreference;

    /**
     * Constructor of DownloadLocationPreferenceAdapter.
     */
    public DownloadLocationPreferenceAdapter(
            Context context, DownloadLocationPreference preference) {
        super(context);
        mPreference = preference;

        if (getSelectedItemId() == NO_SELECTED_ITEM_ID) {
            useFirstValidSelectableItemId();
        }
    }

    @Override
    public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            view = LayoutInflater.from(getContext())
                           .inflate(R.layout.download_location_preference_item, null);
        }

        view.setTag(position);
        view.setOnClickListener(this);

        RadioButton radioButton = view.findViewById(R.id.radio_button);
        radioButton.setChecked(getSelectedItemId() == position);
        radioButton.setTag(position);
        radioButton.setOnClickListener(this);

        // Only show the radio button when there are multiple items.
        if (getCount() <= 1) radioButton.setVisibility(View.GONE);

        view.setEnabled(isEnabled(position));

        DirectoryOption directoryOption = (DirectoryOption) getItem(position);
        if (directoryOption == null) return view;

        // TODO(xingliu): Refactor these to base class.
        TextView titleText = (TextView) view.findViewById(R.id.title);
        titleText.setText(directoryOption.name);

        TextView summaryText = (TextView) view.findViewById(R.id.description);
        if (isEnabled(position)) {
            String summary = DownloadUtils.getStringForAvailableBytes(
                    getContext(), directoryOption.availableSpace);
            summaryText.setText(summary);

            // Build description for accessibility.
            StringBuilder accessibilityDescription = new StringBuilder();
            accessibilityDescription.append(directoryOption.name);
            accessibilityDescription.append(" ");
            accessibilityDescription.append(summary);
            radioButton.setContentDescription(accessibilityDescription);
        } else {
            radioButton.setEnabled(false);
            titleText.setEnabled(false);
            summaryText.setEnabled(false);

            if (hasAvailableLocations()) {
                summaryText.setText(
                        getContext().getText(R.string.download_location_not_enough_space));
            } else {
                summaryText.setVisibility(View.GONE);
            }
        }

        return view;
    }

    @Override
    public void onClick(View v) {
        int selectedId = (int) v.getTag();
        DirectoryOption option = (DirectoryOption) getItem(selectedId);
        if (option == null) return;

        // Update the native pref, which persists the download directory selected by the user.
        PrefServiceBridge.getInstance().setDownloadAndSaveFileDefaultDirectory(option.location);

        // Update the android pref and update the summary in download settings page.
        SharedPreferences.Editor editor = ContextUtils.getAppSharedPreferences().edit();
        editor.putString(DownloadPreferences.PREF_LOCATION_CHANGE, option.location);
        editor.apply();

        mSelectedPosition = selectedId;

        // Update the preference after selected position is updated.
        mPreference.updateSummary();

        option.recordDirectoryOptionType();

        // Refresh the list of download directories UI.
        notifyDataSetChanged();
    }
}
