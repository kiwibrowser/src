/*
 * Copyright (C) 2006, 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.uicc;

import android.app.AlertDialog;
import android.app.usage.UsageStatsManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.content.res.Resources;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.RegistrantList;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.Rlog;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.LocalLog;
import android.view.WindowManager;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.CommandsInterface.RadioState;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.IccCardStatus.CardState;
import com.android.internal.telephony.uicc.IccCardStatus.PinState;
import com.android.internal.telephony.cat.CatService;

import com.android.internal.R;

import java.io.FileDescriptor;
import java.io.PrintWriter;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

/**
 * {@hide}
 */
public class UiccCard {
    protected static final String LOG_TAG = "UiccCard";
    protected static final boolean DBG = true;

    public static final String EXTRA_ICC_CARD_ADDED =
            "com.android.internal.telephony.uicc.ICC_CARD_ADDED";

    private static final String OPERATOR_BRAND_OVERRIDE_PREFIX = "operator_branding_";

    private final Object mLock = new Object();
    private CardState mCardState;
    private PinState mUniversalPinState;
    private int mGsmUmtsSubscriptionAppIndex;
    private int mCdmaSubscriptionAppIndex;
    private int mImsSubscriptionAppIndex;
    private UiccCardApplication[] mUiccApplications =
            new UiccCardApplication[IccCardStatus.CARD_MAX_APPS];
    private Context mContext;
    private CommandsInterface mCi;
    private CatService mCatService;
    private RadioState mLastRadioState =  RadioState.RADIO_UNAVAILABLE;
    private UiccCarrierPrivilegeRules mCarrierPrivilegeRules;

    private RegistrantList mAbsentRegistrants = new RegistrantList();
    private RegistrantList mCarrierPrivilegeRegistrants = new RegistrantList();

    private static final int EVENT_CARD_REMOVED = 13;
    private static final int EVENT_CARD_ADDED = 14;
    private static final int EVENT_OPEN_LOGICAL_CHANNEL_DONE = 15;
    private static final int EVENT_CLOSE_LOGICAL_CHANNEL_DONE = 16;
    private static final int EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE = 17;
    private static final int EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE = 18;
    private static final int EVENT_SIM_IO_DONE = 19;
    private static final int EVENT_CARRIER_PRIVILIGES_LOADED = 20;

    private static final LocalLog mLocalLog = new LocalLog(100);

    private int mPhoneId;

    public UiccCard(Context c, CommandsInterface ci, IccCardStatus ics) {
        if (DBG) log("Creating");
        mCardState = ics.mCardState;
        update(c, ci, ics);
    }

    public UiccCard(Context c, CommandsInterface ci, IccCardStatus ics, int phoneId) {
        mCardState = ics.mCardState;
        mPhoneId = phoneId;
        update(c, ci, ics);
    }

    protected UiccCard() {
    }

    public void dispose() {
        synchronized (mLock) {
            if (DBG) log("Disposing card");
            if (mCatService != null) mCatService.dispose();
            for (UiccCardApplication app : mUiccApplications) {
                if (app != null) {
                    app.dispose();
                }
            }
            mCatService = null;
            mUiccApplications = null;
            mCarrierPrivilegeRules = null;
        }
    }

