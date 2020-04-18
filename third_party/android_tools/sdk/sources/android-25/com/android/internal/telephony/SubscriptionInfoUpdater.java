/*
* Copyright (C) 2014 The Android Open Source Project
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

package com.android.internal.telephony;

import android.app.ActivityManagerNative;
import android.app.IUserSwitchObserver;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.IPackageManager;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.IRemoteCallback;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.CarrierConfigManager;
import android.telephony.Rlog;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.IccConstants;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccUtils;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import static android.Manifest.permission.READ_PHONE_STATE;
import static android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE;

/**
 *@hide
 */
public class SubscriptionInfoUpdater extends Handler {
    private static final String LOG_TAG = "SubscriptionInfoUpdater";
    private static final int PROJECT_SIM_NUM = TelephonyManager.getDefault().getPhoneCount();

    private static final int EVENT_SIM_LOCKED_QUERY_ICCID_DONE = 1;
    private static final int EVENT_GET_NETWORK_SELECTION_MODE_DONE = 2;
    private static final int EVENT_SIM_LOADED = 3;
    private static final int EVENT_SIM_ABSENT = 4;
    private static final int EVENT_SIM_LOCKED = 5;
    private static final int EVENT_SIM_IO_ERROR = 6;
    private static final int EVENT_SIM_UNKNOWN = 7;
    private static final int EVENT_SIM_RESTRICTED = 8;

    private static final String ICCID_STRING_FOR_NO_SIM = "";
    /**
     *  int[] sInsertSimState maintains all slots' SIM inserted status currently,
     *  it may contain 4 kinds of values:
     *    SIM_NOT_INSERT : no SIM inserted in slot i now
     *    SIM_CHANGED    : a valid SIM insert in slot i and is different SIM from last time
     *                     it will later become SIM_NEW or SIM_REPOSITION during update procedure
     *    SIM_NOT_CHANGE : a valid SIM insert in slot i and is the same SIM as last time
     *    SIM_NEW        : a valid SIM insert in slot i and is a new SIM
     *    SIM_REPOSITION : a valid SIM insert in slot i and is inserted in different slot last time
     *    positive integer #: index to distinguish SIM cards with the same IccId
     */
    public static final int SIM_NOT_CHANGE = 0;
    public static final int SIM_CHANGED    = -1;
    public static final int SIM_NEW        = -2;
    public static final int SIM_REPOSITION = -3;
    public static final int SIM_NOT_INSERT = -99;

    public static final int STATUS_NO_SIM_INSERTED = 0x00;
    public static final int STATUS_SIM1_INSERTED = 0x01;
    public static final int STATUS_SIM2_INSERTED = 0x02;
    public static final int STATUS_SIM3_INSERTED = 0x04;
    public static final int STATUS_SIM4_INSERTED = 0x08;

    // Key used to read/write the current IMSI. Updated on SIM_STATE_CHANGED - LOADED.
    public static final String CURR_SUBID = "curr_subid";

    private static Phone[] mPhone;
    private static Context mContext = null;
    private static String mIccId[] = new String[PROJECT_SIM_NUM];
    private static int[] mInsertSimState = new int[PROJECT_SIM_NUM];
    private SubscriptionManager mSubscriptionManager = null;
    private IPackageManager mPackageManager;
    private UserManager mUserManager;
    private Map<Integer, Intent> rebroadcastIntentsOnUnlock = new HashMap<>();

    // The current foreground user ID.
    private int mCurrentlyActiveUserId;
    private CarrierServiceBindHelper mCarrierServiceBindHelper;

    public SubscriptionInfoUpdater(Context context, Phone[] phone, CommandsInterface[] ci) {
        logd("Constructor invoked");

        mContext = context;
        mPhone = phone;
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mPackageManager = IPackageManager.Stub.asInterface(ServiceManager.getService("package"));
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);

