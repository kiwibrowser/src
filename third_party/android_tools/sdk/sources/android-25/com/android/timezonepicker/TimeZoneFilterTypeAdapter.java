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

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Collections;

public class TimeZoneFilterTypeAdapter extends BaseAdapter implements Filterable, OnClickListener {
    public static final String TAG = "TimeZoneFilterTypeAdapter";

    private static final boolean DEBUG = false;

    public static final int FILTER_TYPE_EMPTY = -1;
    public static final int FILTER_TYPE_NONE = 0;
    public static final int FILTER_TYPE_COUNTRY = 1;
    public static final int FILTER_TYPE_STATE = 2;
    public static final int FILTER_TYPE_GMT = 3;

    public interface OnSetFilterListener {
        void onSetFilter(int filterType, String str, int time);
    }

    static class ViewHolder {
        int filterType;
        String str;
        int time;
        TextView strTextView;

        static void setupViewHolder(View v) {
            ViewHolder vh = new ViewHolder();
            vh.strTextView = (TextView) v.findViewById(R.id.value);
            v.setTag(vh);
        }
    }

    class FilterTypeResult {
        int type;
        String constraint;
        public int time;

        public FilterTypeResult(int type, String constraint, int time) {
            this.type = type;
            this.constraint = constraint;
            this.time = time;
        }

        @Override
        public String toString() {
            return constraint;
        }
    }

    private ArrayList<FilterTypeResult> mLiveResults = new ArrayList<FilterTypeResult>();
    private int mLiveResultsCount = 0;

    private ArrayFilter mFilter;

    private LayoutInflater mInflater;

    private TimeZoneData mTimeZoneData;
    private OnSetFilterListener mListener;

