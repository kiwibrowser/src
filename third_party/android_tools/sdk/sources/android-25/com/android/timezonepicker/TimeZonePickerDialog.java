/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.timezonepicker;

import android.app.Dialog;
import android.app.DialogFragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

public class TimeZonePickerDialog extends DialogFragment implements
        TimeZonePickerView.OnTimeZoneSetListener {
    public static final String TAG = TimeZonePickerDialog.class.getSimpleName();

    public static final String BUNDLE_START_TIME_MILLIS = "bundle_event_start_time";
    public static final String BUNDLE_TIME_ZONE = "bundle_event_time_zone";

    private static final String KEY_HAS_RESULTS = "has_results";
    private static final String KEY_LAST_FILTER_STRING = "last_filter_string";
    private static final String KEY_LAST_FILTER_TYPE = "last_filter_type";
    private static final String KEY_LAST_FILTER_TIME = "last_filter_time";
    private static final String KEY_HIDE_FILTER_SEARCH = "hide_filter_search";

    private OnTimeZoneSetListener mTimeZoneSetListener;
    private TimeZonePickerView mView;
    private boolean mHasCachedResults = false;

    public interface OnTimeZoneSetListener {
        void onTimeZoneSet(TimeZoneInfo tzi);
    }

    public void setOnTimeZoneSetListener(OnTimeZoneSetListener l) {
        mTimeZoneSetListener = l;
    }

    public TimeZonePickerDialog() {
        super();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        long timeMillis = 0;
        String timeZone = null;
        Bundle b = getArguments();
        if (b != null) {
            timeMillis = b.getLong(BUNDLE_START_TIME_MILLIS);
            timeZone = b.getString(BUNDLE_TIME_ZONE);
        }
        boolean hideFilterSearch = false;

        if (savedInstanceState != null) {
            hideFilterSearch = savedInstanceState.getBoolean(KEY_HIDE_FILTER_SEARCH);
        }
        mView = new TimeZonePickerView(getActivity(), null, timeZone, timeMillis, this,
                hideFilterSearch);
        if (savedInstanceState != null && savedInstanceState.getBoolean(KEY_HAS_RESULTS, false)) {
            mView.showFilterResults(savedInstanceState.getInt(KEY_LAST_FILTER_TYPE),
                                    savedInstanceState.getString(KEY_LAST_FILTER_STRING),
                                    savedInstanceState.getInt(KEY_LAST_FILTER_TIME));
        }
        return mView;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(KEY_HAS_RESULTS, mView != null && mView.hasResults());
        if (mView != null) {
            outState.putInt(KEY_LAST_FILTER_TYPE, mView.getLastFilterType());
            outState.putString(KEY_LAST_FILTER_STRING, mView.getLastFilterString());
            outState.putInt(KEY_LAST_FILTER_TIME, mView.getLastFilterTime());
            outState.putBoolean(KEY_HIDE_FILTER_SEARCH, mView.getHideFilterSearchOnStart());
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        return dialog;
    }

    @Override
    public void onTimeZoneSet(TimeZoneInfo tzi) {
        if (mTimeZoneSetListener != null) {
            mTimeZoneSetListener.onTimeZoneSet(tzi);
        }
        dismiss();
    }
}
