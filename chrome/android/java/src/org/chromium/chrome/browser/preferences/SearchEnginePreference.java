// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.ListView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;

/**
* A preference fragment for selecting a default search engine.
*/
public class SearchEnginePreference extends PreferenceFragment {
    private ListView mListView;

    private SearchEngineAdapter mSearchEngineAdapter;

    @VisibleForTesting
    String getValueForTesting() {
        return mSearchEngineAdapter.getValueForTesting();
    }

    @VisibleForTesting
    String setValueForTesting(String value) {
        return mSearchEngineAdapter.setValueForTesting(value);
    }

    @VisibleForTesting
    String getKeywordFromIndexForTesting(int index) {
        return mSearchEngineAdapter.getKeywordForTesting(index);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_search_engine);
        mSearchEngineAdapter = new SearchEngineAdapter(getActivity());
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mListView = (ListView) getView().findViewById(android.R.id.list);
        int marginTop = getActivity().getResources().getDimensionPixelSize(
                R.dimen.search_engine_list_margin_top);
        MarginLayoutParams layoutParams = (MarginLayoutParams) mListView.getLayoutParams();
        layoutParams.setMargins(0, marginTop, 0, 0);
        mListView.setLayoutParams(layoutParams);
        mListView.setAdapter(mSearchEngineAdapter);
        mListView.setDivider(null);
    }

    @Override
    public void onStart() {
        super.onStart();
        mSearchEngineAdapter.start();
    }

    @Override
    public void onStop() {
        super.onStop();
        mSearchEngineAdapter.stop();
    }
}
