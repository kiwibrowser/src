// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.permissions;

import android.content.DialogInterface;
import android.support.v4.widget.TextViewCompat;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;

/**
 * The Permission dialog that is either app modal or tab modal.
 */
public class PermissionDialogView {
    private AlertDialog mDialog;
    private PermissionDialogDelegate mDialogDelegate;

    /**
     * Constructor for the Dialog View. Creates the AlertDialog.
     */
    public PermissionDialogView(PermissionDialogDelegate delegate) {
        mDialogDelegate = delegate;
        ChromeActivity activity = mDialogDelegate.getTab().getActivity();
        AlertDialog.Builder builder = new AlertDialog.Builder(activity, R.style.AlertDialogTheme);
        mDialog = builder.create();
        mDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        mDialog.setCanceledOnTouchOutside(false);
    }

    /**
     * Prepares the dialog before show. Creates the View inside of the dialog,
     * and adds the buttons. Callbacks that are needed for buttons and dismiss
     * are the input.
     * @param positiveClickListener callback for positive button.
     * @param negativeClickListener callback for negative button.
     * @param dismissListener is called when user dismissed the dialog.
     */
    public void createView(DialogInterface.OnClickListener positiveClickListener,
            DialogInterface.OnClickListener negativeCliclListener,
            DialogInterface.OnDismissListener dismissListener) {
        ChromeActivity activity = mDialogDelegate.getTab().getActivity();
        LayoutInflater inflater = LayoutInflater.from(activity);
        View view = inflater.inflate(R.layout.permission_dialog, null);
        TextView messageTextView = (TextView) view.findViewById(R.id.text);
        String messageText = mDialogDelegate.getMessageText();
        assert !TextUtils.isEmpty(messageText);
        messageTextView.setText(messageText);
        messageTextView.setVisibility(View.VISIBLE);
        messageTextView.announceForAccessibility(messageText);
        TextViewCompat.setCompoundDrawablesRelativeWithIntrinsicBounds(
                messageTextView, mDialogDelegate.getDrawableId(), 0, 0, 0);
        mDialog.setView(view);
        mDialog.setButton(DialogInterface.BUTTON_POSITIVE, mDialogDelegate.getPrimaryButtonText(),
                positiveClickListener);
        mDialog.setButton(DialogInterface.BUTTON_NEGATIVE, mDialogDelegate.getSecondaryButtonText(),
                negativeCliclListener);
        mDialog.setOnDismissListener(dismissListener);
    }

    /* Shows the dialog */
    public void show() {
        mDialog.show();
    }

    /* Dismiss the dialog */
    public void dismiss() {
        mDialog.dismiss();
    }

    /**
     * Returns the {@link Button} from the dialog, or null if
     * a button does not exist.
     * @param whichButton The identifier of the button that should be returned.
     */
    public Button getButton(int whichButton) {
        return mDialog.getButton(whichButton);
    }
}
