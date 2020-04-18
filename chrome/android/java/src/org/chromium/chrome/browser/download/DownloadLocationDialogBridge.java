// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.modaldialog.ModalDialogManager;
import org.chromium.chrome.browser.modaldialog.ModalDialogView;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.ui.base.WindowAndroid;

import java.io.File;

/**
 * Helper class to handle communication between download location dialog and native.
 */
public class DownloadLocationDialogBridge implements ModalDialogView.Controller {
    private long mNativeDownloadLocationDialogBridge;
    private DownloadLocationDialog mLocationDialog;
    private ModalDialogManager mModalDialogManager;

    private DownloadLocationDialogBridge(long nativeDownloadLocationDialogBridge) {
        mNativeDownloadLocationDialogBridge = nativeDownloadLocationDialogBridge;
    }

    @CalledByNative
    public static DownloadLocationDialogBridge create(long nativeDownloadLocationDialogBridge) {
        return new DownloadLocationDialogBridge(nativeDownloadLocationDialogBridge);
    }

    @CalledByNative
    private void destroy() {
        mNativeDownloadLocationDialogBridge = 0;
        if (mModalDialogManager != null) mModalDialogManager.dismissDialog(mLocationDialog);
    }

    @CalledByNative
    public void showDialog(WindowAndroid windowAndroid, long totalBytes,
            @DownloadLocationDialogType int dialogType, String suggestedPath) {
        ChromeActivity activity = (ChromeActivity) windowAndroid.getActivity().get();
        // If the activity has gone away, just clean up the native pointer.
        if (activity == null) {
            onCancel();
            return;
        }

        mModalDialogManager = activity.getModalDialogManager();

        if (mLocationDialog != null) return;
        mLocationDialog = DownloadLocationDialog.create(
                this, activity, totalBytes, dialogType, new File(suggestedPath));

        mModalDialogManager.showDialog(mLocationDialog, ModalDialogManager.APP_MODAL);
    }

    @Override
    public void onClick(@ModalDialogView.ButtonType int buttonType) {
        switch (buttonType) {
            case ModalDialogView.BUTTON_POSITIVE:
                handleResponses(mLocationDialog.getFileName(), mLocationDialog.getDirectoryOption(),
                        mLocationDialog.getDontShowAgain());
                mModalDialogManager.dismissDialog(mLocationDialog);
                break;
            case ModalDialogView.BUTTON_NEGATIVE:
            // Intentional fall-through.
            default:
                cancel();
                mModalDialogManager.dismissDialog(mLocationDialog);
        }

        mLocationDialog = null;
    }

    @Override
    public void onCancel() {
        cancel();
        mLocationDialog = null;
    }

    @Override
    public void onDismiss() {}

    /**
     * Pass along information from location dialog to native.
     *
     * @param fileName      Name the user gave the file.
     * @param directoryOption  Location the user wants the file saved to.
     * @param dontShowAgain Whether the user wants the "Save download to..." dialog shown again.
     */
    private void handleResponses(
            String fileName, DirectoryOption directoryOption, boolean dontShowAgain) {
        // If there's no file location, treat as a cancellation.
        if (directoryOption == null || directoryOption.location == null) {
            cancel();
            return;
        }

        // Update native with new path.
        if (mNativeDownloadLocationDialogBridge != 0) {
            PrefServiceBridge.getInstance().setDownloadAndSaveFileDefaultDirectory(
                    directoryOption.location);
            directoryOption.recordDirectoryOptionType();

            File file = new File(directoryOption.location, fileName);
            nativeOnComplete(mNativeDownloadLocationDialogBridge, file.getAbsolutePath());
        }

        // Update preference to show prompt based on whether checkbox is checked.
        if (dontShowAgain) {
            PrefServiceBridge.getInstance().setPromptForDownloadAndroid(
                    DownloadPromptStatus.DONT_SHOW);
        } else {
            PrefServiceBridge.getInstance().setPromptForDownloadAndroid(
                    DownloadPromptStatus.SHOW_PREFERENCE);
        }
    }

    private void cancel() {
        if (mNativeDownloadLocationDialogBridge != 0) {
            nativeOnCanceled(mNativeDownloadLocationDialogBridge);
        }
    }

    public native void nativeOnComplete(
            long nativeDownloadLocationDialogBridge, String returnedPath);
    public native void nativeOnCanceled(long nativeDownloadLocationDialogBridge);
}