    public void update(Context c, CommandsInterface ci, IccCardStatus ics) {
        synchronized (mLock) {
            CardState oldState = mCardState;
            mCardState = ics.mCardState;
            mUniversalPinState = ics.mUniversalPinState;
            mGsmUmtsSubscriptionAppIndex = ics.mGsmUmtsSubscriptionAppIndex;
            mCdmaSubscriptionAppIndex = ics.mCdmaSubscriptionAppIndex;
            mImsSubscriptionAppIndex = ics.mImsSubscriptionAppIndex;
            mContext = c;
            mCi = ci;

            //update applications
            if (DBG) log(ics.mApplications.length + " applications");
            for ( int i = 0; i < mUiccApplications.length; i++) {
                if (mUiccApplications[i] == null) {
                    //Create newly added Applications
                    if (i < ics.mApplications.length) {
                        mUiccApplications[i] = new UiccCardApplication(this,
                                ics.mApplications[i], mContext, mCi);
                    }
                } else if (i >= ics.mApplications.length) {
                    //Delete removed applications
                    mUiccApplications[i].dispose();
                    mUiccApplications[i] = null;
                } else {
                    //Update the rest
                    mUiccApplications[i].update(ics.mApplications[i], mContext, mCi);
                }
            }

            createAndUpdateCatService();

            // Reload the carrier privilege rules if necessary.
            log("Before privilege rules: " + mCarrierPrivilegeRules + " : " + mCardState);
            if (mCarrierPrivilegeRules == null && mCardState == CardState.CARDSTATE_PRESENT) {
                mCarrierPrivilegeRules = new UiccCarrierPrivilegeRules(this,
                        mHandler.obtainMessage(EVENT_CARRIER_PRIVILIGES_LOADED));
            } else if (mCarrierPrivilegeRules != null && mCardState != CardState.CARDSTATE_PRESENT) {
                mCarrierPrivilegeRules = null;
            }

            sanitizeApplicationIndexes();

            RadioState radioState = mCi.getRadioState();
            if (DBG) log("update: radioState=" + radioState + " mLastRadioState="
                    + mLastRadioState);
            // No notifications while radio is off or we just powering up
            if (radioState == RadioState.RADIO_ON && mLastRadioState == RadioState.RADIO_ON) {
                if (oldState != CardState.CARDSTATE_ABSENT &&
                        mCardState == CardState.CARDSTATE_ABSENT) {
                    if (DBG) log("update: notify card removed");
                    mAbsentRegistrants.notifyRegistrants();
                    mHandler.sendMessage(mHandler.obtainMessage(EVENT_CARD_REMOVED, null));
                } else if (oldState == CardState.CARDSTATE_ABSENT &&
                        mCardState != CardState.CARDSTATE_ABSENT) {
                    if (DBG) log("update: notify card added");
                    mHandler.sendMessage(mHandler.obtainMessage(EVENT_CARD_ADDED, null));
                }
            }
            mLastRadioState = radioState;
        }
    }

    protected void createAndUpdateCatService() {
        if (mUiccApplications.length > 0 && mUiccApplications[0] != null) {
            // Initialize or Reinitialize CatService
            if (mCatService == null) {
                mCatService = CatService.getInstance(mCi, mContext, this, mPhoneId);
            } else {
                ((CatService)mCatService).update(mCi, mContext, this);
            }
        } else {
            if (mCatService != null) {
                mCatService.dispose();
            }
            mCatService = null;
        }
    }

    public CatService getCatService() {
        return mCatService;
    }

    @Override
    protected void finalize() {
        if (DBG) log("UiccCard finalized");
    }

    /**
     * This function makes sure that application indexes are valid
     * and resets invalid indexes. (This should never happen, but in case
     * RIL misbehaves we need to manage situation gracefully)
     */
    private void sanitizeApplicationIndexes() {
        mGsmUmtsSubscriptionAppIndex =
                checkIndex(mGsmUmtsSubscriptionAppIndex, AppType.APPTYPE_SIM, AppType.APPTYPE_USIM);
        mCdmaSubscriptionAppIndex =
                checkIndex(mCdmaSubscriptionAppIndex, AppType.APPTYPE_RUIM, AppType.APPTYPE_CSIM);
        mImsSubscriptionAppIndex =
                checkIndex(mImsSubscriptionAppIndex, AppType.APPTYPE_ISIM, null);
    }

    private int checkIndex(int index, AppType expectedAppType, AppType altExpectedAppType) {
        if (mUiccApplications == null || index >= mUiccApplications.length) {
            loge("App index " + index + " is invalid since there are no applications");
            return -1;
        }

        if (index < 0) {
            // This is normal. (i.e. no application of this type)
            return -1;
        }

        if (mUiccApplications[index].getType() != expectedAppType &&
            mUiccApplications[index].getType() != altExpectedAppType) {
            loge("App index " + index + " is invalid since it's not " +
                    expectedAppType + " and not " + altExpectedAppType);
            return -1;
        }

        // Seems to be valid
        return index;
    }

