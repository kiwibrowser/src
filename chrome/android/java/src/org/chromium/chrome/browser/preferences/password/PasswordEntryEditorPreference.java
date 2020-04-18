// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import android.app.Activity;
import android.os.Bundle;
import android.preference.Preference;

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
}
