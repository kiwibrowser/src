// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.content.Context;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.widget.TextViewCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DirectoryOption;
import org.chromium.chrome.browser.download.DownloadUtils;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.widget.TintedImageView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Custom adapter that populates the list of which directories the user can choose as their default
 * download location.
 */
public class DownloadDirectoryAdapter extends ArrayAdapter<Object> {
    public static int NO_SELECTED_ITEM_ID = -1;
    public static int SELECTED_ITEM_NOT_INITIALIZED = -2;

    protected int mSelectedPosition = SELECTED_ITEM_NOT_INITIALIZED;

    private Context mContext;
    private LayoutInflater mLayoutInflater;

    private List<DirectoryOption> mCanonicalOptions = new ArrayList<>();
    private List<DirectoryOption> mAdditionalOptions = new ArrayList<>();
    private List<DirectoryOption> mErrorOptions = new ArrayList<>();

    public DownloadDirectoryAdapter(@NonNull Context context) {
        super(context, android.R.layout.simple_spinner_item);

        mContext = context;
        mLayoutInflater = LayoutInflater.from(context);

        refreshData();
    }

    @Override
    public int getCount() {
        return mCanonicalOptions.size() + mAdditionalOptions.size() + mErrorOptions.size();
    }

    @Nullable
    @Override
    public Object getItem(int position) {
        if (!mErrorOptions.isEmpty()) {
            assert position == 0;
            assert getCount() == 1;
            return mErrorOptions.get(position);
        }

        if (position < mCanonicalOptions.size()) {
            return mCanonicalOptions.get(position);
        } else {
            return mAdditionalOptions.get(position - mCanonicalOptions.size());
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @NonNull
    @Override
    public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            view = mLayoutInflater.inflate(R.layout.download_location_spinner_item, null);
        }

        view.setTag(position);

        DirectoryOption directoryOption = (DirectoryOption) getItem(position);
        if (directoryOption == null) return view;

        TextView titleText = (TextView) view.findViewById(R.id.text);
        titleText.setText(directoryOption.name);

        // ModalDialogView may do a measure pass on the view hierarchy to limit the layout inside
        // certain area, where LayoutParams cannot be null.
        if (view.getLayoutParams() == null) {
            view.setLayoutParams(
                    new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }
        return view;
    }

    @Override
    public View getDropDownView(
            int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            view = mLayoutInflater.inflate(R.layout.download_location_spinner_dropdown_item, null);
        }

        view.setTag(position);

        DirectoryOption directoryOption = (DirectoryOption) getItem(position);
        if (directoryOption == null) return view;

        TextView titleText = (TextView) view.findViewById(R.id.title);
        titleText.setText(directoryOption.name);

        TextView summaryText = (TextView) view.findViewById(R.id.description);
        if (isEnabled(position)) {
            TextViewCompat.setTextAppearance(titleText, R.style.BlackTitle1);
            TextViewCompat.setTextAppearance(summaryText, R.style.BlackBody);
            summaryText.setText(DownloadUtils.getStringForAvailableBytes(
                    mContext, directoryOption.availableSpace));
        } else {
            TextViewCompat.setTextAppearance(titleText, R.style.BlackDisabledText1);
            TextViewCompat.setTextAppearance(summaryText, R.style.BlackDisabledText3);
            if (mErrorOptions.isEmpty()) {
                summaryText.setText(mContext.getText(R.string.download_location_not_enough_space));
            } else {
                summaryText.setVisibility(View.GONE);
            }
        }

        TintedImageView imageView = (TintedImageView) view.findViewById(R.id.icon_view);
        imageView.setVisibility(View.GONE);

        return view;
    }

    @Override
    public boolean isEnabled(int position) {
        DirectoryOption directoryOption = (DirectoryOption) getItem(position);
        return directoryOption != null && directoryOption.availableSpace != 0;
    }

