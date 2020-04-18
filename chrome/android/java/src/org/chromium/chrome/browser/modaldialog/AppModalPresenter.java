// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.app.Dialog;
import android.content.Context;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

/** The presenter that shows a {@link ModalDialogView} in an Android dialog. */
public class AppModalPresenter extends ModalDialogManager.Presenter {
    private final Context mContext;
    private Dialog mDialog;

    public AppModalPresenter(Context context) {
        mContext = context;
    }

    @Override
    protected void addDialogView(View dialogView) {
        mDialog = new Dialog(mContext, R.style.ModalDialogTheme);
        mDialog.setOnCancelListener(dialogInterface -> cancelCurrentDialog());
        ViewGroup container = (ViewGroup) LayoutInflater.from(mContext).inflate(
                R.layout.modal_dialog_container, null);
        mDialog.setContentView(container);
        FrameLayout.LayoutParams params =
                new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER);
        dialogView.setBackgroundResource(R.drawable.menu_bg);
        container.addView(dialogView, params);
        if (getModalDialog().getCancelOnTouchOutside()) {
            mDialog.setCanceledOnTouchOutside(true);
            // The dialog container covers the entire screen. To achieve the cancel on touch outside
            // behavior cancel the dialog if the container is touched.
            container.setOnClickListener((v) -> mDialog.cancel());
        }
        mDialog.show();
    }

    @Override
    protected void removeDialogView(View dialogView) {
        // Dismiss the currently showing dialog.
        if (mDialog != null) mDialog.dismiss();
        mDialog = null;
    }
}
