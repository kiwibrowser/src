/*
 * Copyright (C) 2015 The Android Open Source Project
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
package android.databinding.adapters;

import android.content.Context;
import android.databinding.ObservableList;
import android.databinding.ObservableList.OnListChangedCallback;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import java.util.List;

class ObservableListAdapter<T> extends BaseAdapter {
    private List<T> mList;
    private ObservableList.OnListChangedCallback mListChangedCallback;
    private final Context mContext;
    private final int mDropDownResourceId;
    private final int mResourceId;
    private final int mTextViewResourceId;
    private final LayoutInflater mLayoutInflater;

    public ObservableListAdapter(Context context, List<T> list, int resourceId,
            int dropDownResourceId, int textViewResourceId) {
        mContext = context;
        mResourceId = resourceId;
        mDropDownResourceId = dropDownResourceId;
        mTextViewResourceId = textViewResourceId;
        mLayoutInflater = (resourceId == 0) ? null :
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        setList(list);
    }

    public void setList(List<T> list) {
        if (mList == list) {
            return;
        }
        if (mList instanceof ObservableList) {
            ((ObservableList) mList).removeOnListChangedCallback(mListChangedCallback);
        }
        mList = list;
        if (mList instanceof ObservableList) {
            if (mListChangedCallback == null) {
                mListChangedCallback = new OnListChangedCallback() {
                    @Override
                    public void onChanged(ObservableList observableList) {
                        notifyDataSetChanged();
                    }

                    @Override
                    public void onItemRangeChanged(ObservableList observableList, int i,
                            int i1) {
                        notifyDataSetChanged();
                    }

                    @Override
                    public void onItemRangeInserted(ObservableList observableList, int i,
                            int i1) {
                        notifyDataSetChanged();
                    }

                    @Override
                    public void onItemRangeMoved(ObservableList observableList, int i, int i1,
                            int i2) {
                        notifyDataSetChanged();
                    }

                    @Override
                    public void onItemRangeRemoved(ObservableList observableList, int i,
                            int i1) {
                        notifyDataSetChanged();
                    }
                };
            }
            ((ObservableList) mList).addOnListChangedCallback(mListChangedCallback);
        }
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mList.size();
    }

    @Override
    public Object getItem(int position) {
        return mList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        return getViewForResource(mResourceId, position, convertView, parent);
    }

    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
        return getViewForResource(mDropDownResourceId, position, convertView, parent);
    }

    public View getViewForResource(int resourceId, int position, View convertView,
            ViewGroup parent) {
        if (convertView == null) {
            if (resourceId == 0) {
                convertView = new TextView(mContext);
            } else {
                convertView = mLayoutInflater.inflate(resourceId, parent, false);
            }
        }
        TextView text = (TextView) (mTextViewResourceId == 0 ? convertView :
                convertView.findViewById(mTextViewResourceId));
        T item = mList.get(position);
        CharSequence value;
        if (item instanceof CharSequence) {
            value = (CharSequence) item;
        } else {
            value = String.valueOf(item);
        }
        text.setText(value);
        return convertView;
    }
}