        IntentFilter intentFilter = new IntentFilter(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        intentFilter.addAction(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED);
        intentFilter.addAction(Intent.ACTION_USER_UNLOCKED);
        mContext.registerReceiver(sReceiver, intentFilter);

        mCarrierServiceBindHelper = new CarrierServiceBindHelper(mContext);
        initializeCarrierApps();
    }

    private void initializeCarrierApps() {
        // Initialize carrier apps:
        // -Now (on system startup)
        // -Whenever new carrier privilege rules might change (new SIM is loaded)
        // -Whenever we switch to a new user
        mCurrentlyActiveUserId = 0;
        try {
            ActivityManagerNative.getDefault().registerUserSwitchObserver(
                    new IUserSwitchObserver.Stub() {
                @Override
                public void onUserSwitching(int newUserId, IRemoteCallback reply)
                        throws RemoteException {
                    mCurrentlyActiveUserId = newUserId;
                    CarrierAppUtils.disableCarrierAppsUntilPrivileged(mContext.getOpPackageName(),
                            mPackageManager, TelephonyManager.getDefault(),
                            mContext.getContentResolver(), mCurrentlyActiveUserId);

                    if (reply != null) {
                        try {
                            reply.sendResult(null);
                        } catch (RemoteException e) {
                        }
                    }
                }

                @Override
                public void onUserSwitchComplete(int newUserId) {
                    // Ignore.
                }

                @Override
                public void onForegroundProfileSwitch(int newProfileId) throws RemoteException {
                    // Ignore.
                }
            }, LOG_TAG);
            mCurrentlyActiveUserId = ActivityManagerNative.getDefault().getCurrentUser().id;
        } catch (RemoteException e) {
            logd("Couldn't get current user ID; guessing it's 0: " + e.getMessage());
        }
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(mContext.getOpPackageName(),
                mPackageManager, TelephonyManager.getDefault(), mContext.getContentResolver(),
                mCurrentlyActiveUserId);
    }

    private final BroadcastReceiver sReceiver = new  BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            logd("[Receiver]+");
            String action = intent.getAction();
            logd("Action: " + action);

            if (action.equals(Intent.ACTION_USER_UNLOCKED)) {
                // broadcast pending intents
                Iterator iterator = rebroadcastIntentsOnUnlock.entrySet().iterator();
                while (iterator.hasNext()) {
                    Map.Entry pair = (Map.Entry) iterator.next();
                    Intent i = (Intent)pair.getValue();
                    iterator.remove();
                    logd("Broadcasting intent ACTION_SIM_STATE_CHANGED for mCardIndex: " +
                            pair.getKey());
                    ActivityManagerNative.broadcastStickyIntent(i, READ_PHONE_STATE,
                            UserHandle.USER_ALL);
                }
                logd("[Receiver]-");
                return;
            }

