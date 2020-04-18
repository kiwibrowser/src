// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.content.Context;
import android.preference.DialogPreference;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ListView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DirectoryOption;

/**
 * The preference used to save the download directory in download settings page.
 */
public class DownloadLocationPreference extends DialogPreference {
    /**
     * Provides data for the list of available download directories options.
     */
    private DownloadLocationPreferenceAdapter mAdapter;

    /**
     * The view contains the list of download directories.
     */
    private ListView mListView;

    /**
     * Constructor for DownloadLocationPreference.
     */
    public DownloadLocationPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mAdapter = new DownloadLocationPreferenceAdapter(context, this);
    }

    /**
     * Updates the summary that shows the download location directory.
     */
    public void updateSummary() {
        DirectoryOption directoryOption =
                (DirectoryOption) mAdapter.getItem(mAdapter.getSelectedItemId());
        final SpannableStringBuilder summaryBuilder = new SpannableStringBuilder();
        summaryBuilder.append(directoryOption.name);
        summaryBuilder.append(" ");
        summaryBuilder.append(directoryOption.location);
        summaryBuilder.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0,
                directoryOption.name.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        setSummary(summaryBuilder);
    }

    @Override
    protected View onCreateDialogView() {
        View view = LayoutInflater.from(getContext())
                            .inflate(R.layout.download_location_preference, null);
        mListView = (ListView) (view.findViewById(R.id.location_preference_list_view));
        mListView.setAdapter(mAdapter);
        return view;
    }
}
