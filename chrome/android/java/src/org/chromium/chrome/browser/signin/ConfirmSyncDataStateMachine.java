// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Context;
import android.os.Handler;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentManager;
import android.text.TextUtils;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.signin.ConfirmImportSyncDataDialog.ImportSyncType;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * This class takes care of the various dialogs that must be shown when the user changes the
 * account they are syncing to (either directly, or by signing in to a new account). Most of the
 * complexity is due to many of the decisions getting answered through callbacks.
 *
 * This class progresses along the following state machine:
 *
 *       E-----\  G--\
 *       ^     |  ^  |
 *       |     v  |  v
 * A->B->C->D->+->F->H
 *    |        ^
 *    v        |
 *    \--------/
 *
 * Where:
 * A - Start
 * B - Decision: progress to C if the user signed in previously to a different account, F otherwise.
 * C - Decision: progress to E if we are switching from a managed account, D otherwise.
 * D - Action: show Import Data Dialog.
 * E - Action: show Switching from Managed Account Dialog.
 * F - Decision: progress to G if we are switching to a managed account, H otherwise.
 * G - Action: show Switching to Managed Account Dialog.
 * H - End: perform {@link ConfirmImportSyncDataDialog.Listener#onConfirm} with the result of the
 *     Import Data Dialog, if displayed or true if switching from a managed account.
 *
 * At any dialog, the user can cancel the dialog and end the whole process (resulting in
 * {@link ConfirmImportSyncDataDialog.Listener#onCancel}).
 */
public class ConfirmSyncDataStateMachine
        implements ConfirmImportSyncDataDialog.Listener, ConfirmManagedSyncDataDialog.Listener {
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({BEFORE_OLD_ACCOUNT_DIALOG, BEFORE_NEW_ACCOUNT_DIALOG, AFTER_NEW_ACCOUNT_DIALOG, DONE})
    private @interface State {}
    private static final int BEFORE_OLD_ACCOUNT_DIALOG = 0;  // Start of state B.
    private static final int BEFORE_NEW_ACCOUNT_DIALOG = 1;  // Start of state F.
    private static final int AFTER_NEW_ACCOUNT_DIALOG = 2;   // Start of state H.
    private static final int DONE = 4;

    @State private int mState = BEFORE_OLD_ACCOUNT_DIALOG;

    private static final int ACCOUNT_CHECK_TIMEOUT_MS = 30000;

    private final ConfirmImportSyncDataDialog.Listener mCallback;
    private final @Nullable String mOldAccountName;
    private final String mNewAccountName;
    private final boolean mCurrentlyManaged;
    private final FragmentManager mFragmentManager;
    private final Context mContext;
    private final ImportSyncType mImportSyncType;
    private final ConfirmSyncDataStateMachineDelegate mDelegate;
    private final Handler mHandler = new Handler();

    private boolean mWipeData;
    private Boolean mNewAccountManaged;
    private Runnable mCheckTimeoutRunnable;

    /**
     * Create and run state machine, displaying the appropriate dialogs.
     * @param importSyncType SWITCHING_SYNC_ACCOUNTS if there's already a signed in account,
     *         PREVIOUS_DATA_FOUND otherwise
     * @param oldAccountName if importSyncType is SWITCHING_SYNC_ACCOUNTS: the name of the signed in
     *         account; if importSyncType is PREVIOUS_DATA_FOUND: the name of the last signed in
     *         account or null
     * @param newAccountName the name of the account user is signing in with
     * @param callback the listener to receive the result of this state machine
     */
    public ConfirmSyncDataStateMachine(Context context, FragmentManager fragmentManager,
            ImportSyncType importSyncType, @Nullable String oldAccountName, String newAccountName,
            ConfirmImportSyncDataDialog.Listener callback) {
        ThreadUtils.assertOnUiThread();
        // Includes implicit not-null assertion.
        assert !newAccountName.equals("") : "New account name must be provided.";

        mOldAccountName = oldAccountName;
        mNewAccountName = newAccountName;
        mImportSyncType = importSyncType;
        mFragmentManager = fragmentManager;
        mContext = context;
        mCallback = callback;

        mCurrentlyManaged = SigninManager.get().getManagementDomain() != null;

        mDelegate = new ConfirmSyncDataStateMachineDelegate(mFragmentManager);

        // New account management status isn't needed right now, but fetching it
        // can take a few seconds, so we kick it off early.
        requestNewAccountManagementStatus();

        progress();
    }

    /**
     * Cancels this state machine, dismissing any dialogs being shown.
     * @param isBeingDestroyed whether state machine is being cancelled because enclosing UI object
     *         is being destroyed. This state machine will not invoke callbacks or dismiss dialogs
     *         if isBeingDestroyed is true.
     */
    public void cancel(boolean isBeingDestroyed) {
        ThreadUtils.assertOnUiThread();

        cancelTimeout();
        mState = DONE;

        if (isBeingDestroyed) return;
        mCallback.onCancel();
        mDelegate.dismissAllDialogs();
        dismissDialog(ConfirmImportSyncDataDialog.CONFIRM_IMPORT_SYNC_DATA_DIALOG_TAG);
        dismissDialog(ConfirmManagedSyncDataDialog.CONFIRM_IMPORT_SYNC_DATA_DIALOG_TAG);
    }

    private void dismissDialog(String tag) {
        DialogFragment fragment = (DialogFragment) mFragmentManager.findFragmentByTag(tag);
        if (fragment == null) return;
        fragment.dismissAllowingStateLoss();
    }

    /**
     * This will progress the state machine, by moving the state along and then by either calling
     * itself directly or creating a dialog. If the dialog is dismissed or answered negatively the
     * entire flow is over, if it is answered positively one of the onConfirm functions is called
     * and this function is called again.
     */
    private void progress() {
        switch (mState) {
            case BEFORE_OLD_ACCOUNT_DIALOG:
                mState = BEFORE_NEW_ACCOUNT_DIALOG;

                if (TextUtils.isEmpty(mOldAccountName) || mNewAccountName.equals(mOldAccountName)) {
                    // If there is no old account or the user is just logging back into whatever
                    // they were previously logged in as, progress past the old account checks.
                    progress();
                } else if (mCurrentlyManaged
                        && mImportSyncType == ImportSyncType.SWITCHING_SYNC_ACCOUNTS) {
                    // We only care about the user's previous account being managed if they are
                    // switching accounts.

                    mWipeData = true;

                    // This will call back into onConfirm() on success.
                    ConfirmManagedSyncDataDialog.showSwitchFromManagedAccountDialog(this,
                            mFragmentManager, mContext.getResources(),
                            SigninManager.extractDomainName(mOldAccountName),
                            mOldAccountName, mNewAccountName);
                } else {
                    // This will call back into onConfirm(boolean wipeData) on success.
                    ConfirmImportSyncDataDialog.showNewInstance(mOldAccountName, mNewAccountName,
                            mImportSyncType, mFragmentManager, this);
                }

                break;
            case BEFORE_NEW_ACCOUNT_DIALOG:
                mState = AFTER_NEW_ACCOUNT_DIALOG;
                if (mNewAccountManaged != null) {
                    // No need to show dialog if account management status is already known
                    handleNewAccountManagementStatus();
                } else {
                    showProgressDialog();
                    scheduleTimeout();
                }
                break;
            case AFTER_NEW_ACCOUNT_DIALOG:
                mState = DONE;
                mCallback.onConfirm(mWipeData);
                break;
            case DONE:
                throw new IllegalStateException("Can't progress from DONE state!");
        }
    }

    private void requestNewAccountManagementStatus() {
        SigninManager.isUserManaged(mNewAccountName, this::setIsNewAccountManaged);
    }

    private void setIsNewAccountManaged(Boolean isManaged) {
        assert isManaged != null;
        mNewAccountManaged = isManaged;
        if (mState == AFTER_NEW_ACCOUNT_DIALOG) {
            cancelTimeout();
            handleNewAccountManagementStatus();
        }
    }

    private void handleNewAccountManagementStatus() {
        assert mNewAccountManaged != null;
        assert mState == AFTER_NEW_ACCOUNT_DIALOG;

        mDelegate.dismissAllDialogs();

        if (mNewAccountManaged) {
            // Show 'logging into managed account' dialog
            // This will call back into onConfirm on success.
            ConfirmManagedSyncDataDialog.showSignInToManagedAccountDialog(
                    ConfirmSyncDataStateMachine.this, mFragmentManager, mContext.getResources(),
                    SigninManager.extractDomainName(mNewAccountName));
        } else {
            progress();
        }
    }

    private void showProgressDialog() {
        mDelegate.showFetchManagementPolicyProgressDialog(this::onCancel);
    }

    private void scheduleTimeout() {
        if (mCheckTimeoutRunnable == null) {
            mCheckTimeoutRunnable = this::checkTimeout;
        }
        mHandler.postDelayed(mCheckTimeoutRunnable, ACCOUNT_CHECK_TIMEOUT_MS);
    }

    private void cancelTimeout() {
        if (mCheckTimeoutRunnable == null) {
            return;
        }
        mHandler.removeCallbacks(mCheckTimeoutRunnable);
        mCheckTimeoutRunnable = null;
    }

    private void checkTimeout() {
        assert mState == AFTER_NEW_ACCOUNT_DIALOG;
        assert mNewAccountManaged == null;

        mDelegate.showFetchManagementPolicyTimeoutDialog(
                new ConfirmSyncDataStateMachineDelegate.TimeoutDialogListener() {
                    @Override
                    public void onCancel() {
                        ConfirmSyncDataStateMachine.this.onCancel();
                    }

                    @Override
                    public void onRetry() {
                        requestNewAccountManagementStatus();
                        scheduleTimeout();
                        showProgressDialog();
                    }
                });
    }

    // ConfirmImportSyncDataDialog.Listener implementation.
    @Override
    public void onConfirm(boolean wipeData) {
        mWipeData = wipeData;
        progress();
    }

    // ConfirmManagedSyncDataDialog.Listener implementation.
    @Override
    public void onConfirm() {
        progress();
    }

    // ConfirmImportSyncDataDialog.Listener & ConfirmManagedSyncDataDialog.Listener implementation.
    @Override
    public void onCancel() {
        cancel(false);
    }
}