            if (!action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED) &&
                    !action.equals(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED)) {
                return;
            }

            int slotId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                    SubscriptionManager.INVALID_SIM_SLOT_INDEX);
            logd("slotId: " + slotId);
            if (slotId == SubscriptionManager.INVALID_SIM_SLOT_INDEX) {
                return;
            }

            String simStatus = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
            logd("simStatus: " + simStatus);

            if (action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                rebroadcastIntentsOnUnlock.put(slotId, intent);
                if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(simStatus)) {
                    sendMessage(obtainMessage(EVENT_SIM_ABSENT, slotId, -1));
                } else if (IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(simStatus)) {
                    sendMessage(obtainMessage(EVENT_SIM_UNKNOWN, slotId, -1));
                } else if (IccCardConstants.INTENT_VALUE_ICC_CARD_IO_ERROR.equals(simStatus)) {
                    sendMessage(obtainMessage(EVENT_SIM_IO_ERROR, slotId, -1));
                } else if (IccCardConstants.INTENT_VALUE_ICC_CARD_RESTRICTED.equals(simStatus)) {
                    sendMessage(obtainMessage(EVENT_SIM_RESTRICTED, slotId, -1));
                } else {
                    logd("Ignoring simStatus: " + simStatus);
                }
            } else if (action.equals(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED)) {
                if (IccCardConstants.INTENT_VALUE_ICC_LOCKED.equals(simStatus)) {
                    String reason = intent.getStringExtra(
                        IccCardConstants.INTENT_KEY_LOCKED_REASON);
                    sendMessage(obtainMessage(EVENT_SIM_LOCKED, slotId, -1, reason));
                } else if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(simStatus)) {
                    sendMessage(obtainMessage(EVENT_SIM_LOADED, slotId, -1));
                } else {
                    logd("Ignoring simStatus: " + simStatus);
                }
            }
            logd("[Receiver]-");
        }
    };

    private boolean isAllIccIdQueryDone() {
        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            if (mIccId[i] == null) {
                logd("Wait for SIM" + (i + 1) + " IccId");
                return false;
            }
        }
        logd("All IccIds query complete");

        return true;
    }

    public void setDisplayNameForNewSub(String newSubName, int subId, int newNameSource) {
        SubscriptionInfo subInfo = mSubscriptionManager.getActiveSubscriptionInfo(subId);
        if (subInfo != null) {
            // overwrite SIM display name if it is not assigned by user
            int oldNameSource = subInfo.getNameSource();
            CharSequence oldSubName = subInfo.getDisplayName();
            logd("[setDisplayNameForNewSub] subId = " + subInfo.getSubscriptionId()
                    + ", oldSimName = " + oldSubName + ", oldNameSource = " + oldNameSource
                    + ", newSubName = " + newSubName + ", newNameSource = " + newNameSource);
            if (oldSubName == null ||
                (oldNameSource ==
                    SubscriptionManager.NAME_SOURCE_DEFAULT_SOURCE && newSubName != null) ||
                (oldNameSource == SubscriptionManager.NAME_SOURCE_SIM_SOURCE && newSubName != null
                        && !newSubName.equals(oldSubName))) {
                mSubscriptionManager.setDisplayName(newSubName, subInfo.getSubscriptionId(),
                        newNameSource);
            }
        } else {
            logd("SUB" + (subId + 1) + " SubInfo not created yet");
        }
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case EVENT_SIM_LOCKED_QUERY_ICCID_DONE: {
                AsyncResult ar = (AsyncResult)msg.obj;
                QueryIccIdUserObj uObj = (QueryIccIdUserObj) ar.userObj;
                int slotId = uObj.slotId;
                logd("handleMessage : <EVENT_SIM_LOCKED_QUERY_ICCID_DONE> SIM" + (slotId + 1));
                if (ar.exception == null) {
                    if (ar.result != null) {
                        byte[] data = (byte[])ar.result;
                        mIccId[slotId] = IccUtils.bcdToString(data, 0, data.length);
                    } else {
                        logd("Null ar");
                        mIccId[slotId] = ICCID_STRING_FOR_NO_SIM;
                    }
                } else {
                    mIccId[slotId] = ICCID_STRING_FOR_NO_SIM;
                    logd("Query IccId fail: " + ar.exception);
                }
                logd("sIccId[" + slotId + "] = " + mIccId[slotId]);
                if (isAllIccIdQueryDone()) {
                    updateSubscriptionInfoByIccId();
                }
                broadcastSimStateChanged(slotId, IccCardConstants.INTENT_VALUE_ICC_LOCKED,
                                         uObj.reason);
                if (!ICCID_STRING_FOR_NO_SIM.equals(mIccId[slotId])) {
                    updateCarrierServices(slotId, IccCardConstants.INTENT_VALUE_ICC_LOCKED);
                }
                break;
            }

            case EVENT_GET_NETWORK_SELECTION_MODE_DONE: {
                AsyncResult ar = (AsyncResult)msg.obj;
                Integer slotId = (Integer)ar.userObj;
                if (ar.exception == null && ar.result != null) {
                    int[] modes = (int[])ar.result;
                    if (modes[0] == 1) {  // Manual mode.
                        mPhone[slotId].setNetworkSelectionModeAutomatic(null);
                    }
                } else {
                    logd("EVENT_GET_NETWORK_SELECTION_MODE_DONE: error getting network mode.");
                }
                break;
            }

           case EVENT_SIM_LOADED:
                handleSimLoaded(msg.arg1);
                break;

            case EVENT_SIM_ABSENT:
                handleSimAbsent(msg.arg1);
                break;

            case EVENT_SIM_LOCKED:
                handleSimLocked(msg.arg1, (String) msg.obj);
                break;

            case EVENT_SIM_UNKNOWN:
                updateCarrierServices(msg.arg1, IccCardConstants.INTENT_VALUE_ICC_UNKNOWN);
                break;

            case EVENT_SIM_IO_ERROR:
                updateCarrierServices(msg.arg1, IccCardConstants.INTENT_VALUE_ICC_CARD_IO_ERROR);
                break;

            case EVENT_SIM_RESTRICTED:
                updateCarrierServices(msg.arg1, IccCardConstants.INTENT_VALUE_ICC_CARD_RESTRICTED);
                break;

            default:
                logd("Unknown msg:" + msg.what);
        }
    }

    private static class QueryIccIdUserObj {
        public String reason;
        public int slotId;

        QueryIccIdUserObj(String reason, int slotId) {
            this.reason = reason;
            this.slotId = slotId;
        }
    };

    private void handleSimLocked(int slotId, String reason) {
        if (mIccId[slotId] != null && mIccId[slotId].equals(ICCID_STRING_FOR_NO_SIM)) {
            logd("SIM" + (slotId + 1) + " hot plug in");
            mIccId[slotId] = null;
        }


        IccFileHandler fileHandler = mPhone[slotId].getIccCard() == null ? null :
                mPhone[slotId].getIccCard().getIccFileHandler();

        if (fileHandler != null) {
            String iccId = mIccId[slotId];
            if (iccId == null) {
                logd("Querying IccId");
                fileHandler.loadEFTransparent(IccConstants.EF_ICCID,
                        obtainMessage(EVENT_SIM_LOCKED_QUERY_ICCID_DONE,
                                new QueryIccIdUserObj(reason, slotId)));
            } else {
                logd("NOT Querying IccId its already set sIccid[" + slotId + "]=" + iccId);
                updateCarrierServices(slotId, IccCardConstants.INTENT_VALUE_ICC_LOCKED);
                broadcastSimStateChanged(slotId, IccCardConstants.INTENT_VALUE_ICC_LOCKED, reason);
            }
        } else {
            logd("sFh[" + slotId + "] is null, ignore");
        }
    }

    private void handleSimLoaded(int slotId) {
        logd("handleSimStateLoadedInternal: slotId: " + slotId);

        // The SIM should be loaded at this state, but it is possible in cases such as SIM being
        // removed or a refresh RESET that the IccRecords could be null. The right behavior is to
        // not broadcast the SIM loaded.
        IccRecords records = mPhone[slotId].getIccCard().getIccRecords();
        if (records == null) {  // Possibly a race condition.
            logd("onRecieve: IccRecords null");
            return;
        }
        if (records.getIccId() == null) {
            logd("onRecieve: IccID null");
            return;
        }
        mIccId[slotId] = records.getIccId();

        if (isAllIccIdQueryDone()) {
            updateSubscriptionInfoByIccId();
        }

        int subId = SubscriptionManager.DEFAULT_SUBSCRIPTION_ID;
        int[] subIds = SubscriptionController.getInstance().getSubId(slotId);
        if (subIds != null) {   // Why an array?
            subId = subIds[0];
        }

        if (SubscriptionManager.isValidSubscriptionId(subId)) {
            TelephonyManager tm = TelephonyManager.getDefault();

            String operator = tm.getSimOperatorNumericForPhone(slotId);

            if (!TextUtils.isEmpty(operator)) {
                if (subId == SubscriptionController.getInstance().getDefaultSubId()) {
                    MccTable.updateMccMncConfiguration(mContext, operator, false);
                }
                SubscriptionController.getInstance().setMccMnc(operator, subId);
            } else {
                logd("EVENT_RECORDS_LOADED Operator name is null");
            }

            String msisdn = tm.getLine1Number(subId);
            ContentResolver contentResolver = mContext.getContentResolver();

            if (msisdn != null) {
                ContentValues number = new ContentValues(1);
                number.put(SubscriptionManager.NUMBER, msisdn);
                contentResolver.update(SubscriptionManager.CONTENT_URI, number,
                        SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "="
                        + Long.toString(subId), null);
            }

            SubscriptionInfo subInfo = mSubscriptionManager.getActiveSubscriptionInfo(subId);
            String nameToSet;
            String simCarrierName = tm.getSimOperatorName(subId);
            ContentValues name = new ContentValues(1);

            if (subInfo != null && subInfo.getNameSource() !=
                    SubscriptionManager.NAME_SOURCE_USER_INPUT) {
                if (!TextUtils.isEmpty(simCarrierName)) {
                    nameToSet = simCarrierName;
                } else {
                    nameToSet = "CARD " + Integer.toString(slotId + 1);
                }
                name.put(SubscriptionManager.DISPLAY_NAME, nameToSet);
                logd("sim name = " + nameToSet);
                contentResolver.update(SubscriptionManager.CONTENT_URI, name,
                        SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID
                        + "=" + Long.toString(subId), null);
            }

            /* Update preferred network type and network selection mode on SIM change.
             * Storing last subId in SharedPreference for now to detect SIM change. */
            SharedPreferences sp =
                    PreferenceManager.getDefaultSharedPreferences(mContext);
            int storedSubId = sp.getInt(CURR_SUBID + slotId, -1);

            if (storedSubId != subId) {
                int networkType = RILConstants.PREFERRED_NETWORK_MODE;

                // Set the modem network mode
                mPhone[slotId].setPreferredNetworkType(networkType, null);
                Settings.Global.putInt(mPhone[slotId].getContext().getContentResolver(),
                        Settings.Global.PREFERRED_NETWORK_MODE + subId,
                        networkType);

                // Only support automatic selection mode on SIM change.
                mPhone[slotId].getNetworkSelectionMode(
                        obtainMessage(EVENT_GET_NETWORK_SELECTION_MODE_DONE, new Integer(slotId)));

                // Update stored subId
                SharedPreferences.Editor editor = sp.edit();
                editor.putInt(CURR_SUBID + slotId, subId);
                editor.apply();
            }
        } else {
            logd("Invalid subId, could not update ContentResolver");
        }

        // Update set of enabled carrier apps now that the privilege rules may have changed.
        CarrierAppUtils.disableCarrierAppsUntilPrivileged(mContext.getOpPackageName(),
                mPackageManager, TelephonyManager.getDefault(), mContext.getContentResolver(),
                mCurrentlyActiveUserId);

        broadcastSimStateChanged(slotId, IccCardConstants.INTENT_VALUE_ICC_LOADED, null);
        updateCarrierServices(slotId, IccCardConstants.INTENT_VALUE_ICC_LOADED);
    }

    private void updateCarrierServices(int slotId, String simState) {
        CarrierConfigManager configManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        configManager.updateConfigForPhoneId(slotId, simState);
        mCarrierServiceBindHelper.updateForPhoneId(slotId, simState);
    }

    private void handleSimAbsent(int slotId) {
        if (mIccId[slotId] != null && !mIccId[slotId].equals(ICCID_STRING_FOR_NO_SIM)) {
            logd("SIM" + (slotId + 1) + " hot plug out");
        }
        mIccId[slotId] = ICCID_STRING_FOR_NO_SIM;
        if (isAllIccIdQueryDone()) {
            updateSubscriptionInfoByIccId();
        }
        updateCarrierServices(slotId, IccCardConstants.INTENT_VALUE_ICC_ABSENT);
    }

    /**
     * TODO: Simplify more, as no one is interested in what happened
     * only what the current list contains.
     */
    synchronized private void updateSubscriptionInfoByIccId() {
        logd("updateSubscriptionInfoByIccId:+ Start");

        mSubscriptionManager.clearSubscriptionInfo();

        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            mInsertSimState[i] = SIM_NOT_CHANGE;
        }

        int insertedSimCount = PROJECT_SIM_NUM;
        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            if (ICCID_STRING_FOR_NO_SIM.equals(mIccId[i])) {
                insertedSimCount--;
                mInsertSimState[i] = SIM_NOT_INSERT;
            }
        }
        logd("insertedSimCount = " + insertedSimCount);

        int index = 0;
        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            if (mInsertSimState[i] == SIM_NOT_INSERT) {
                continue;
            }
            index = 2;
            for (int j = i + 1; j < PROJECT_SIM_NUM; j++) {
                if (mInsertSimState[j] == SIM_NOT_CHANGE && mIccId[i].equals(mIccId[j])) {
                    mInsertSimState[i] = 1;
                    mInsertSimState[j] = index;
                    index++;
                }
            }
        }

        ContentResolver contentResolver = mContext.getContentResolver();
        String[] oldIccId = new String[PROJECT_SIM_NUM];
        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            oldIccId[i] = null;
            List<SubscriptionInfo> oldSubInfo =
                    SubscriptionController.getInstance().getSubInfoUsingSlotIdWithCheck(i, false,
                    mContext.getOpPackageName());
            if (oldSubInfo != null) {
                oldIccId[i] = oldSubInfo.get(0).getIccId();
                logd("updateSubscriptionInfoByIccId: oldSubId = "
                        + oldSubInfo.get(0).getSubscriptionId());
                if (mInsertSimState[i] == SIM_NOT_CHANGE && !mIccId[i].equals(oldIccId[i])) {
                    mInsertSimState[i] = SIM_CHANGED;
                }
                if (mInsertSimState[i] != SIM_NOT_CHANGE) {
                    ContentValues value = new ContentValues(1);
                    value.put(SubscriptionManager.SIM_SLOT_INDEX,
                            SubscriptionManager.INVALID_SIM_SLOT_INDEX);
                    contentResolver.update(SubscriptionManager.CONTENT_URI, value,
                            SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "="
                            + Integer.toString(oldSubInfo.get(0).getSubscriptionId()), null);
                }
            } else {
                if (mInsertSimState[i] == SIM_NOT_CHANGE) {
                    // no SIM inserted last time, but there is one SIM inserted now
                    mInsertSimState[i] = SIM_CHANGED;
                }
                oldIccId[i] = ICCID_STRING_FOR_NO_SIM;
                logd("updateSubscriptionInfoByIccId: No SIM in slot " + i + " last time");
            }
        }

        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            logd("updateSubscriptionInfoByIccId: oldIccId[" + i + "] = " + oldIccId[i] +
                    ", sIccId[" + i + "] = " + mIccId[i]);
        }

        //check if the inserted SIM is new SIM
        int nNewCardCount = 0;
        int nNewSimStatus = 0;
        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            if (mInsertSimState[i] == SIM_NOT_INSERT) {
                logd("updateSubscriptionInfoByIccId: No SIM inserted in slot " + i + " this time");
            } else {
                if (mInsertSimState[i] > 0) {
                    //some special SIMs may have the same IccIds, add suffix to distinguish them
                    //FIXME: addSubInfoRecord can return an error.
                    mSubscriptionManager.addSubscriptionInfoRecord(mIccId[i]
                            + Integer.toString(mInsertSimState[i]), i);
                    logd("SUB" + (i + 1) + " has invalid IccId");
                } else /*if (sInsertSimState[i] != SIM_NOT_INSERT)*/ {
                    mSubscriptionManager.addSubscriptionInfoRecord(mIccId[i], i);
                }
                if (isNewSim(mIccId[i], oldIccId)) {
                    nNewCardCount++;
                    switch (i) {
                        case PhoneConstants.SUB1:
                            nNewSimStatus |= STATUS_SIM1_INSERTED;
                            break;
                        case PhoneConstants.SUB2:
                            nNewSimStatus |= STATUS_SIM2_INSERTED;
                            break;
                        case PhoneConstants.SUB3:
                            nNewSimStatus |= STATUS_SIM3_INSERTED;
                            break;
                        //case PhoneConstants.SUB3:
                        //    nNewSimStatus |= STATUS_SIM4_INSERTED;
                        //    break;
                    }

                    mInsertSimState[i] = SIM_NEW;
                }
            }
        }

        for (int i = 0; i < PROJECT_SIM_NUM; i++) {
            if (mInsertSimState[i] == SIM_CHANGED) {
                mInsertSimState[i] = SIM_REPOSITION;
            }
            logd("updateSubscriptionInfoByIccId: sInsertSimState[" + i + "] = "
                    + mInsertSimState[i]);
        }

        List<SubscriptionInfo> subInfos = mSubscriptionManager.getActiveSubscriptionInfoList();
        int nSubCount = (subInfos == null) ? 0 : subInfos.size();
        logd("updateSubscriptionInfoByIccId: nSubCount = " + nSubCount);
        for (int i=0; i < nSubCount; i++) {
            SubscriptionInfo temp = subInfos.get(i);

            String msisdn = TelephonyManager.getDefault().getLine1Number(
                    temp.getSubscriptionId());

            if (msisdn != null) {
                ContentValues value = new ContentValues(1);
                value.put(SubscriptionManager.NUMBER, msisdn);
                contentResolver.update(SubscriptionManager.CONTENT_URI, value,
                        SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "="
                        + Integer.toString(temp.getSubscriptionId()), null);
            }
        }

        // Ensure the modems are mapped correctly
        mSubscriptionManager.setDefaultDataSubId(
                mSubscriptionManager.getDefaultDataSubscriptionId());

        SubscriptionController.getInstance().notifySubscriptionInfoChanged();
        logd("updateSubscriptionInfoByIccId:- SsubscriptionInfo update complete");
    }

    private boolean isNewSim(String iccId, String[] oldIccId) {
        boolean newSim = true;
        for(int i = 0; i < PROJECT_SIM_NUM; i++) {
            if(iccId.equals(oldIccId[i])) {
                newSim = false;
                break;
            }
        }
        logd("newSim = " + newSim);

        return newSim;
    }

    private void broadcastSimStateChanged(int slotId, String state, String reason) {
        Intent i = new Intent(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        // TODO - we'd like this intent to have a single snapshot of all sim state,
        // but until then this should not use REPLACE_PENDING or we may lose
        // information
        // i.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING
        //         | Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        i.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        i.putExtra(PhoneConstants.PHONE_NAME_KEY, "Phone");
        i.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE, state);
        i.putExtra(IccCardConstants.INTENT_KEY_LOCKED_REASON, reason);
        SubscriptionManager.putPhoneIdAndSubIdExtra(i, slotId);
        logd("Broadcasting intent ACTION_SIM_STATE_CHANGED " + state + " reason " + reason +
             " for mCardIndex: " + slotId);
        ActivityManagerNative.broadcastStickyIntent(i, READ_PHONE_STATE, UserHandle.USER_ALL);
        rebroadcastIntentsOnUnlock.put(slotId, i);
    }

    public void dispose() {
        logd("[dispose]");
        mContext.unregisterReceiver(sReceiver);
    }

    private void logd(String message) {
        Rlog.d(LOG_TAG, message);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("SubscriptionInfoUpdater:");
        mCarrierServiceBindHelper.dump(fd, pw, args);
    }
}