    public TimeZoneFilterTypeAdapter(Context context, TimeZoneData tzd, OnSetFilterListener l) {
        mTimeZoneData = tzd;
        mListener = l;
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public int getCount() {
        return mLiveResultsCount;
    }

    @Override
    public FilterTypeResult getItem(int position) {
        return mLiveResults.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View v;

        if (convertView != null) {
            v = convertView;
        } else {
            v = mInflater.inflate(R.layout.time_zone_filter_item, null);
            ViewHolder.setupViewHolder(v);
        }

        ViewHolder vh = (ViewHolder) v.getTag();

        if (position >= mLiveResults.size()) {
            Log.e(TAG, "getView: " + position + " of " + mLiveResults.size());
        }

        FilterTypeResult filter = mLiveResults.get(position);

        vh.filterType = filter.type;
        vh.str = filter.constraint;
        vh.time = filter.time;
        vh.strTextView.setText(filter.constraint);
        return v;
    }

    OnClickListener mDummyListener = new OnClickListener() {

        @Override
        public void onClick(View v) {
        }
    };

    // Implements OnClickListener

    // This onClickListener is actually called from the AutoCompleteTextView's
    // onItemClickListener. Trying to update the text in AutoCompleteTextView
    // is causing an infinite loop.
    @Override
    public void onClick(View v) {
        if (mListener != null && v != null) {
            ViewHolder vh = (ViewHolder) v.getTag();
            mListener.onSetFilter(vh.filterType, vh.str, vh.time);
        }
        notifyDataSetInvalidated();
    }

    // Implements Filterable
    @Override
    public Filter getFilter() {
        if (mFilter == null) {
            mFilter = new ArrayFilter();
        }
        return mFilter;
    }

    private class ArrayFilter extends Filter {
        @Override
        protected FilterResults performFiltering(CharSequence prefix) {
            if (DEBUG) {
                Log.d(TAG, "performFiltering >>>> [" + prefix + "]");
            }

            FilterResults results = new FilterResults();
            String prefixString = null;
            if (prefix != null) {
                prefixString = prefix.toString().trim().toLowerCase();
            }

            if (TextUtils.isEmpty(prefixString)) {
                results.values = null;
                results.count = 0;
                return results;
            }

            // TODO Perf - we can loop through the filtered list if the new
            // search string starts with the old search string
            ArrayList<FilterTypeResult> filtered = new ArrayList<FilterTypeResult>();

            // ////////////////////////////////////////
            // Search by local time and GMT offset
            // ////////////////////////////////////////
            boolean gmtOnly = false;
            int startParsePosition = 0;
            if (prefixString.charAt(0) == '+' || prefixString.charAt(0) == '-') {
                gmtOnly = true;
            }

            if (prefixString.startsWith("gmt")) {
                startParsePosition = 3;
                gmtOnly = true;
            }

            int num = parseNum(prefixString, startParsePosition);
            if (num != Integer.MIN_VALUE) {
                boolean positiveOnly = prefixString.length() > startParsePosition
                        && prefixString.charAt(startParsePosition) == '+';
                handleSearchByGmt(filtered, num, positiveOnly);
            }

            // ////////////////////////////////////////
            // Search by country
            // ////////////////////////////////////////
            ArrayList<String> countries = new ArrayList<String>();
            for (String country : mTimeZoneData.mTimeZonesByCountry.keySet()) {
                // TODO Perf - cache toLowerCase()?
                if (!TextUtils.isEmpty(country)) {
                    final String lowerCaseCountry = country.toLowerCase();
                    boolean isMatch = false;
                    if (lowerCaseCountry.startsWith(prefixString)
                            || (lowerCaseCountry.charAt(0) == prefixString.charAt(0) &&
                            isStartingInitialsFor(prefixString, lowerCaseCountry))) {
                        isMatch = true;
                    } else if (lowerCaseCountry.contains(" ")){
                        // We should also search other words in the country name, so that
                        // searches like "Korea" yield "South Korea".
                        for (String word : lowerCaseCountry.split(" ")) {
                            if (word.startsWith(prefixString)) {
                                isMatch = true;
                                break;
                            }
                        }
                    }
                    if (isMatch) {
                        countries.add(country);
                    }
                }
            }
            if (countries.size() > 0) {
                // Sort countries alphabetically.
                Collections.sort(countries);
                for (String country : countries) {
                    filtered.add(new FilterTypeResult(FILTER_TYPE_COUNTRY, country, 0));
                }
            }

            // ////////////////////////////////////////
            // TODO Search by state
            // ////////////////////////////////////////
            if (DEBUG) {
                Log.d(TAG, "performFiltering <<<< " + filtered.size() + "[" + prefix + "]");
            }

            results.values = filtered;
            results.count = filtered.size();
            return results;
        }

        /**
         * Returns true if the prefixString is an initial for string. Note that
         * this method will return true even if prefixString does not cover all
         * the words. Words are separated by non-letters which includes spaces
         * and symbols).
         *
         * For example:
         * isStartingInitialsFor("UA", "United Arab Emirates") would return true
         * isStartingInitialsFor("US", "U.S. Virgin Island") would return true
         *
         * @param prefixString
         * @param string
         * @return
         */
        private boolean isStartingInitialsFor(String prefixString, String string) {
            final int initialLen = prefixString.length();
            final int strLen = string.length();

            int initialIdx = 0;
            boolean wasWordBreak = true;
            for (int i = 0; i < strLen; i++) {
                if (!Character.isLetter(string.charAt(i))) {
                    wasWordBreak = true;
                    continue;
                }

                if (wasWordBreak) {
                    if (prefixString.charAt(initialIdx++) != string.charAt(i)) {
                        return false;
                    }
                    if (initialIdx == initialLen) {
                        return true;
                    }
                    wasWordBreak = false;
                }
            }

            // Special case for "USA". Note that both strings have been turned to lowercase already.
            if (prefixString.equals("usa") && string.equals("united states")) {
                return true;
            }
            return false;
        }

        private void handleSearchByGmt(ArrayList<FilterTypeResult> filtered, int num,
                boolean positiveOnly) {

            FilterTypeResult r;
            if (num >= 0) {
                if (num == 1) {
                    for (int i = 19; i >= 10; i--) {
                        if (mTimeZoneData.hasTimeZonesInHrOffset(i)) {
                            r = new FilterTypeResult(FILTER_TYPE_GMT, "GMT+" + i, i);
                            filtered.add(r);
                        }
                    }
                }

                if (mTimeZoneData.hasTimeZonesInHrOffset(num)) {
                    r = new FilterTypeResult(FILTER_TYPE_GMT, "GMT+" + num, num);
                    filtered.add(r);
                }
                num *= -1;
            }

            if (!positiveOnly && num != 0) {
                if (mTimeZoneData.hasTimeZonesInHrOffset(num)) {
                    r = new FilterTypeResult(FILTER_TYPE_GMT, "GMT" + num, num);
                    filtered.add(r);
                }

                if (num == -1) {
                    for (int i = -10; i >= -19; i--) {
                        if (mTimeZoneData.hasTimeZonesInHrOffset(i)) {
                            r = new FilterTypeResult(FILTER_TYPE_GMT, "GMT" + i, i);
                            filtered.add(r);
                        }
                    }
                }
            }
        }

        /**
         * Acceptable strings are in the following format: [+-]?[0-9]?[0-9]
         *
         * @param str
         * @param startIndex
         * @return Integer.MIN_VALUE as invalid
         */
        public int parseNum(String str, int startIndex) {
            int idx = startIndex;
            int num = Integer.MIN_VALUE;
            int negativeMultiplier = 1;

            // First char - check for + and -
            char ch = str.charAt(idx++);
            switch (ch) {
                case '-':
                    negativeMultiplier = -1;
                    // fall through
                case '+':
                    if (idx >= str.length()) {
                        // No more digits
                        return Integer.MIN_VALUE;
                    }

                    ch = str.charAt(idx++);
                    break;
            }

            if (!Character.isDigit(ch)) {
                // No digit
                return Integer.MIN_VALUE;
            }

            // Got first digit
            num = Character.digit(ch, 10);

            // Check next char
            if (idx < str.length()) {
                ch = str.charAt(idx++);
                if (Character.isDigit(ch)) {
                    // Got second digit
                    num = 10 * num + Character.digit(ch, 10);
                } else {
                    return Integer.MIN_VALUE;
                }
            }

            if (idx != str.length()) {
                // Invalid
                return Integer.MIN_VALUE;
            }

            if (DEBUG) {
                Log.d(TAG, "Parsing " + str + " -> " + negativeMultiplier * num);
            }
            return negativeMultiplier * num;
        }

        @SuppressWarnings("unchecked")
        @Override
        protected void publishResults(CharSequence constraint, FilterResults
                results) {
            if (results.values == null || results.count == 0) {
                if (mListener != null) {
                    int filterType;
                    if (TextUtils.isEmpty(constraint)) {
                        filterType = FILTER_TYPE_NONE;
                    } else {
                        filterType = FILTER_TYPE_EMPTY;
                    }
                    mListener.onSetFilter(filterType, null, 0);
                }
                if (DEBUG) {
                    Log.d(TAG, "publishResults: " + results.count + " of null [" + constraint);
                }
            } else {
                mLiveResults = (ArrayList<FilterTypeResult>) results.values;
                if (DEBUG) {
                    Log.d(TAG, "publishResults: " + results.count + " of " + mLiveResults.size()
                            + " [" + constraint);
                }
            }
            mLiveResultsCount = results.count;

            if (results.count > 0) {
                notifyDataSetChanged();
            } else {
                notifyDataSetInvalidated();
            }
        }
    }
}
