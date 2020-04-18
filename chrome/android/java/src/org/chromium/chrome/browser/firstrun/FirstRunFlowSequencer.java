// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.accounts.Account;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.text.TextUtils;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.CommandLine;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.locale.LocaleManager;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.privacy.PrivacyPreferencesManager;
import org.chromium.chrome.browser.services.AndroidEduAndChildAccountHelper;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.vr_shell.VrIntentUtils;
import org.chromium.chrome.browser.webapps.WebApkActivity;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ChildAccountStatus;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.ui.base.DeviceFormFactor;

import java.lang.ref.WeakReference;
import java.util.List;

/**
 * A helper to determine what should be the sequence of First Run Experience screens, and whether
 * it should be run.
 *
 * Usage:
 * new FirstRunFlowSequencer(activity, launcherProvidedProperties) {
 *     override onFlowIsKnown
 * }.start();
 */
public abstract class FirstRunFlowSequencer  {
    private static final int FIRST_RUN_EXPERIENCE_REQUEST_CODE = 101;
    private static final String TAG = "firstrun";

    private final Activity mActivity;

    // The following are initialized via initializeSharedState().
    private boolean mIsAndroidEduDevice;
    private @ChildAccountStatus.Status int mChildAccountStatus;
    private Account[] mGoogleAccounts;
    private boolean mForceEduSignIn;

    /**
     * Callback that is called once the flow is determined.
     * If the properties is null, the First Run experience needs to finish and
     * restart the original intent if necessary.
     * @param freProperties Properties to be used in the First Run activity, or null.
     */
    public abstract void onFlowIsKnown(Bundle freProperties);

    public FirstRunFlowSequencer(Activity activity) {
        mActivity = activity;
    }

