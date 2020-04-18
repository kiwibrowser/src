// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.v7.content.res.AppCompatResources;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

/**
 * Autofill credit cards fragment, which allows the user to edit credit cards.
 */
public class AutofillCreditCardsFragment
        extends PreferenceFragment implements PersonalDataManager.PersonalDataManagerObserver {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceUtils.addPreferencesFromResource(
                this, R.xml.autofill_and_payments_preference_fragment_screen);
        getActivity().setTitle(R.string.autofill_credit_cards_title);
    }

    @Override
    public void onResume() {
        super.onResume();
        // Always rebuild our list of credit cards.  Although we could detect if credit cards are
        // added or deleted, the credit card summary (number) might be different.  To be safe, we
        // update all.
        rebuildCreditCardList();
    }

    private void rebuildCreditCardList() {
        getPreferenceScreen().removeAll();
        getPreferenceScreen().setOrderingAsAdded(true);

        for (CreditCard card : PersonalDataManager.getInstance().getCreditCardsForSettings()) {
            // Add a preference for the credit card.
            Preference pref = new Preference(getActivity());
            pref.setTitle(card.getObfuscatedNumber());
            pref.setSummary(card.getFormattedExpirationDate(getActivity()));
            pref.setIcon(
                    AppCompatResources.getDrawable(getActivity(), card.getIssuerIconDrawableId()));

            if (card.getIsLocal()) {
                pref.setFragment(AutofillLocalCardEditor.class.getName());
            } else {
                pref.setFragment(AutofillServerCardEditor.class.getName());
                pref.setWidgetLayoutResource(R.layout.autofill_server_data_label);
            }

            Bundle args = pref.getExtras();
            args.putString(AutofillAndPaymentsPreferences.AUTOFILL_GUID, card.getGUID());
            getPreferenceScreen().addPreference(pref);
        }

        // Add 'Add credit card' button. Tap of it brings up card editor which allows users type in
        // new credit cards.
        Preference pref = new Preference(getActivity());
        Drawable plusIcon = ApiCompatibilityUtils.getDrawable(getResources(), R.drawable.plus);
        plusIcon.mutate();
        plusIcon.setColorFilter(
                ApiCompatibilityUtils.getColor(getResources(), R.color.pref_accent_color),
                PorterDuff.Mode.SRC_IN);
        pref.setIcon(plusIcon);
        pref.setTitle(R.string.autofill_create_credit_card);
        pref.setFragment(AutofillLocalCardEditor.class.getName());
        getPreferenceScreen().addPreference(pref);
    }

    @Override
    public void onPersonalDataChanged() {
        rebuildCreditCardList();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        PersonalDataManager.getInstance().registerDataObserver(this);
    }

    @Override
    public void onDestroyView() {
        PersonalDataManager.getInstance().unregisterDataObserver(this);
        super.onDestroyView();
    }
}
