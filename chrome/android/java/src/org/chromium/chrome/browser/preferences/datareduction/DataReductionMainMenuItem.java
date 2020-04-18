// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.datareduction;

import android.content.Context;
import android.content.Intent;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.text.format.DateUtils;
import android.text.format.Formatter;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.preferences.PreferencesLauncher;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.third_party.android.datausagechart.ChartDataUsageView;

/**
 * Specific {@link FrameLayout} that displays the data savings of Data Saver in the main menu.
 */
public class DataReductionMainMenuItem extends FrameLayout implements View.OnClickListener {
    /**
     * Constructs a new {@link DataReductionMainMenuItem} with the appropriate context.
     */
    public DataReductionMainMenuItem(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        TextView itemText = (TextView) findViewById(R.id.menu_item_text);
        TextView itemSummary = (TextView) findViewById(R.id.menu_item_summary);

        if (DataReductionProxySettings.getInstance().isDataReductionProxyEnabled()) {
            DataReductionProxyUma.dataReductionProxyUIAction(
                    DataReductionProxyUma.ACTION_MAIN_MENU_DISPLAYED_ON);

            String dataSaved = Formatter.formatShortFileSize(getContext(),
                    DataReductionProxySettings.getInstance()
                            .getContentLengthSavedInHistorySummary());

            long chartStartDateInMillisSinceEpoch =
                    DataReductionProxySettings.getInstance().getDataReductionLastUpdateTime()
                    - DateUtils.DAY_IN_MILLIS * ChartDataUsageView.DAYS_IN_CHART;
            long firstEnabledInMillisSinceEpoch = DataReductionProxySettings.getInstance()
                                                          .getDataReductionProxyFirstEnabledTime();
            long mostRecentTime = chartStartDateInMillisSinceEpoch > firstEnabledInMillisSinceEpoch
                    ? chartStartDateInMillisSinceEpoch
                    : firstEnabledInMillisSinceEpoch;

            final int flags = DateUtils.FORMAT_ABBREV_MONTH | DateUtils.FORMAT_NO_YEAR;
            String date = DateUtils.formatDateTime(getContext(), mostRecentTime, flags).toString();

            itemText.setText(
                    getContext().getString(R.string.data_reduction_saved_label, dataSaved));
            itemSummary.setText(getContext().getString(R.string.data_reduction_date_label, date));

            int lightActiveColor = ApiCompatibilityUtils.getColor(
                    getContext().getResources(), R.color.light_active_color);
            itemText.setTextColor(lightActiveColor);

            // Reset the icon to blue.
            ImageView icon = (ImageView) findViewById(R.id.chart_icon);
            LayerDrawable layers = (LayerDrawable) icon.getDrawable();
            Drawable chart = layers.findDrawableByLayerId(R.id.main_menu_chart);
            chart.setColorFilter(null);
        } else {
            DataReductionProxyUma.dataReductionProxyUIAction(
                    DataReductionProxyUma.ACTION_MAIN_MENU_DISPLAYED_OFF);

            itemText.setText(R.string.data_reduction_title);
            itemSummary.setText(R.string.text_off);

            // Make the icon grey.
            ImageView icon = (ImageView) findViewById(R.id.chart_icon);
            LayerDrawable layers = (LayerDrawable) icon.getDrawable();
            Drawable chart = layers.findDrawableByLayerId(R.id.main_menu_chart);
            ColorMatrix matrix = new ColorMatrix();
            matrix.setSaturation(0);
            chart.setColorFilter(new ColorMatrixColorFilter(matrix));
        }

        setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        Intent intent = PreferencesLauncher.createIntentForSettingsPage(
                getContext(), DataReductionPreferences.class.getName());
        RecordUserAction.record("MobileMenuDataSaverOpened");
        intent.putExtra(DataReductionPreferences.FROM_MAIN_MENU, true);
        getContext().startActivity(intent);

        Tracker tracker = TrackerFactory.getTrackerForProfile(Profile.getLastUsedProfile());
        tracker.notifyEvent(EventConstants.DATA_SAVER_DETAIL_OPENED);
    }
}