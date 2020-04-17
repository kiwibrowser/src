// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import android.app.Activity;
import android.os.Bundle;
import android.preference.Preference;

import android.view.View;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.TextView;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.widget.prefeditor.EditorDialog;
import org.chromium.chrome.browser.widget.prefeditor.PasswordEditor;

/**
 * Launches the UI to create a credential.
 */
public class PasswordEntryEditorPreference extends Preference {
    final private Activity mActivity;
    private EditorDialog mEditorDialog;
    private Bundle mExtras;

    public PasswordEntryEditorPreference(Activity activity) {
        super(activity);
        mActivity = activity;
    }

    /** @return The common editor user interface. */
    public EditorDialog getEditorDialog() {
        return mEditorDialog;
    }

    @Override
    protected void onClick() {
        mExtras = getExtras();
        preparePasswordEditor();
    }

    private void preparePasswordEditor() {
        mEditorDialog = new EditorDialog(mActivity, null /* observer */, null /* runnable */);
        PasswordEditor passwordEditor = new PasswordEditor();
        passwordEditor.setEditorDialog(mEditorDialog);

        passwordEditor.edit(null /* toEdit */, new Callback<SavedPasswordEntry>() {
            @Override
            public void onResult(SavedPasswordEntry credential) {
                PasswordManagerHandlerProvider.getInstance()
                        .getPasswordManagerHandler()
                        .updatePasswordLists();
            }
        });
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
