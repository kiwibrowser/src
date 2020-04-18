// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static org.chromium.chrome.browser.preferences.download.DownloadDirectoryAdapter.NO_SELECTED_ITEM_ID;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Spinner;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modaldialog.ModalDialogView;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.download.DownloadDirectoryAdapter;
import org.chromium.chrome.browser.widget.AlertDialogEditText;

import java.io.File;

import javax.annotation.Nullable;

/**
 * Dialog that is displayed to ask user where they want to download the file.
 */
public class DownloadLocationDialog extends ModalDialogView implements OnCheckedChangeListener {
    private DownloadDirectoryAdapter mDirectoryAdapter;

    private AlertDialogEditText mFileName;
    private Spinner mFileLocation;
    private CheckBox mDontShowAgain;

    /**
     * Create a {@link DownloadLocationDialog} with the given properties.
     *
     * @param controller    Controller that listens to the events from the dialog.
     * @param context       Context from which the dialog emerged.
     * @param totalBytes    The total bytes of the file. Can be 0 if size is unknown.
     * @param dialogType    Type of dialog that should be displayed, dictates title/subtitle.
     * @param suggestedPath The path that was automatically generated, used as a starting point.
     * @return              A {@link DownloadLocationDialog} with the given properties.
     */
    public static DownloadLocationDialog create(Controller controller, Context context,
            long totalBytes, @DownloadLocationDialogType int dialogType, File suggestedPath) {
        Params params = new Params();
        params.positiveButtonTextId = R.string.duplicate_download_infobar_download_button;
        params.negativeButtonTextId = R.string.cancel;
        params.customView =
                LayoutInflater.from(context).inflate(R.layout.download_location_dialog, null);

        params.title = context.getString(R.string.download_location_dialog_title);
        TextView subtitleText = params.customView.findViewById(R.id.subtitle);
        subtitleText.setVisibility(
                dialogType == DownloadLocationDialogType.DEFAULT ? View.GONE : View.VISIBLE);

        switch (dialogType) {
            case DownloadLocationDialogType.LOCATION_FULL:
                params.title = context.getString(R.string.download_location_not_enough_space);
                subtitleText.setText(R.string.download_location_download_to_default_folder);
                break;

            case DownloadLocationDialogType.LOCATION_NOT_FOUND:
                params.title = context.getString(R.string.download_location_no_sd_card);
                subtitleText.setText(R.string.download_location_download_to_default_folder);
                break;

            case DownloadLocationDialogType.NAME_CONFLICT:
                params.title = context.getString(R.string.download_location_download_again);
                subtitleText.setText(R.string.download_location_name_exists);
                break;

            case DownloadLocationDialogType.NAME_TOO_LONG:
                params.title = context.getString(R.string.download_location_rename_file);
                subtitleText.setText(R.string.download_location_name_too_long);
                break;

            case DownloadLocationDialogType.DEFAULT:
                if (totalBytes > 0) {
                    StringBuilder title = new StringBuilder(params.title);
                    title.append(" ");
                    title.append(DownloadUtils.getStringForBytes(context, totalBytes));
                    params.title = title.toString();
                }
                break;
        }

        return new DownloadLocationDialog(controller, context, dialogType, suggestedPath, params);
    }

    private DownloadLocationDialog(Controller controller, Context context,
            @DownloadLocationDialogType int dialogType, File suggestedPath, Params params) {
        super(controller, params);

        mDirectoryAdapter = new DownloadDirectoryAdapter(context);

        mFileName = (AlertDialogEditText) params.customView.findViewById(R.id.file_name);
        mFileName.setText(suggestedPath.getName());

        mFileLocation = (Spinner) params.customView.findViewById(R.id.file_location);
        mFileLocation.setAdapter(mDirectoryAdapter);

        int selectedItemId = mDirectoryAdapter.getSelectedItemId();
        if (selectedItemId == NO_SELECTED_ITEM_ID
                || dialogType == DownloadLocationDialogType.LOCATION_FULL
                || dialogType == DownloadLocationDialogType.LOCATION_NOT_FOUND) {
            selectedItemId = mDirectoryAdapter.useFirstValidSelectableItemId();
        }
        mFileLocation.setSelection(selectedItemId);

        // Automatically check "don't show again" the first time the user is seeing the dialog.
        mDontShowAgain = (CheckBox) params.customView.findViewById(R.id.show_again_checkbox);
        boolean isInitial = PrefServiceBridge.getInstance().getPromptForDownloadAndroid()
                == DownloadPromptStatus.SHOW_INITIAL;
        mDontShowAgain.setChecked(isInitial);
        mDontShowAgain.setOnCheckedChangeListener(this);
    }

    // CompoundButton.OnCheckedChangeListener implementation.
    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        int newStatus =
                isChecked ? DownloadPromptStatus.DONT_SHOW : DownloadPromptStatus.SHOW_PREFERENCE;
        PrefServiceBridge.getInstance().setPromptForDownloadAndroid(newStatus);
    }

    // Helper methods available to DownloadLocationDialogBridge.

    /**
     * @return  The text that the user inputted as the name of the file.
     */
    @Nullable
    String getFileName() {
        if (mFileName == null) return null;
        return mFileName.getText().toString();
    }

    /**
     * @return  The file path based on what the user selected as the location of the file.
     */
    @Nullable
    DirectoryOption getDirectoryOption() {
        if (mFileLocation == null) return null;
        DirectoryOption selected = (DirectoryOption) mFileLocation.getSelectedItem();
        return selected;
    }

    /**
     * @return  Whether the "don't show again" checkbox is checked.
     */
    boolean getDontShowAgain() {
        return mDontShowAgain != null && mDontShowAgain.isChecked();
    }
}