    /**
     * Notifies handler of any transition into State.ABSENT
     */
    public void registerForAbsent(Handler h, int what, Object obj) {
        synchronized (mLock) {
            Registrant r = new Registrant (h, what, obj);

            mAbsentRegistrants.add(r);

            if (mCardState == CardState.CARDSTATE_ABSENT) {
                r.notifyRegistrant();
            }
        }
    }

    public void unregisterForAbsent(Handler h) {
        synchronized (mLock) {
            mAbsentRegistrants.remove(h);
        }
    }

    /**
     * Notifies handler when carrier privilege rules are loaded.
     */
    public void registerForCarrierPrivilegeRulesLoaded(Handler h, int what, Object obj) {
        synchronized (mLock) {
            Registrant r = new Registrant (h, what, obj);

            mCarrierPrivilegeRegistrants.add(r);

            if (areCarrierPriviligeRulesLoaded()) {
                r.notifyRegistrant();
            }
        }
    }

    public void unregisterForCarrierPrivilegeRulesLoaded(Handler h) {
        synchronized (mLock) {
            mCarrierPrivilegeRegistrants.remove(h);
        }
    }

    private void onIccSwap(boolean isAdded) {

        boolean isHotSwapSupported = mContext.getResources().getBoolean(
                R.bool.config_hotswapCapable);

        if (isHotSwapSupported) {
            log("onIccSwap: isHotSwapSupported is true, don't prompt for rebooting");
            return;
        }
        log("onIccSwap: isHotSwapSupported is false, prompt for rebooting");

        promptForRestart(isAdded);
    }

    private void promptForRestart(boolean isAdded) {
        synchronized (mLock) {
            final Resources res = mContext.getResources();
            final String dialogComponent = res.getString(
                    R.string.config_iccHotswapPromptForRestartDialogComponent);
            if (dialogComponent != null) {
                Intent intent = new Intent().setComponent(ComponentName.unflattenFromString(
                        dialogComponent)).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        .putExtra(EXTRA_ICC_CARD_ADDED, isAdded);
                try {
                    mContext.startActivity(intent);
                    return;
                } catch (ActivityNotFoundException e) {
                    loge("Unable to find ICC hotswap prompt for restart activity: " + e);
                }
            }

            // TODO: Here we assume the device can't handle SIM hot-swap
            //      and has to reboot. We may want to add a property,
            //      e.g. REBOOT_ON_SIM_SWAP, to indicate if modem support
            //      hot-swap.
            DialogInterface.OnClickListener listener = null;


            // TODO: SimRecords is not reset while SIM ABSENT (only reset while
            //       Radio_off_or_not_available). Have to reset in both both
            //       added or removed situation.
            listener = new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    synchronized (mLock) {
                        if (which == DialogInterface.BUTTON_POSITIVE) {
                            if (DBG) log("Reboot due to SIM swap");
                            PowerManager pm = (PowerManager) mContext
                                    .getSystemService(Context.POWER_SERVICE);
                            pm.reboot("SIM is added.");
                        }
                    }
                }

            };

            Resources r = Resources.getSystem();

            String title = (isAdded) ? r.getString(R.string.sim_added_title) :
                r.getString(R.string.sim_removed_title);
            String message = (isAdded) ? r.getString(R.string.sim_added_message) :
                r.getString(R.string.sim_removed_message);
            String buttonTxt = r.getString(R.string.sim_restart_button);

