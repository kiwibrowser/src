// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.datareduction;

import static android.text.format.DateUtils.FORMAT_ABBREV_MONTH;
import static android.text.format.DateUtils.FORMAT_NO_YEAR;
import static android.text.format.DateUtils.FORMAT_SHOW_DATE;

import static org.chromium.third_party.android.datausagechart.ChartDataUsageView.DAYS_IN_CHART;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.preference.Preference;
import android.support.v7.app.AlertDialog;
import android.text.format.DateUtils;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.util.FileSizeUtil;
import org.chromium.third_party.android.datausagechart.ChartDataUsageView;
import org.chromium.third_party.android.datausagechart.NetworkStats;
import org.chromium.third_party.android.datausagechart.NetworkStatsHistory;

import java.util.List;
import java.util.TimeZone;

/**
 * Preference used to display statistics on data reduction.
 */
public class DataReductionStatsPreference extends Preference {
    private static final String TAG = "DataSaverStats";

    /**
     * Key used to save the date on which the site breakdown should be shown. If the user has
     * historical data saver stats, the site breakdown cannot be shown for DAYS_IN_CHART.
     */
    private static final String PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE =
            "data_reduction_site_breakdown_allowed_date";

    private NetworkStatsHistory mOriginalNetworkStatsHistory;
    private NetworkStatsHistory mReceivedNetworkStatsHistory;
    private List<DataReductionDataUseItem> mSiteBreakdownItems;

    private TextView mDataSavingsTextView;
    private TextView mDataUsageTextView;
    private TextView mStartDateTextView;
    private TextView mEndDateTextView;
    private Button mResetStatisticsButton;
    private ChartDataUsageView mChartDataUsageView;
    private DataReductionSiteBreakdownView mDataReductionBreakdownView;
    private long mLeftPosition;
    private long mRightPosition;
    private Long mCurrentTime;
    private CharSequence mOriginalTotalPhrase;
    private CharSequence mSavingsTotalPhrase;
    private CharSequence mReceivedTotalPhrase;
    private String mStartDatePhrase;
    private String mEndDatePhrase;

    /**
     * If this is the first time the site breakdown feature is enabled, set the first allowable date
     * the breakdown can be shown.
     */
    public static void initializeDataReductionSiteBreakdownPref() {
        // If the site breakdown pref has already been set, don't set it.
        if (ContextUtils.getAppSharedPreferences().contains(
                    PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE)) {
            return;
        }

        long lastUpdateTimeMillis =
                DataReductionProxySettings.getInstance().getDataReductionLastUpdateTime();

        // If the site breakdown is enabled and there are historical stats within the last
        // DAYS_IN_CHART days, don't show the breakdown for another DAYS_IN_CHART days from the last
        // update time. Otherwise, the site breakdown can be shown starting now.
        long timeChartCanBeShown = lastUpdateTimeMillis + DAYS_IN_CHART * DateUtils.DAY_IN_MILLIS;
        long now = System.currentTimeMillis();
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putLong(PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE,
                        timeChartCanBeShown > now ? timeChartCanBeShown : now)
                .apply();
    }

