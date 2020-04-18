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

package com.example.bitmapsample;

import android.app.Activity;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;

import com.android.bitmap.BitmapCache;
import com.android.bitmap.DecodeAggregator;
import com.android.bitmap.UnrefedBitmapCache;
import com.android.bitmap.drawable.ExtendedBitmapDrawable;
import com.android.bitmap.drawable.ExtendedBitmapDrawable.ExtendedOptions;

public class MainActivity extends Activity {

    private ListView mListView;
    private final BitmapCache mCache = new UnrefedBitmapCache(TARGET_CACHE_SIZE_BYTES, 0, 0);
    private final DecodeAggregator mDecodeAggregator = new DecodeAggregator();

    private static Drawable PLACEHOLDER;
    private static Drawable PROGRESS;

    private static final float NORMAL_PARALLAX_MULTIPLIER = 1.5f;
    private static final int TARGET_CACHE_SIZE_BYTES = 5 * 1024 * 1024;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (PLACEHOLDER == null) {
            Resources res = getResources();
            PLACEHOLDER = res.getDrawable(R.drawable.ic_placeholder);
            PROGRESS = res.getDrawable(R.drawable.progress);
        }

        mListView = (ListView) findViewById(R.id.list);
        mListView.setAdapter(new MyAdapter());
    }

    private class MyAdapter extends BaseAdapter {

        private final String[] mItems;

        private final String[] ITEMS = new String[]{
                "https://www.google.com/images/srpr/logo4w.png",
                "http://www.google.com/logos/2012/celibidache12-hp.jpg",
                "http://www.google.com/logos/2012/clara_schuman-2012-hp.jpg",
                "http://www.google.com/logos/2011/royalwedding11-hp.png",
                "http://www.google.com/logos/2012/vets_day-12-hp.jpg",
                "http://www.google.com/logos/2011/firstmaninspace11-hp-js.jpg",
                "http://www.google.com/logos/2011/nat-sov-and-childrens-turkey-hp.png",
                "http://www.google.com/logos/2012/First_Day_Of_School_Isreal-2012-hp.jpg",
                "http://www.google.com/logos/2012/celibidache12-hp.jpg",
                "http://www.google.com/logos/2012/korea12-hp.png"
        };

        private static final int COPIES = 50;

        public MyAdapter() {
            mItems = new String[ITEMS.length * COPIES];
            for (int i = 0; i < COPIES; i++) {
                for (int j = 0; j < ITEMS.length; j++) {
                    mItems[i * ITEMS.length + j] = ITEMS[j];
                }
            }
        }

        @Override
        public int getCount() {
            return mItems.length;
        }

        @Override
        public Object getItem(int position) {
            return mItems[position];
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            BitmapView v;
            if (convertView != null) {
                v = (BitmapView) convertView;
            } else {
                v = new BitmapView(MainActivity.this);
                ExtendedOptions opts = new ExtendedOptions(
                        ExtendedOptions.FEATURE_ORDERED_DISPLAY | ExtendedOptions.FEATURE_PARALLAX
                                | ExtendedOptions.FEATURE_STATE_CHANGES, PLACEHOLDER, PROGRESS);
                opts.decodeAggregator = mDecodeAggregator;
                opts.parallaxSpeedMultiplier = NORMAL_PARALLAX_MULTIPLIER;
                opts.backgroundColor = Color.LTGRAY;
                final ExtendedBitmapDrawable drawable = new ExtendedBitmapDrawable(getResources(),
                        mCache, true /* limit density */, opts);

                v.setTypedDrawable(drawable);
                v.setListView(mListView);
            }
            v.getTypedDrawable().bind(new BitmapRequestKeyImpl(mItems[position]));
            return v;
        }
    }
}