    /**
     * Starts determining parameters for the First Run.
     * Once finished, calls onFlowIsKnown().
     */
    public void start() {
        if (CommandLine.getInstance().hasSwitch(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
                || ApiCompatibilityUtils.isDemoUser(mActivity)) {
            onFlowIsKnown(null);
            return;
        }

        new AndroidEduAndChildAccountHelper() {
            @Override
            public void onParametersReady() {
                initializeSharedState(isAndroidEduDevice(), getChildAccountStatus());
                processFreEnvironmentPreNative();
            }
        }.start();
    }

    @VisibleForTesting
    protected boolean isFirstRunFlowComplete() {
        return FirstRunStatus.getFirstRunFlowComplete();
    }

    @VisibleForTesting
    protected boolean isSignedIn() {
        return ChromeSigninController.get().isSignedIn();
    }

    @VisibleForTesting
    protected boolean isSyncAllowed() {
        SigninManager signinManager = SigninManager.get();
        return FeatureUtilities.canAllowSync(mActivity) && !signinManager.isSigninDisabledByPolicy()
                && signinManager.isSigninSupported();
    }

    @VisibleForTesting
    protected Account[] getGoogleAccounts() {
        return AccountManagerFacade.get().tryGetGoogleAccounts();
    }

    @VisibleForTesting
    protected boolean hasAnyUserSeenToS() {
        return ToSAckedReceiver.checkAnyUserHasSeenToS();
    }

    @VisibleForTesting
    protected boolean shouldSkipFirstUseHints() {
        return ApiCompatibilityUtils.shouldSkipFirstUseHints(mActivity.getContentResolver());
    }

    @VisibleForTesting
    protected boolean isFirstRunEulaAccepted() {
        return PrefServiceBridge.getInstance().isFirstRunEulaAccepted();
    }

    protected boolean shouldShowDataReductionPage() {
        return !DataReductionProxySettings.getInstance().isDataReductionProxyManaged()
                && DataReductionProxySettings.getInstance().isDataReductionProxyFREPromoAllowed();
    }

    @VisibleForTesting
    protected boolean shouldShowSearchEnginePage() {
        int searchPromoType = LocaleManager.getInstance().getSearchEnginePromoShowType();
        return searchPromoType == LocaleManager.SEARCH_ENGINE_PROMO_SHOW_NEW
                || searchPromoType == LocaleManager.SEARCH_ENGINE_PROMO_SHOW_EXISTING;
    }

    @VisibleForTesting
    protected void setDefaultMetricsAndCrashReporting() {
        PrivacyPreferencesManager.getInstance().setUsageAndCrashReporting(
                FirstRunActivity.DEFAULT_METRICS_AND_CRASH_REPORTING);
    }

    @VisibleForTesting
    protected void setFirstRunFlowSignInComplete() {
        FirstRunSignInProcessor.setFirstRunFlowSignInComplete(true);
    }

    void initializeSharedState(
            boolean isAndroidEduDevice, @ChildAccountStatus.Status int childAccountStatus) {
        mIsAndroidEduDevice = isAndroidEduDevice;
        mChildAccountStatus = childAccountStatus;
        mGoogleAccounts = getGoogleAccounts();
        // EDU devices should always have exactly 1 google account, which will be automatically
        // signed-in. All FRE screens are skipped in this case.
        mForceEduSignIn = mIsAndroidEduDevice && mGoogleAccounts.length == 1 && !isSignedIn();
    }

    void processFreEnvironmentPreNative() {
        if (isFirstRunFlowComplete()) {
            assert isFirstRunEulaAccepted();
            // We do not need any interactive FRE.
            onFlowIsKnown(null);
            return;
        }

        Bundle freProperties = new Bundle();

        // In the full FRE we always show the Welcome page, except on EDU devices.
        boolean showWelcomePage = !mForceEduSignIn;
        freProperties.putBoolean(FirstRunActivity.SHOW_WELCOME_PAGE, showWelcomePage);
        freProperties.putInt(AccountFirstRunFragment.CHILD_ACCOUNT_STATUS, mChildAccountStatus);

        // Initialize usage and crash reporting according to the default value.
        // The user can explicitly enable or disable the reporting on the Welcome page.
        // This is controlled by the administrator via a policy on EDU devices.
        setDefaultMetricsAndCrashReporting();

        onFlowIsKnown(freProperties);
        if (ChildAccountStatus.isChild(mChildAccountStatus) || mForceEduSignIn) {
            // Child and Edu forced signins are processed independently.
            setFirstRunFlowSignInComplete();
        }
    }

    /**
     * Called onNativeInitialized() a given flow as completed.
     * @param data Resulting FRE properties bundle.
     */
    public void onNativeInitialized(Bundle freProperties) {
        // We show the sign-in page if sync is allowed, and not signed in, and this is not
        // an EDU device, and
        // - no "skip the first use hints" is set, or
        // - "skip the first use hints" is set, but there is at least one account.
        boolean offerSignInOk = isSyncAllowed() && !isSignedIn() && !mForceEduSignIn
                && (!shouldSkipFirstUseHints() || mGoogleAccounts.length > 0);
        freProperties.putBoolean(FirstRunActivity.SHOW_SIGNIN_PAGE, offerSignInOk);
        if (mForceEduSignIn || ChildAccountStatus.isChild(mChildAccountStatus)) {
            // If the device is an Android EDU device or has a child account, there should be
            // exactly account on the device. Force sign-in in to that account.
            freProperties.putString(
                    AccountFirstRunFragment.FORCE_SIGNIN_ACCOUNT_TO, mGoogleAccounts[0].name);
        }

        freProperties.putBoolean(
                FirstRunActivity.SHOW_DATA_REDUCTION_PAGE, shouldShowDataReductionPage());
        freProperties.putBoolean(
                FirstRunActivity.SHOW_SEARCH_ENGINE_PAGE, shouldShowSearchEnginePage());
    }

    /**
     * Marks a given flow as completed.
     * @param signInAccountName The account name for the pending sign-in request. (Or null)
     * @param showSignInSettings Whether the user selected to see the settings once signed in.
     */
    public static void markFlowAsCompleted(String signInAccountName, boolean showSignInSettings) {
        // When the user accepts ToS in the Setup Wizard (see ToSAckedReceiver), we do not
        // show the ToS page to the user because the user has already accepted one outside FRE.
        if (!PrefServiceBridge.getInstance().isFirstRunEulaAccepted()) {
            PrefServiceBridge.getInstance().setEulaAccepted();
        }

        // Mark the FRE flow as complete and set the sign-in flow preferences if necessary.
        FirstRunSignInProcessor.finalizeFirstRunFlowState(signInAccountName, showSignInSettings);
    }

    /**
     * Checks if the First Run needs to be launched.
     * @param context The context.
     * @param fromIntent The intent that was used to launch Chrome.
     * @param preferLightweightFre Whether to prefer the Lightweight First Run Experience.
     * @return Whether the First Run Experience needs to be launched.
     */
    public static boolean checkIfFirstRunIsNecessary(
            Context context, Intent fromIntent, boolean preferLightweightFre) {
        // If FRE is disabled (e.g. in tests), proceed directly to the intent handling.
        if (CommandLine.getInstance().hasSwitch(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
                || ApiCompatibilityUtils.isDemoUser(context)) {
            return false;
        }

        // If Chrome isn't opened via the Chrome icon, and the user accepted the ToS
        // in the Setup Wizard, skip any First Run Experience screens and proceed directly
        // to the intent handling.
        final boolean fromChromeIcon =
                fromIntent != null && TextUtils.equals(fromIntent.getAction(), Intent.ACTION_MAIN);
        if (!fromChromeIcon && ToSAckedReceiver.checkAnyUserHasSeenToS()) return false;

        if (FirstRunStatus.getFirstRunFlowComplete()) {
            // Promo pages are removed, so there is nothing else to show in FRE.
            return false;
        }
        return !preferLightweightFre
                || (!FirstRunStatus.shouldSkipWelcomePage()
                           && !FirstRunStatus.getLightweightFirstRunFlowComplete());
    }

    /**
     * @return A generic intent to show the First Run Activity.
     * @param context        The context.
     * @param fromChromeIcon Whether Chrome is opened via the Chrome icon.
     */
    public static Intent createGenericFirstRunIntent(Context context, boolean fromChromeIcon) {
        Intent intent = new Intent();
        intent.setClassName(context, FirstRunActivity.class.getName());
        intent.putExtra(FirstRunActivity.EXTRA_COMING_FROM_CHROME_ICON, fromChromeIcon);
        return intent;
    }

    /**
     * Returns an intent to show the lightweight first run activity.
     * @param context        The context.
     * @param fromIntent     The intent that was used to launch Chrome.
     */
    private static Intent createLightweightFirstRunIntent(Context context, Intent fromIntent) {
        Intent intent = new Intent();
        intent.setClassName(context, LightweightFirstRunActivity.class.getName());
        String appName = WebApkActivity.slowExtractNameFromIntentIfTargetIsWebApk(fromIntent);
        intent.putExtra(LightweightFirstRunActivity.EXTRA_ASSOCIATED_APP_NAME, appName);
        return intent;
    }

    /**
     * Adds fromIntent as a PendingIntent to the firstRunIntent. This should be used to add a
     * PendingIntent that will be sent when first run is either completed or canceled.
     *
     * @param caller            The context that corresponds to the Intent.
     * @param firstRunIntent    The intent that will be used to start first run.
     * @param fromIntent        The intent that was used to launch Chrome.
     * @param requiresBroadcast Whether or not the fromIntent must be broadcasted.
     */
    private static void addPendingIntent(
            Context caller, Intent firstRunIntent, Intent fromIntent, boolean requiresBroadcast) {
        PendingIntent pendingIntent = null;
        int pendingIntentFlags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_ONE_SHOT;
        if (requiresBroadcast) {
            pendingIntent = PendingIntent.getBroadcast(
                    caller, FIRST_RUN_EXPERIENCE_REQUEST_CODE, fromIntent, pendingIntentFlags);
        } else {
            pendingIntent = PendingIntent.getActivity(
                    caller, FIRST_RUN_EXPERIENCE_REQUEST_CODE, fromIntent, pendingIntentFlags);
        }
        firstRunIntent.putExtra(FirstRunActivity.EXTRA_CHROME_LAUNCH_INTENT, pendingIntent);
        firstRunIntent.putExtra(FirstRunActivity.EXTRA_CHROME_LAUNCH_INTENT_IS_CCT,
                LaunchIntentDispatcher.isCustomTabIntent(fromIntent));
    }

    /**
     * Tries to launch the First Run Experience.  If the Activity was launched with the wrong Intent
     * flags, we first relaunch it to make sure it runs in its own task, then trigger First Run.
     *
     * @param caller               Activity instance that is checking if first run is necessary.
     * @param intent               Intent used to launch the caller.
     * @param requiresBroadcast    Whether or not the Intent triggers a BroadcastReceiver.
     * @param preferLightweightFre Whether to prefer the Lightweight First Run Experience.
     * @return Whether startup must be blocked (e.g. via Activity#finish or dropping the Intent).
     */
    public static boolean launch(Context caller, Intent intent, boolean requiresBroadcast,
            boolean preferLightweightFre) {
        // Check if the user just came back from the FRE.
        boolean firstRunActivityResult = IntentUtils.safeGetBooleanExtra(
                intent, FirstRunActivity.EXTRA_FIRST_RUN_ACTIVITY_RESULT, false);
        boolean firstRunComplete = IntentUtils.safeGetBooleanExtra(
                intent, FirstRunActivity.EXTRA_FIRST_RUN_COMPLETE, false);
        if (firstRunActivityResult && !firstRunComplete) {
            Log.d(TAG, "User failed to complete the FRE.  Aborting");
            return true;
        }

        // Check if the user needs to go through First Run at all.
        if (!checkIfFirstRunIsNecessary(caller, intent, preferLightweightFre)) return false;

        Log.d(TAG, "Redirecting user through FRE.");
        if ((intent.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK) != 0) {
            boolean isVrIntent = VrIntentUtils.isVrIntent(intent);
            boolean isGenericFreActive = false;
            List<WeakReference<Activity>> activities = ApplicationStatus.getRunningActivities();
            for (WeakReference<Activity> weakActivity : activities) {
                Activity activity = weakActivity.get();
                if (activity instanceof FirstRunActivity) {
                    isGenericFreActive = true;
                    break;
                }
            }

            // Launch the Generic First Run Experience if it was previously active.
            Intent freIntent = null;
            if (preferLightweightFre && !isGenericFreActive) {
                freIntent = createLightweightFirstRunIntent(caller, intent);
            } else {
                freIntent = createGenericFirstRunIntent(
                        caller, TextUtils.equals(intent.getAction(), Intent.ACTION_MAIN));

                if (maybeSwitchToTabbedMode(caller, freIntent)) {
                    // We switched to TabbedModeFRE. We need to disable animation on the original
                    // intent, to make transition seamless.
                    intent = new Intent(intent);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
                }
            }

            // Add a PendingIntent so that the intent used to launch Chrome will be resent when
            // First Run is completed or canceled.
            addPendingIntent(caller, freIntent, intent, requiresBroadcast);

            if (!(caller instanceof Activity)) freIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            if (isVrIntent) {
                freIntent = VrIntentUtils.setupVrFreIntent(caller, intent, freIntent);
            }
            IntentUtils.safeStartActivity(caller, freIntent);
        } else {
            // First Run requires that the Intent contains NEW_TASK so that it doesn't sit on top
            // of something else.
            Intent newIntent = new Intent(intent);
            newIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            IntentUtils.safeStartActivity(caller, newIntent);
        }
        return true;
    }

    /**
     * On tablets, where FRE activity is a dialog, transitions from fillscreen activities
     * (the ones that use TabbedModeTheme, e.g. ChromeTabbedActivity) look ugly, because
     * when FRE is started from CTA.onCreate(), currently running animation for CTA window
     * is aborted. This is perceived as a flash of white and doesn't look good.
     *
     * To solve this, we added TabbedMode FRE activity, which has the same window background
     * as TabbedModeTheme activities, but shows content in a FRE-like dialog.
     *
     * This function attempts to switch FRE to TabbedModeFRE if certain conditions are met.
     */
    private static boolean maybeSwitchToTabbedMode(Context caller, Intent freIntent) {
        // Caller must be an activity.
        if (!(caller instanceof Activity)) return false;

        // We must be on a tablet (where FRE is a dialog).
        if (!DeviceFormFactor.isNonMultiDisplayContextOnTablet(caller)) return false;

        // Caller must use a theme with @drawable/window_background (the same background
        // used by TabbedModeFRE).
        TypedArray a = caller.obtainStyledAttributes(new int[] {android.R.attr.windowBackground});
        int backgroundResourceId = a.getResourceId(0 /* index */, 0);
        a.recycle();
        if (backgroundResourceId != R.drawable.window_background) return false;

        // Switch FRE -> TabbedModeFRE.
        freIntent.setClass(caller, TabbedModeFirstRunActivity.class);
        return true;
    }
}