    public DataReductionStatsPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setWidgetLayoutResource(R.layout.data_reduction_stats_layout);
    }

    @Override
    public boolean isEnabled() {
        return super.isEnabled();
    }

    /**
     * Updates the preference screen to convey current statistics on data reduction.
     */
    public void updateReductionStatistics() {
        long original[] = DataReductionProxySettings.getInstance().getOriginalNetworkStatsHistory();
        long received[] = DataReductionProxySettings.getInstance().getReceivedNetworkStatsHistory();

        mCurrentTime = DataReductionProxySettings.getInstance().getDataReductionLastUpdateTime();
        mRightPosition = mCurrentTime + DateUtils.HOUR_IN_MILLIS
                - TimeZone.getDefault().getOffset(mCurrentTime);
        mLeftPosition = mCurrentTime - DateUtils.DAY_IN_MILLIS * DAYS_IN_CHART;
        mOriginalNetworkStatsHistory = getNetworkStatsHistory(original, DAYS_IN_CHART);
        mReceivedNetworkStatsHistory = getNetworkStatsHistory(received, DAYS_IN_CHART);

        if (mDataReductionBreakdownView != null
                && System.currentTimeMillis()
                        > ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE,
                                  Long.MAX_VALUE)) {
            DataReductionProxySettings.getInstance().queryDataUsage(
                    DAYS_IN_CHART, new Callback<List<DataReductionDataUseItem>>() {
                        @Override
                        public void onResult(List<DataReductionDataUseItem> result) {
                            mSiteBreakdownItems = result;
                            mDataReductionBreakdownView.setAndDisplayDataUseItems(
                                    mSiteBreakdownItems);
                        }
                    });
        }
    }

    private static NetworkStatsHistory getNetworkStatsHistory(long[] history, int days) {
        if (days > history.length) days = history.length;
        NetworkStatsHistory networkStatsHistory = new NetworkStatsHistory(
                DateUtils.DAY_IN_MILLIS, days, NetworkStatsHistory.FIELD_RX_BYTES);

        DataReductionProxySettings config = DataReductionProxySettings.getInstance();
        long time = config.getDataReductionLastUpdateTime() - days * DateUtils.DAY_IN_MILLIS;
        for (int i = history.length - days, bucket = 0; i < history.length; i++, bucket++) {
            NetworkStats.Entry entry = new NetworkStats.Entry();
            entry.rxBytes = history[i];
            long startTime = time + (DateUtils.DAY_IN_MILLIS * bucket);
            // Spread each day's record over the first hour of the day.
            networkStatsHistory.recordData(startTime, startTime + DateUtils.HOUR_IN_MILLIS, entry);
        }
        return networkStatsHistory;
    }

    private void setDetailText() {
        final Context context = getContext();
        updateDetailData();
        mStartDateTextView.setText(mStartDatePhrase);
        mStartDateTextView.setContentDescription(context.getString(
                R.string.data_reduction_start_date_content_description, mStartDatePhrase));
        mEndDateTextView.setText(mEndDatePhrase);
        mEndDateTextView.setContentDescription(context.getString(
                R.string.data_reduction_end_date_content_description, mEndDatePhrase));
        if (mDataUsageTextView != null) mDataUsageTextView.setText(mReceivedTotalPhrase);
        if (mDataSavingsTextView != null) mDataSavingsTextView.setText(mSavingsTotalPhrase);
    }

    /**
     * Keep the graph labels LTR oriented. In RTL languages, numbers and plots remain LTR.
     */
    @SuppressLint("RtlHardcoded")
    private void forceLayoutGravityOfGraphLabels() {
        ((FrameLayout.LayoutParams) mStartDateTextView.getLayoutParams()).gravity = Gravity.LEFT;
        ((FrameLayout.LayoutParams) mEndDateTextView.getLayoutParams()).gravity = Gravity.RIGHT;
    }

    /**
     * Sets up a data usage chart and text views containing data reduction statistics.
     * @param view The current view.
     */
    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        mDataUsageTextView = (TextView) view.findViewById(R.id.data_reduction_usage);
        mDataSavingsTextView = (TextView) view.findViewById(R.id.data_reduction_savings);
        mStartDateTextView = (TextView) view.findViewById(R.id.data_reduction_start_date);
        mEndDateTextView = (TextView) view.findViewById(R.id.data_reduction_end_date);
        mDataReductionBreakdownView =
                (DataReductionSiteBreakdownView) view.findViewById(R.id.breakdown);
        forceLayoutGravityOfGraphLabels();
        if (mOriginalNetworkStatsHistory == null) {
            // This will query data usage. Only set mSiteBreakdownItems if the statistics are not
            // being queried.
            updateReductionStatistics();
        } else if (mSiteBreakdownItems != null) {
            mDataReductionBreakdownView.setAndDisplayDataUseItems(mSiteBreakdownItems);
        }
        setDetailText();

        mChartDataUsageView = (ChartDataUsageView) view.findViewById(R.id.chart);
        mChartDataUsageView.bindOriginalNetworkStats(mOriginalNetworkStatsHistory);
        mChartDataUsageView.bindCompressedNetworkStats(mReceivedNetworkStatsHistory);
        mChartDataUsageView.setVisibleRange(
                mCurrentTime - DateUtils.DAY_IN_MILLIS * DAYS_IN_CHART,
                mCurrentTime + DateUtils.HOUR_IN_MILLIS, mLeftPosition, mRightPosition);

        if (DataReductionProxySettings.getInstance().isDataReductionProxyUnreachable()) {
            // Leave breadcrumb in log for user feedback report.
            Log.w(TAG, "Data Saver proxy unreachable when user viewed Data Saver stats");
        }

        mResetStatisticsButton = (Button) view.findViewById(R.id.data_reduction_reset_statistics);
        if (mResetStatisticsButton != null) {
            setUpResetStatisticsButton();
        }
    }

    private void setUpResetStatisticsButton() {
        mResetStatisticsButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                DialogInterface.OnClickListener dialogListener = new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (which == AlertDialog.BUTTON_POSITIVE) {
                            // If the site breakdown hasn't been shown yet because there was
                            // historical data, reset that state so that the site breakdown can
                            // now be shown.
                            long now = System.currentTimeMillis();
                            if (ContextUtils.getAppSharedPreferences().getLong(
                                        PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE,
                                        Long.MAX_VALUE)
                                    > now) {
                                ContextUtils.getAppSharedPreferences()
                                        .edit()
                                        .putLong(PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE,
                                                now)
                                        .apply();
                            }
                            DataReductionProxySettings.getInstance().clearDataSavingStatistics(
                                    DataReductionProxySavingsClearedReason
                                            .USER_ACTION_SETTINGS_MENU);
                            updateReductionStatistics();
                            setDetailText();
                            notifyChanged();
                            DataReductionProxyUma.dataReductionProxyUIAction(
                                    DataReductionProxyUma.ACTION_STATS_RESET);
                        } else {
                            // Do nothing if canceled.
                        }
                    }
                };

                new AlertDialog.Builder(getContext(), R.style.AlertDialogTheme)
                        .setTitle(R.string.data_reduction_usage_reset_statistics_confirmation_title)
                        .setMessage(
                                R.string.data_reduction_usage_reset_statistics_confirmation_dialog)
                        .setPositiveButton(
                                R.string.data_reduction_usage_reset_statistics_confirmation_button,
                                dialogListener)
                        .setNegativeButton(R.string.cancel, dialogListener)
                        .show();
            }
        });
    }

    /**
     * Update data reduction statistics whenever the chart's inspection
     * range changes. In particular, this creates strings describing the total
     * original size of all data received over the date range, the total size
     * of all data received (after compression), and the percent data reduction
     * and the range of dates over which these statistics apply.
     */
    // TODO(crbug.com/635567): Fix this properly.
    @SuppressLint("DefaultLocale")
    private void updateDetailData() {
        final long start = mLeftPosition;
        // Include up to the last second of the currently selected day.
        final long end = mRightPosition;
        final Context context = getContext();

        final long compressedTotalBytes = mReceivedNetworkStatsHistory.getTotalBytes();
        mReceivedTotalPhrase = FileSizeUtil.formatFileSize(context, compressedTotalBytes);
        final long originalTotalBytes = mOriginalNetworkStatsHistory.getTotalBytes();
        mOriginalTotalPhrase = FileSizeUtil.formatFileSize(context, originalTotalBytes);
        final long savingsTotalBytes = originalTotalBytes - compressedTotalBytes;
        mSavingsTotalPhrase = FileSizeUtil.formatFileSize(context, savingsTotalBytes);
        mStartDatePhrase = formatDate(context, start);
        mEndDatePhrase = formatDate(context, end);

        DataReductionProxyUma.dataReductionProxyUserViewedSavings(
                compressedTotalBytes, originalTotalBytes);
    }

    private static String formatDate(Context context, long millisSinceEpoch) {
        final int flags = FORMAT_SHOW_DATE | FORMAT_ABBREV_MONTH | FORMAT_NO_YEAR;
        return DateUtils.formatDateTime(context, millisSinceEpoch, flags).toString();
    }
}
