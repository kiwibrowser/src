// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.app.Activity;
import android.os.Bundle;
import android.preference.Preference;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.payments.AddressEditor;
import org.chromium.chrome.browser.payments.AutofillAddress;
import org.chromium.chrome.browser.payments.SettingsAutofillAndPaymentsObserver;
import org.chromium.chrome.browser.widget.prefeditor.EditorDialog;
import org.chromium.chrome.browser.widget.prefeditor.EditorObserverForTest;

import android.view.View;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.TextView;

/**
 * Launches the UI to edit, create or delete an Autofill profile entry.
 */
public class AutofillProfileEditorPreference extends Preference {
    final private Activity mActivity;
    final private EditorObserverForTest mObserverForTest;
    private EditorDialog mEditorDialog;
    private AutofillAddress mAutofillAddress;
    private String mGUID;

    public AutofillProfileEditorPreference(
            Activity activity, EditorObserverForTest observerForTest) {
        super(activity);
        mActivity = activity;
        mObserverForTest = observerForTest;
    }

    public EditorDialog getEditorDialog() {
        return mEditorDialog;
    }

    @Override
    protected void onClick() {
        Bundle extras = getExtras();
        // We know which profile to edit based on the GUID stuffed in our extras
        // by AutofillAndPaymentsPreferences.
        mGUID = extras.getString(AutofillAndPaymentsPreferences.AUTOFILL_GUID);
        prepareEditorDialog();
        prepareAddressEditor();
    }

    private void prepareAddressEditor() {
        AddressEditor addressEditor =
                new AddressEditor(/*emailFieldIncluded=*/true, /*saveToDisk=*/true);
        addressEditor.setEditorDialog(mEditorDialog);

        addressEditor.edit(mAutofillAddress, new Callback<AutofillAddress>() {
            /*
             * There are four cases for |address| here.
             * (1) |address| is null: the user canceled address creation
             * (2) |address| is non-null: the user canceled editing an existing address
             * (3) |address| is non-null: the user edited an existing address.
             * (4) |address| is non-null: the user created a new address.
             * We should save the changes (set the profile) for cases 3 and 4,
             * and it's OK to set the profile for 2.
             */
            @Override
            public void onResult(AutofillAddress address) {
                if (address != null) {
                    PersonalDataManager.getInstance().setProfile(address.getProfile());
                    SettingsAutofillAndPaymentsObserver.getInstance().notifyOnAddressUpdated(
                            address);
                }
                if (mObserverForTest != null) {
                    mObserverForTest.onEditorReadyToEdit();
                }
            }
        });
    }

    private void prepareEditorDialog() {
        Runnable runnable = null;
        mAutofillAddress = null;
        if (mGUID != null) {
            mAutofillAddress = new AutofillAddress(
                    mActivity, PersonalDataManager.getInstance().getProfile(mGUID));
            runnable = new Runnable() {
                @Override
                public void run() {
                    if (mGUID != null) {
                        PersonalDataManager.getInstance().deleteProfile(mGUID);
                        SettingsAutofillAndPaymentsObserver.getInstance().notifyOnAddressDeleted(
                                mGUID);
                    }
                    if (mObserverForTest != null) {
                        mObserverForTest.onEditorReadyToEdit();
                    }
                }
            };
        }
        mEditorDialog = new EditorDialog(mActivity, mObserverForTest, runnable);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            if (((TextView) view.findViewById(android.R.id.title)) != null)
               ((TextView) view.findViewById(android.R.id.title)).setTextColor(Color.WHITE);
            if (((TextView) view.findViewById(android.R.id.summary)) != null)
               ((TextView) view.findViewById(android.R.id.summary)).setTextColor(Color.GRAY);
        }
    }
}