            AlertDialog dialog = new AlertDialog.Builder(mContext)
            .setTitle(title)
            .setMessage(message)
            .setPositiveButton(buttonTxt, listener)
            .create();
            dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            dialog.show();
        }
    }

    protected Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg){
            switch (msg.what) {
                case EVENT_CARD_REMOVED:
                    onIccSwap(false);
                    break;
                case EVENT_CARD_ADDED:
                    onIccSwap(true);
                    break;
                case EVENT_OPEN_LOGICAL_CHANNEL_DONE:
                case EVENT_CLOSE_LOGICAL_CHANNEL_DONE:
                case EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE:
                case EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE:
                case EVENT_SIM_IO_DONE:
                    AsyncResult ar = (AsyncResult)msg.obj;
                    if (ar.exception != null) {
                        loglocal("Exception: " + ar.exception);
                        log("Error in SIM access with exception" + ar.exception);
                    }
                    AsyncResult.forMessage((Message)ar.userObj, ar.result, ar.exception);
                    ((Message)ar.userObj).sendToTarget();
                    break;
                case EVENT_CARRIER_PRIVILIGES_LOADED:
                    onCarrierPriviligesLoadedMessage();
                    break;
                default:
                    loge("Unknown Event " + msg.what);
            }
        }
    };

    private boolean isPackageInstalled(String pkgName) {
        PackageManager pm = mContext.getPackageManager();
        try {
            pm.getPackageInfo(pkgName, PackageManager.GET_ACTIVITIES);
            if (DBG) log(pkgName + " is installed.");
            return true;
        } catch (PackageManager.NameNotFoundException e) {
            if (DBG) log(pkgName + " is not installed.");
            return false;
        }
    }

    private class ClickListener implements DialogInterface.OnClickListener {
        String pkgName;
        public ClickListener(String pkgName) {
            this.pkgName = pkgName;
        }
        @Override
        public void onClick(DialogInterface dialog, int which) {
            synchronized (mLock) {
                if (which == DialogInterface.BUTTON_POSITIVE) {
                    Intent market = new Intent(Intent.ACTION_VIEW);
                    market.setData(Uri.parse("market://details?id=" + pkgName));
                    market.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    mContext.startActivity(market);
                } else if (which == DialogInterface.BUTTON_NEGATIVE) {
                    if (DBG) log("Not now clicked for carrier app dialog.");
                }
            }
        }
    }

    private void promptInstallCarrierApp(String pkgName) {
        DialogInterface.OnClickListener listener = new ClickListener(pkgName);

        Resources r = Resources.getSystem();
        String message = r.getString(R.string.carrier_app_dialog_message);
        String buttonTxt = r.getString(R.string.carrier_app_dialog_button);
        String notNowTxt = r.getString(R.string.carrier_app_dialog_not_now);

        AlertDialog dialog = new AlertDialog.Builder(mContext)
        .setMessage(message)
        .setNegativeButton(notNowTxt, listener)
        .setPositiveButton(buttonTxt, listener)
        .create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.show();
    }

    private void onCarrierPriviligesLoadedMessage() {
        UsageStatsManager usm = (UsageStatsManager) mContext.getSystemService(
                Context.USAGE_STATS_SERVICE);
        if (usm != null) {
            usm.onCarrierPrivilegedAppsChanged();
        }
        synchronized (mLock) {
            mCarrierPrivilegeRegistrants.notifyRegistrants();
            String whitelistSetting = Settings.Global.getString(mContext.getContentResolver(),
                    Settings.Global.CARRIER_APP_WHITELIST);
            if (TextUtils.isEmpty(whitelistSetting)) {
                return;
            }
            HashSet<String> carrierAppSet = new HashSet<String>(
                    Arrays.asList(whitelistSetting.split("\\s*;\\s*")));
            if (carrierAppSet.isEmpty()) {
                return;
            }

            List<String> pkgNames = mCarrierPrivilegeRules.getPackageNames();
            for (String pkgName : pkgNames) {
                if (!TextUtils.isEmpty(pkgName) && carrierAppSet.contains(pkgName)
                        && !isPackageInstalled(pkgName)) {
                    promptInstallCarrierApp(pkgName);
                }
            }
        }
    }

    public boolean isApplicationOnIcc(IccCardApplicationStatus.AppType type) {
        synchronized (mLock) {
            for (int i = 0 ; i < mUiccApplications.length; i++) {
                if (mUiccApplications[i] != null && mUiccApplications[i].getType() == type) {
                    return true;
                }
            }
            return false;
        }
    }

    public CardState getCardState() {
        synchronized (mLock) {
            return mCardState;
        }
    }

    public PinState getUniversalPinState() {
        synchronized (mLock) {
            return mUniversalPinState;
        }
    }

    public UiccCardApplication getApplication(int family) {
        synchronized (mLock) {
            int index = IccCardStatus.CARD_MAX_APPS;
            switch (family) {
                case UiccController.APP_FAM_3GPP:
                    index = mGsmUmtsSubscriptionAppIndex;
                    break;
                case UiccController.APP_FAM_3GPP2:
                    index = mCdmaSubscriptionAppIndex;
                    break;
                case UiccController.APP_FAM_IMS:
                    index = mImsSubscriptionAppIndex;
                    break;
            }
            if (index >= 0 && index < mUiccApplications.length) {
                return mUiccApplications[index];
            }
            return null;
        }
    }

    public UiccCardApplication getApplicationIndex(int index) {
        synchronized (mLock) {
            if (index >= 0 && index < mUiccApplications.length) {
                return mUiccApplications[index];
            }
            return null;
        }
    }

    /**
     * Returns the SIM application of the specified type.
     *
     * @param type ICC application type (@see com.android.internal.telephony.PhoneConstants#APPTYPE_xxx)
     * @return application corresponding to type or a null if no match found
     */
    public UiccCardApplication getApplicationByType(int type) {
        synchronized (mLock) {
            for (int i = 0 ; i < mUiccApplications.length; i++) {
                if (mUiccApplications[i] != null &&
                        mUiccApplications[i].getType().ordinal() == type) {
                    return mUiccApplications[i];
                }
            }
            return null;
        }
    }

    /**
     * Resets the application with the input AID. Returns true if any changes were made.
     *
     * A null aid implies a card level reset - all applications must be reset.
     */
    public boolean resetAppWithAid(String aid) {
        synchronized (mLock) {
            boolean changed = false;
            for (int i = 0; i < mUiccApplications.length; i++) {
                if (mUiccApplications[i] != null &&
                    (aid == null || aid.equals(mUiccApplications[i].getAid()))) {
                    // Delete removed applications
                    mUiccApplications[i].dispose();
                    mUiccApplications[i] = null;
                    changed = true;
                }
            }
            return changed;
        }
        // TODO: For a card level notification, we should delete the CarrierPrivilegeRules and the
        // CAT service.
    }

    /**
     * Exposes {@link CommandsInterface.iccOpenLogicalChannel}
     */
    public void iccOpenLogicalChannel(String AID, Message response) {
        loglocal("Open Logical Channel: " + AID + " by pid:" + Binder.getCallingPid()
                + " uid:" + Binder.getCallingUid());
        mCi.iccOpenLogicalChannel(AID,
                mHandler.obtainMessage(EVENT_OPEN_LOGICAL_CHANNEL_DONE, response));
    }

    /**
     * Exposes {@link CommandsInterface.iccCloseLogicalChannel}
     */
    public void iccCloseLogicalChannel(int channel, Message response) {
        loglocal("Close Logical Channel: " + channel);
        mCi.iccCloseLogicalChannel(channel,
                mHandler.obtainMessage(EVENT_CLOSE_LOGICAL_CHANNEL_DONE, response));
    }

    /**
     * Exposes {@link CommandsInterface.iccTransmitApduLogicalChannel}
     */
    public void iccTransmitApduLogicalChannel(int channel, int cla, int command,
            int p1, int p2, int p3, String data, Message response) {
        mCi.iccTransmitApduLogicalChannel(channel, cla, command, p1, p2, p3,
                data, mHandler.obtainMessage(EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE, response));
    }

    /**
     * Exposes {@link CommandsInterface.iccTransmitApduBasicChannel}
     */
    public void iccTransmitApduBasicChannel(int cla, int command,
            int p1, int p2, int p3, String data, Message response) {
        mCi.iccTransmitApduBasicChannel(cla, command, p1, p2, p3,
                data, mHandler.obtainMessage(EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE, response));
    }

    /**
     * Exposes {@link CommandsInterface.iccIO}
     */
    public void iccExchangeSimIO(int fileID, int command, int p1, int p2, int p3,
            String pathID, Message response) {
        mCi.iccIO(command, fileID, pathID, p1, p2, p3, null, null,
                mHandler.obtainMessage(EVENT_SIM_IO_DONE, response));
    }

    /**
     * Exposes {@link CommandsInterface.sendEnvelopeWithStatus}
     */
    public void sendEnvelopeWithStatus(String contents, Message response) {
        mCi.sendEnvelopeWithStatus(contents, response);
    }

    /* Returns number of applications on this card */
    public int getNumApplications() {
        int count = 0;
        for (UiccCardApplication a : mUiccApplications) {
            if (a != null) {
                count++;
            }
        }
        return count;
    }

    public int getPhoneId() {
        return mPhoneId;
    }

    /**
     * Returns true iff carrier privileges rules are null (dont need to be loaded) or loaded.
     */
    public boolean areCarrierPriviligeRulesLoaded() {
        return mCarrierPrivilegeRules == null
            || mCarrierPrivilegeRules.areCarrierPriviligeRulesLoaded();
    }

    /**
     * Returns true if there are some carrier privilege rules loaded and specified.
     */
    public boolean hasCarrierPrivilegeRules() {
        return mCarrierPrivilegeRules != null
                && mCarrierPrivilegeRules.hasCarrierPrivilegeRules();
    }

    /**
     * Exposes {@link UiccCarrierPrivilegeRules.getCarrierPrivilegeStatus}.
     */
    public int getCarrierPrivilegeStatus(Signature signature, String packageName) {
        return mCarrierPrivilegeRules == null ?
            TelephonyManager.CARRIER_PRIVILEGE_STATUS_RULES_NOT_LOADED :
            mCarrierPrivilegeRules.getCarrierPrivilegeStatus(signature, packageName);
    }

    /**
     * Exposes {@link UiccCarrierPrivilegeRules.getCarrierPrivilegeStatus}.
     */
    public int getCarrierPrivilegeStatus(PackageManager packageManager, String packageName) {
        return mCarrierPrivilegeRules == null ?
            TelephonyManager.CARRIER_PRIVILEGE_STATUS_RULES_NOT_LOADED :
            mCarrierPrivilegeRules.getCarrierPrivilegeStatus(packageManager, packageName);
    }

    /**
     * Exposes {@link UiccCarrierPrivilegeRules.getCarrierPrivilegeStatus}.
     */
    public int getCarrierPrivilegeStatus(PackageInfo packageInfo) {
        return mCarrierPrivilegeRules == null ?
            TelephonyManager.CARRIER_PRIVILEGE_STATUS_RULES_NOT_LOADED :
            mCarrierPrivilegeRules.getCarrierPrivilegeStatus(packageInfo);
    }

    /**
     * Exposes {@link UiccCarrierPrivilegeRules.getCarrierPrivilegeStatusForCurrentTransaction}.
     */
    public int getCarrierPrivilegeStatusForCurrentTransaction(PackageManager packageManager) {
        return mCarrierPrivilegeRules == null ?
            TelephonyManager.CARRIER_PRIVILEGE_STATUS_RULES_NOT_LOADED :
            mCarrierPrivilegeRules.getCarrierPrivilegeStatusForCurrentTransaction(packageManager);
    }

    /**
     * Exposes {@link UiccCarrierPrivilegeRules.getCarrierPackageNamesForIntent}.
     */
    public List<String> getCarrierPackageNamesForIntent(
            PackageManager packageManager, Intent intent) {
        return mCarrierPrivilegeRules == null ? null :
            mCarrierPrivilegeRules.getCarrierPackageNamesForIntent(
                    packageManager, intent);
    }

    public boolean setOperatorBrandOverride(String brand) {
        log("setOperatorBrandOverride: " + brand);
        log("current iccId: " + getIccId());

        String iccId = getIccId();
        if (TextUtils.isEmpty(iccId)) {
            return false;
        }

        SharedPreferences.Editor spEditor =
                PreferenceManager.getDefaultSharedPreferences(mContext).edit();
        String key = OPERATOR_BRAND_OVERRIDE_PREFIX + iccId;
        if (brand == null) {
            spEditor.remove(key).commit();
        } else {
            spEditor.putString(key, brand).commit();
        }
        return true;
    }

    public String getOperatorBrandOverride() {
        String iccId = getIccId();
        if (TextUtils.isEmpty(iccId)) {
            return null;
        }
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
        return sp.getString(OPERATOR_BRAND_OVERRIDE_PREFIX + iccId, null);
    }

    public String getIccId() {
        // ICCID should be same across all the apps.
        for (UiccCardApplication app : mUiccApplications) {
            if (app != null) {
                IccRecords ir = app.getIccRecords();
                if (ir != null && ir.getIccId() != null) {
                    return ir.getIccId();
                }
            }
        }
        return null;
    }

    private void log(String msg) {
        Rlog.d(LOG_TAG, msg);
    }

    private void loge(String msg) {
        Rlog.e(LOG_TAG, msg);
    }

    private void loglocal(String msg) {
        if (DBG) mLocalLog.log(msg);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("UiccCard:");
        pw.println(" mCi=" + mCi);
        pw.println(" mLastRadioState=" + mLastRadioState);
        pw.println(" mCatService=" + mCatService);
        pw.println(" mAbsentRegistrants: size=" + mAbsentRegistrants.size());
        for (int i = 0; i < mAbsentRegistrants.size(); i++) {
            pw.println("  mAbsentRegistrants[" + i + "]="
                    + ((Registrant)mAbsentRegistrants.get(i)).getHandler());
        }
        for (int i = 0; i < mCarrierPrivilegeRegistrants.size(); i++) {
            pw.println("  mCarrierPrivilegeRegistrants[" + i + "]="
                    + ((Registrant)mCarrierPrivilegeRegistrants.get(i)).getHandler());
        }
        pw.println(" mCardState=" + mCardState);
        pw.println(" mUniversalPinState=" + mUniversalPinState);
        pw.println(" mGsmUmtsSubscriptionAppIndex=" + mGsmUmtsSubscriptionAppIndex);
        pw.println(" mCdmaSubscriptionAppIndex=" + mCdmaSubscriptionAppIndex);
        pw.println(" mImsSubscriptionAppIndex=" + mImsSubscriptionAppIndex);
        pw.println(" mImsSubscriptionAppIndex=" + mImsSubscriptionAppIndex);
        pw.println(" mUiccApplications: length=" + mUiccApplications.length);
        for (int i = 0; i < mUiccApplications.length; i++) {
            if (mUiccApplications[i] == null) {
                pw.println("  mUiccApplications[" + i + "]=" + null);
            } else {
                pw.println("  mUiccApplications[" + i + "]="
                        + mUiccApplications[i].getType() + " " + mUiccApplications[i]);
            }
        }
        pw.println();
        // Print details of all applications
        for (UiccCardApplication app : mUiccApplications) {
            if (app != null) {
                app.dump(fd, pw, args);
                pw.println();
            }
        }
        // Print details of all IccRecords
        for (UiccCardApplication app : mUiccApplications) {
            if (app != null) {
                IccRecords ir = app.getIccRecords();
                if (ir != null) {
                    ir.dump(fd, pw, args);
                    pw.println();
                }
            }
        }
        // Print UiccCarrierPrivilegeRules and registrants.
        if (mCarrierPrivilegeRules == null) {
            pw.println(" mCarrierPrivilegeRules: null");
        } else {
            pw.println(" mCarrierPrivilegeRules: " + mCarrierPrivilegeRules);
            mCarrierPrivilegeRules.dump(fd, pw, args);
        }
        pw.println(" mCarrierPrivilegeRegistrants: size=" + mCarrierPrivilegeRegistrants.size());
        for (int i = 0; i < mCarrierPrivilegeRegistrants.size(); i++) {
            pw.println("  mCarrierPrivilegeRegistrants[" + i + "]="
                    + ((Registrant)mCarrierPrivilegeRegistrants.get(i)).getHandler());
        }
        pw.flush();
        pw.println("mLocalLog:");
        mLocalLog.dump(fd, pw, args);
        pw.flush();
    }
}
