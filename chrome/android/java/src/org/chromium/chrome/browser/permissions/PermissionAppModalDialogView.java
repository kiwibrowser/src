// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.permissions;

import android.support.v4.widget.TextViewCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modaldialog.ModalDialogView;

/**
 * The Permission dialog that is app modal.
 */
public class PermissionAppModalDialogView extends ModalDialogView {
    private PermissionDialogDelegate mDialogDelegate;
    private View mView;

    /**
     * Constructor for the Dialog View. Creates the AlertDialog.
     */
    public static PermissionAppModalDialogView create(
            Controller controller, PermissionDialogDelegate delegate) {
        Params params = new Params();
        params.positiveButtonText = delegate.getPrimaryButtonText();
        params.negativeButtonText = delegate.getSecondaryButtonText();
        return new PermissionAppModalDialogView(controller, params, delegate);
    }

    private PermissionAppModalDialogView(
            Controller controller, Params params, PermissionDialogDelegate delegate) {
        super(controller, params);
        mDialogDelegate = delegate;
        params.customView = createView();
    }

    @Override
    protected void prepareBeforeShow() {
        super.prepareBeforeShow();
        TextView messageTextView = (TextView) mView.findViewById(R.id.text);
        messageTextView.setText(prepareMainMessageString(mDialogDelegate));
        messageTextView.setVisibility(View.VISIBLE);
        messageTextView.announceForAccessibility(mDialogDelegate.getMessageText());
        TextViewCompat.setCompoundDrawablesRelativeWithIntrinsicBounds(
                messageTextView, mDialogDelegate.getDrawableId(), 0, 0, 0);
    }

    /**
     * Prepares the dialog before show. Creates the View inside of the dialog.
     */
    private View createView() {
        LayoutInflater inflater = LayoutInflater.from(getContext());
        mView = inflater.inflate(R.layout.permission_dialog, null);
        return mView;
    }

    private CharSequence prepareMainMessageString(final PermissionDialogDelegate delegate) {
        String messageText = delegate.getMessageText();
        assert !TextUtils.isEmpty(messageText);

        // TODO(timloh): Currently the strings are shared with infobars, so we for now manually
        // remove the full stop (this code catches most but not all languages). Update the strings
        // after removing the infobar path.
        if (messageText.endsWith(".") || messageText.endsWith("。")) {
            messageText = messageText.substring(0, messageText.length() - 1);
        }

        return messageText;
    }
}
