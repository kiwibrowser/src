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
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.android.timezonepicker.TimeZoneFilterTypeAdapter.OnSetFilterListener;
import com.android.timezonepicker.TimeZonePickerView.OnTimeZoneSetListener;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashSet;

public class TimeZoneResultAdapter extends BaseAdapter implements OnItemClickListener,
        OnSetFilterListener {
    private static final String TAG = "TimeZoneResultAdapter";
    private static final boolean DEBUG = false;
    private static final int VIEW_TAG_TIME_ZONE = R.id.time_zone;
    private static final int EMPTY_INDEX = -100;

    /** SharedPref name and key for recent time zones */
    private static final String SHARED_PREFS_NAME = "com.android.calendar_preferences";
    private static final String KEY_RECENT_TIMEZONES = "preferences_recent_timezones";

    private int mLastFilterType;
    private String mLastFilterString;
    private int mLastFilterTime;

    private boolean mHasResults = false;

    /**
     * The delimiter we use when serializing recent timezones to shared
     * preferences
     */
    private static final String RECENT_TIMEZONES_DELIMITER = ",";

    /** The maximum number of recent timezones to save */
    private static final int MAX_RECENT_TIMEZONES = 3;

    static class ViewHolder {
        TextView timeZone;
        TextView timeOffset;
        TextView location;

        static void setupViewHolder(View v) {
            ViewHolder vh = new ViewHolder();
            vh.timeZone = (TextView) v.findViewById(R.id.time_zone);
            vh.timeOffset = (TextView) v.findViewById(R.id.time_offset);
            vh.location = (TextView) v.findViewById(R.id.location);
            v.setTag(vh);
        }
    }

    private Context mContext;
    private LayoutInflater mInflater;

    private OnTimeZoneSetListener mTimeZoneSetListener;
    private TimeZoneData mTimeZoneData;

    private int[] mFilteredTimeZoneIndices;
    private int mFilteredTimeZoneLength = 0;

    public TimeZoneResultAdapter(Context context, TimeZoneData tzd,
            com.android.timezonepicker.TimeZonePickerView.OnTimeZoneSetListener l) {
        super();

        mContext = context;
        mTimeZoneData = tzd;
        mTimeZoneSetListener = l;

        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        mFilteredTimeZoneIndices = new int[mTimeZoneData.size()];

        onSetFilter(TimeZoneFilterTypeAdapter.FILTER_TYPE_NONE, null, 0);
    }

    public boolean hasResults() {
        return mHasResults;
    }

    public int getLastFilterType() {
        return mLastFilterType;
    }

    public String getLastFilterString() {
        return mLastFilterString;
    }

    public int getLastFilterTime() {
        return mLastFilterTime;
    }

    // Implements OnSetFilterListener
    @Override
    public void onSetFilter(int filterType, String str, int time) {
        if (DEBUG) {
            Log.d(TAG, "onSetFilter: " + filterType + " [" + str + "] " + time);
        }

        mLastFilterType = filterType;
        mLastFilterString = str;
        mLastFilterTime = time;

        mFilteredTimeZoneLength = 0;
        int idx = 0;

        switch (filterType) {
            case TimeZoneFilterTypeAdapter.FILTER_TYPE_EMPTY:
                mFilteredTimeZoneIndices[mFilteredTimeZoneLength++] = EMPTY_INDEX;
                break;
            case TimeZoneFilterTypeAdapter.FILTER_TYPE_NONE:
                // Show the default/current value first
                int defaultTzIndex = mTimeZoneData.getDefaultTimeZoneIndex();
                if (defaultTzIndex != -1) {
                    mFilteredTimeZoneIndices[mFilteredTimeZoneLength++] = defaultTzIndex;
                }

                // Show the recent selections
                SharedPreferences prefs = mContext.getSharedPreferences(SHARED_PREFS_NAME,
                        Context.MODE_PRIVATE);
                String recentsString = prefs.getString(KEY_RECENT_TIMEZONES, null);
                if (!TextUtils.isEmpty(recentsString)) {
                    String[] recents = recentsString.split(RECENT_TIMEZONES_DELIMITER);
                    for (int i = recents.length - 1; i >= 0; i--) {
                        if (!TextUtils.isEmpty(recents[i])
                                && !recents[i].equals(mTimeZoneData.mDefaultTimeZoneId)) {
                            int index = mTimeZoneData.findIndexByTimeZoneIdSlow(recents[i]);
                            if (index != -1) {
                                mFilteredTimeZoneIndices[mFilteredTimeZoneLength++] = index;
                            }
                        }
                    }
                }

                break;
            case TimeZoneFilterTypeAdapter.FILTER_TYPE_GMT:
                ArrayList<Integer> indices = mTimeZoneData.getTimeZonesByOffset(time);
                if (indices != null) {
                    for (Integer i : indices) {
                        mFilteredTimeZoneIndices[mFilteredTimeZoneLength++] = i;
                    }
                }
                break;
            case TimeZoneFilterTypeAdapter.FILTER_TYPE_COUNTRY:
                ArrayList<Integer> tzIds = mTimeZoneData.mTimeZonesByCountry.get(str);
                if (tzIds != null) {
                    for (Integer tzi : tzIds) {
                        mFilteredTimeZoneIndices[mFilteredTimeZoneLength++] = tzi;
                    }
                }
                break;
            case TimeZoneFilterTypeAdapter.FILTER_TYPE_STATE:
                // TODO Filter by state
                break;
            default:
                throw new IllegalArgumentException();
        }
        mHasResults = mFilteredTimeZoneLength > 0;

        notifyDataSetChanged();
    }

    /**
     * Saves the given timezone ID as a recent timezone under shared
     * preferences. If there are already the maximum number of recent timezones
     * saved, it will remove the oldest and append this one.
     *
     * @param id the ID of the timezone to save
     * @see {@link #MAX_RECENT_TIMEZONES}
     */
    public void saveRecentTimezone(String id) {
        SharedPreferences prefs = mContext.getSharedPreferences(SHARED_PREFS_NAME,
                Context.MODE_PRIVATE);
        String recentsString = prefs.getString(KEY_RECENT_TIMEZONES, null);
        if (recentsString == null) {
            recentsString = id;
        } else {
            // De-dup
            LinkedHashSet<String> recents = new LinkedHashSet<String>();
            for(String tzId : recentsString.split(RECENT_TIMEZONES_DELIMITER)) {
                if (!recents.contains(tzId) && !id.equals(tzId)) {
                    recents.add(tzId);
                }
            }

            Iterator<String> it = recents.iterator();
            while (recents.size() >= MAX_RECENT_TIMEZONES) {
                if (!it.hasNext()) {
                    break;
                }
                it.next();
                it.remove();
            }
            recents.add(id);

            StringBuilder builder = new StringBuilder();
            boolean first = true;
            for (String recent : recents) {
                if (first) {
                    first = false;
                } else {
                    builder.append(RECENT_TIMEZONES_DELIMITER);
                }
                builder.append(recent);
            }
            recentsString = builder.toString();
        }

        prefs.edit().putString(KEY_RECENT_TIMEZONES, recentsString).apply();
    }

    @Override
    public int getCount() {
        return mFilteredTimeZoneLength;
    }

    @Override
    public Object getItem(int position) {
        if (position < 0 || position >= mFilteredTimeZoneLength) {
            return null;
        }

        return mTimeZoneData.get(mFilteredTimeZoneIndices[position]);
    }

    @Override
    public boolean areAllItemsEnabled() {
        return false;
    }

    @Override
    public boolean isEnabled(int position) {
        return mFilteredTimeZoneIndices[position] >= 0;
    }

    @Override
    public long getItemId(int position) {
        return mFilteredTimeZoneIndices[position];
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View v = convertView;

        if (mFilteredTimeZoneIndices[position] == EMPTY_INDEX) {
            v = mInflater.inflate(R.layout.empty_time_zone_item, null);
            return v;
        }

        // We'll need to re-inflate the view if it was null, or if it was used as an empty item.
        if (v == null || v.findViewById(R.id.empty_item) != null) {
            v = mInflater.inflate(R.layout.time_zone_item, null);
            ViewHolder.setupViewHolder(v);
        }

        ViewHolder vh = (ViewHolder) v.getTag();

        TimeZoneInfo tzi = mTimeZoneData.get(mFilteredTimeZoneIndices[position]);
        v.setTag(VIEW_TAG_TIME_ZONE, tzi);

        vh.timeZone.setText(tzi.mDisplayName);

        vh.timeOffset.setText(tzi.getGmtDisplayName(mContext));

        String location = tzi.mCountry;
        if (location == null) {
            vh.location.setVisibility(View.INVISIBLE);
        } else {
            vh.location.setText(location);
            vh.location.setVisibility(View.VISIBLE);
        }

        return v;
    }

    @Override
    public boolean hasStableIds() {
        return true;
    }

    // Implements OnItemClickListener
    @Override
    public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
        if (mTimeZoneSetListener != null) {
            TimeZoneInfo tzi = (TimeZoneInfo) v.getTag(VIEW_TAG_TIME_ZONE);
            if (tzi != null) {
              mTimeZoneSetListener.onTimeZoneSet(tzi);
              saveRecentTimezone(tzi.mTzId);
            }
        }
    }
}