    /**
     * @return  ID of the directory option that matches the default download location or
     *          NO_SELECTED_ITEM_ID if no item matches the default path.
     */
    public int getSelectedItemId() {
        if (mSelectedPosition == SELECTED_ITEM_NOT_INITIALIZED) {
            mSelectedPosition = initSelectedIdFromPref();
        }

        return mSelectedPosition;
    }

    private int initSelectedIdFromPref() {
        if (!mErrorOptions.isEmpty()) return 0;
        String defaultLocation = PrefServiceBridge.getInstance().getDownloadDefaultDirectory();
        for (int i = 0; i < getCount(); i++) {
            DirectoryOption option = (DirectoryOption) getItem(i);
            if (option == null) continue;
            if (defaultLocation.equals(option.location)) return i;
        }
        return NO_SELECTED_ITEM_ID;
    }

    /**
     * In the case that there is no selected item ID/the selected item ID is invalid (ie. there is
     * not enough space), select either the default or the next valid item ID. Set the default to be
     * this item and return the ID.
     *
     * @return  ID of the first valid, selectable item and the new default location.
     */
    public int useFirstValidSelectableItemId() {
        for (int i = 0; i < getCount(); i++) {
            DirectoryOption option = (DirectoryOption) getItem(i);
            if (option == null) continue;
            if (option.availableSpace > 0) {
                PrefServiceBridge.getInstance().setDownloadAndSaveFileDefaultDirectory(
                        option.location);
                mSelectedPosition = i;
                return i;
            }
        }

        // Display an option that says there are no available download locations.
        adjustErrorDirectoryOption();
        return 0;
    }

    boolean hasAvailableLocations() {
        return mErrorOptions.isEmpty();
    }

    private void refreshData() {
        setCanonicalDirectoryOptions();
        setAdditionalDirectoryOptions();
        adjustErrorDirectoryOption();
    }

    private void setCanonicalDirectoryOptions() {
        mCanonicalOptions.clear();
        File directoryLocation =
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        mCanonicalOptions.add(new DirectoryOption(mContext.getString(R.string.menu_downloads),
                directoryLocation.getAbsolutePath(), directoryLocation.getUsableSpace(),
                directoryLocation.getTotalSpace(), DirectoryOption.DEFAULT_OPTION));
    }

    private void setAdditionalDirectoryOptions() {
        mAdditionalOptions.clear();

        // If there are no more additional directories, it is only the primary storage available.
        String[] externalDirs = DownloadUtils.getAllDownloadDirectories();
        if (externalDirs.length <= 1) return;

        int numOtherAdditionalDirectories = 0;
        for (String dir : externalDirs) {
            if (TextUtils.isEmpty(dir)) continue;

            // Skip the directory that is in primary storage.
            if (dir.contains(Environment.getExternalStorageDirectory().getAbsolutePath())) continue;

            // Add index (ie. SD Card 2) if there is more than one secondary storage option.
            String directoryName = (numOtherAdditionalDirectories > 0)
                    ? mContext.getString(
                              org.chromium.chrome.R.string.downloads_location_sd_card_number,
                              numOtherAdditionalDirectories + 1)
                    : mContext.getString(org.chromium.chrome.R.string.downloads_location_sd_card);

            File file = new File(dir);
            mAdditionalOptions.add(new DirectoryOption(directoryName, file.getAbsolutePath(),
                    file.getUsableSpace(), file.getTotalSpace(),
                    DirectoryOption.ADDITIONAL_OPTION));
            numOtherAdditionalDirectories++;
        }
    }

    private void adjustErrorDirectoryOption() {
        if ((mCanonicalOptions.size() + mAdditionalOptions.size()) > 0) {
            mErrorOptions.clear();
        } else {
            mErrorOptions.add(new DirectoryOption(
                    mContext.getString(R.string.download_location_no_available_locations), null, 0,
                    0, DirectoryOption.ERROR_OPTION));
        }
    }
}
