// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.support.v7.app.AlertDialog;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RatingBar;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.banners.AppBannerManager;

/**
 * Displays the "Add to Homescreen" dialog, which contains a (possibly editable) title, icon, and
 * possibly an origin.
 *
 * When show() is invoked, the dialog is shown immediately. A spinner is displayed if any data is
 * not yet fetched, and accepting the dialog is disabled until all data is available and in its
 * place on the screen.
 */
public class AddToHomescreenDialog implements View.OnClickListener {
    /**
     * The delegate for which this dialog is displayed. Used by the dialog to indicate when the user
     * accedes to adding to home screen, and when the dialog is dismissed.
     */
    public static interface Delegate {
        /**
         * Called when the user accepts adding the item to the home screen with the provided title.
         */
        void addToHomescreen(String title);

        /**
         * Called when the dialog is explicitly cancelled by the user.
         */
        void onDialogCancelled();

        /**
         * Called when the user wants to view a native app in the Play Store.
         */
        void onNativeAppDetailsRequested();

        /**
         * Called when the dialog's lifetime is over and it disappears from the screen.
         */
        void onDialogDismissed();
    }

    private AlertDialog mDialog;
    private View mProgressBarView;
    private ImageView mIconView;

    /**
     * The {@mShortcutTitleInput} and the {@mAppLayout} are mutually exclusive, depending on whether
     * the home screen item is a bookmark shortcut or a web/native app.
     */
    private EditText mShortcutTitleInput;
    private LinearLayout mAppLayout;

    private TextView mAppNameView;
    private TextView mAppOriginView;
    private RatingBar mAppRatingBar;
    private ImageView mPlayLogoView;

    private Activity mActivity;
    private Delegate mDelegate;

    private boolean mHasIcon;

    public AddToHomescreenDialog(Activity activity, Delegate delegate) {
        mActivity = activity;
        mDelegate = delegate;
    }

    @VisibleForTesting
    public AlertDialog getAlertDialogForTesting() {
        return mDialog;
    }

    /**
     * Shows the dialog for adding a shortcut to the home screen.
     * @param activity The current activity in which to create the dialog.
     */
    public void show() {
        View view = mActivity.getLayoutInflater().inflate(R.layout.add_to_homescreen_dialog, null);
        AlertDialog.Builder builder =
                new AlertDialog.Builder(mActivity, R.style.AlertDialogTheme)
                        .setTitle(AppBannerManager.getHomescreenLanguageOption())
                        .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int id) {
                                mDelegate.onDialogCancelled();
                                dialog.cancel();
                            }
                        });

        mDialog = builder.create();
        mDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        // On click of the menu item for "add to homescreen", an alert dialog pops asking the user
        // if the title needs to be edited. On click of "Add", shortcut is created. Default
        // title is the title of the page.
        mProgressBarView = view.findViewById(R.id.spinny);
        mIconView = (ImageView) view.findViewById(R.id.icon);
        mShortcutTitleInput = (EditText) view.findViewById(R.id.text);
        mAppLayout = (LinearLayout) view.findViewById(R.id.app_info);

        mAppNameView = (TextView) mAppLayout.findViewById(R.id.name);
        mAppOriginView = (TextView) mAppLayout.findViewById(R.id.origin);
        mAppRatingBar = (RatingBar) mAppLayout.findViewById(R.id.control_rating);
        mPlayLogoView = (ImageView) view.findViewById(R.id.play_logo);

        // The dialog's text field is disabled till the "user title" is fetched,
        mShortcutTitleInput.setVisibility(View.INVISIBLE);

        view.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                if (mProgressBarView.getMeasuredHeight() == mShortcutTitleInput.getMeasuredHeight()
                        && mShortcutTitleInput.getBackground() != null) {
                    // Force the text field to align better with the icon by accounting for the
                    // padding introduced by the background drawable.
                    mShortcutTitleInput.getLayoutParams().height =
                            mProgressBarView.getMeasuredHeight()
                            + mShortcutTitleInput.getPaddingBottom();
                    v.requestLayout();
                    v.removeOnLayoutChangeListener(this);
                }
            }
        });

        // The "Add" button should be disabled if the dialog's text field is empty.
        mShortcutTitleInput.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void afterTextChanged(Editable editableText) {
                updateAddButtonEnabledState();
            }
        });

        mDialog.setView(view);
        mDialog.setButton(DialogInterface.BUTTON_POSITIVE,
                mActivity.getResources().getString(R.string.add),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        mDelegate.addToHomescreen(mShortcutTitleInput.getText().toString());
                    }
                });

        mDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d) {
                updateAddButtonEnabledState();
            }
        });

        mDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mDialog = null;
                mDelegate.onDialogDismissed();
            }
        });

        mDialog.show();
    }

    /**
     * Called when the home screen icon title (and possibly information from the web manifest) is
     * available. Used for web apps and bookmark shortcuts.
     * @param title    Text to be displayed in the title field.
     * @param url      URL of the web app/shortcut.
     * @param isWebapp True if this is for a web app, and false otherwise.
     */
    public void onUserTitleAvailable(String title, String url, boolean isWebapp) {
        // Users may edit the title of bookmark shortcuts, but we respect web app names and do not
        // let users change them.
        if (isWebapp) {
            mShortcutTitleInput.setVisibility(View.GONE);
            mAppNameView.setText(title);
            mAppOriginView.setText(url);
            mAppRatingBar.setVisibility(View.GONE);
            mPlayLogoView.setVisibility(View.GONE);
            mAppLayout.setVisibility(View.VISIBLE);
            return;
        }

        mShortcutTitleInput.setText(title);
        mShortcutTitleInput.setVisibility(View.VISIBLE);
    }

    /**
     * Called when the home screen icon title, install text, and app rating are available. Used for
     * native apps.
     * @param title       Text to be displayed in the title field
     * @param installText String to be displayed on the positive button
     * @param rating      The rating of the app in the store.
     */
    public void onUserTitleAvailable(String title, String installText, float rating) {
        mShortcutTitleInput.setVisibility(View.GONE);
        mAppNameView.setText(title);
        mAppOriginView.setVisibility(View.GONE);
        mAppRatingBar.setRating(rating);
        mPlayLogoView.setImageResource(R.drawable.google_play);
        mAppLayout.setVisibility(View.VISIBLE);

        // Update the text on the primary button.
        Button button = mDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        button.setText(installText);
        button.setContentDescription(mActivity.getString(
                R.string.app_banner_view_native_app_install_accessibility, installText));

        // Clicking on the app title or the icon will open the Play Store for more details.
        mAppNameView.setOnClickListener(this);
        mIconView.setOnClickListener(this);
    }

    /**
     * Called when the home screen icon is available. Must be called after onUserTitleAvailable().
     * @param icon Icon to use in the launcher.
     */
    public void onIconAvailable(Bitmap icon) {
        mProgressBarView.setVisibility(View.GONE);
        mIconView.setVisibility(View.VISIBLE);
        mIconView.setImageBitmap(icon);

        mHasIcon = true;
        updateAddButtonEnabledState();
    }

    @Override
    public void onClick(View v) {
        if (v == mAppNameView || v == mIconView) {
            mDelegate.onNativeAppDetailsRequested();
            mDialog.cancel();
        }
    }

    void updateAddButtonEnabledState() {
        boolean enable = mHasIcon
                && (!TextUtils.isEmpty(mShortcutTitleInput.getText())
                           || mAppLayout.getVisibility() == View.VISIBLE);
        mDialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(enable);
    }
}
