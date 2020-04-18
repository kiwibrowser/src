// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import org.chromium.base.ThreadUtils;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.locale.DefaultSearchEngineDialogHelper;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.locale.LocaleManager.SearchEnginePromoType;
import org.chromium.chrome.browser.search_engines.TemplateUrlService;
import org.chromium.chrome.browser.widget.RadioButtonLayout;

/** A {@link Fragment} that presents a set of search engines for the user to choose from. */
public class DefaultSearchEngineFirstRunFragment extends Fragment implements FirstRunFragment {
    /** FRE page that instantiates this fragment. */
    public static class Page implements FirstRunPage<DefaultSearchEngineFirstRunFragment> {
        @Override
        public DefaultSearchEngineFirstRunFragment instantiateFragment() {
            return new DefaultSearchEngineFirstRunFragment();
        }
    }

    @SearchEnginePromoType
    private int mSearchEnginePromoDialoType;
    private boolean mShownRecorded;

    /** Layout that displays the available search engines to the user. */
    private RadioButtonLayout mEngineLayout;

    /** The button that lets a user proceed to the next page after an engine is selected. */
    private Button mButton;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(
                R.layout.default_search_engine_first_run_fragment, container, false);
        mEngineLayout = (RadioButtonLayout) rootView.findViewById(
                R.id.default_search_engine_dialog_options);
        mButton = (Button) rootView.findViewById(R.id.button_primary);
        mButton.setEnabled(false);

        assert TemplateUrlService.getInstance().isLoaded();
        mSearchEnginePromoDialoType = LocaleManager.getInstance().getSearchEnginePromoShowType();
        if (mSearchEnginePromoDialoType != LocaleManager.SEARCH_ENGINE_PROMO_DONT_SHOW) {
            Runnable dismissRunnable = new Runnable() {
                @Override
                public void run() {
                    getPageDelegate().advanceToNextPage();
                }
            };
            new DefaultSearchEngineDialogHelper(
                    mSearchEnginePromoDialoType, mEngineLayout, mButton, dismissRunnable);
        }

        return rootView;
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);

        if (isVisibleToUser) {
            if (mSearchEnginePromoDialoType == LocaleManager.SEARCH_ENGINE_PROMO_DONT_SHOW) {
                ThreadUtils.postOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        getPageDelegate().advanceToNextPage();
                    }
                });
            }

            recordShown();
        }
    }

    private void recordShown() {
        if (mShownRecorded) return;

        if (mSearchEnginePromoDialoType == LocaleManager.SEARCH_ENGINE_PROMO_SHOW_NEW) {
            RecordUserAction.record("SearchEnginePromo.NewDevice.Shown.FirstRun");
        } else if (mSearchEnginePromoDialoType == LocaleManager.SEARCH_ENGINE_PROMO_SHOW_EXISTING) {
            RecordUserAction.record("SearchEnginePromo.ExistingDevice.Shown.FirstRun");
        }

        mShownRecorded = true;
    }
}
