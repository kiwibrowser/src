/*
 * Copyright (c) 2013 The Android Open Source Project
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

package com.android.ims;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.provider.Settings;
import android.telecom.TelecomManager;
import android.telephony.CarrierConfigManager;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsEcbm;
import com.android.ims.internal.IImsMultiEndpoint;
import com.android.ims.internal.IImsRegistrationListener;
import com.android.ims.internal.IImsService;
import com.android.ims.internal.IImsUt;
import com.android.ims.internal.ImsCallSession;
import com.android.ims.internal.IImsConfig;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.HashMap;

/**
 * Provides APIs for IMS services, such as initiating IMS calls, and provides access to
 * the operator's IMS network. This class is the starting point for any IMS actions.
 * You can acquire an instance of it with {@link #getInstance getInstance()}.</p>
 * <p>The APIs in this class allows you to:</p>
 *
 * @hide
 */
public class ImsManager {

    /*
     * Debug flag to override configuration flag
     */
    public static final String PROPERTY_DBG_VOLTE_AVAIL_OVERRIDE = "persist.dbg.volte_avail_ovr";
    public static final int PROPERTY_DBG_VOLTE_AVAIL_OVERRIDE_DEFAULT = 0;
    public static final String PROPERTY_DBG_VT_AVAIL_OVERRIDE = "persist.dbg.vt_avail_ovr";
    public static final int PROPERTY_DBG_VT_AVAIL_OVERRIDE_DEFAULT = 0;
    public static final String PROPERTY_DBG_WFC_AVAIL_OVERRIDE = "persist.dbg.wfc_avail_ovr";
    public static final int PROPERTY_DBG_WFC_AVAIL_OVERRIDE_DEFAULT = 0;
    public static final String PROPERTY_DBG_ALLOW_IMS_OFF_OVERRIDE = "persist.dbg.allow_ims_off";
    public static final int PROPERTY_DBG_ALLOW_IMS_OFF_OVERRIDE_DEFAULT = 0;

    /**
     * For accessing the IMS related service.
     * Internal use only.
     * @hide
     */
    private static final String IMS_SERVICE = "ims";

    /**
     * The result code to be sent back with the incoming call {@link PendingIntent}.
     * @see #open(PendingIntent, ImsConnectionStateListener)
     */
    public static final int INCOMING_CALL_RESULT_CODE = 101;

    /**
     * Key to retrieve the call ID from an incoming call intent.
     * @see #open(PendingIntent, ImsConnectionStateListener)
     */
    public static final String EXTRA_CALL_ID = "android:imsCallID";

    /**
     * Action to broadcast when ImsService is up.
     * Internal use only.
     * @hide
     */
    public static final String ACTION_IMS_SERVICE_UP =
            "com.android.ims.IMS_SERVICE_UP";

    /**
     * Action to broadcast when ImsService is down.
     * Internal use only.
     * @hide
     */
    public static final String ACTION_IMS_SERVICE_DOWN =
            "com.android.ims.IMS_SERVICE_DOWN";

    /**
     * Action to broadcast when ImsService registration fails.
     * Internal use only.
     * @hide
     */
    public static final String ACTION_IMS_REGISTRATION_ERROR =
            "com.android.ims.REGISTRATION_ERROR";

    /**
     * Part of the ACTION_IMS_SERVICE_UP or _DOWN intents.
     * A long value; the phone ID corresponding to the IMS service coming up or down.
     * Internal use only.
     * @hide
     */
    public static final String EXTRA_PHONE_ID = "android:phone_id";

    /**
     * Action for the incoming call intent for the Phone app.
     * Internal use only.
     * @hide
     */
    public static final String ACTION_IMS_INCOMING_CALL =
            "com.android.ims.IMS_INCOMING_CALL";

    /**
     * Part of the ACTION_IMS_INCOMING_CALL intents.
     * An integer value; service identifier obtained from {@link ImsManager#open}.
     * Internal use only.
     * @hide
     */
    public static final String EXTRA_SERVICE_ID = "android:imsServiceId";

    /**
     * Part of the ACTION_IMS_INCOMING_CALL intents.
     * An boolean value; Flag to indicate that the incoming call is a normal call or call for USSD.
     * The value "true" indicates that the incoming call is for USSD.
     * Internal use only.
     * @hide
     */
    public static final String EXTRA_USSD = "android:ussd";

    /**
     * Part of the ACTION_IMS_INCOMING_CALL intents.
     * A boolean value; Flag to indicate whether the call is an unknown
     * dialing call. Such calls are originated by sending commands (like
     * AT commands) directly to modem without Android involvement.
     * Even though they are not incoming calls, they are propagated
     * to Phone app using same ACTION_IMS_INCOMING_CALL intent.
     * Internal use only.
     * @hide
     */
    public static final String EXTRA_IS_UNKNOWN_CALL = "android:isUnknown";

    private static final String TAG = "ImsManager";
    private static final boolean DBG = true;

    private static HashMap<Integer, ImsManager> sImsManagerInstances =
            new HashMap<Integer, ImsManager>();

    private Context mContext;
    private int mPhoneId;
    private IImsService mImsService = null;
    private ImsServiceDeathRecipient mDeathRecipient = new ImsServiceDeathRecipient();
    // Ut interface for the supplementary service configuration
    private ImsUt mUt = null;
    // Interface to get/set ims config items
    private ImsConfig mConfig = null;
    private boolean mConfigUpdated = false;

    private ImsConfigListener mImsConfigListener;

    // ECBM interface
    private ImsEcbm mEcbm = null;

    private ImsMultiEndpoint mMultiEndpoint = null;

    // SystemProperties used as cache
    private static final String VOLTE_PROVISIONED_PROP = "net.lte.ims.volte.provisioned";
    private static final String WFC_PROVISIONED_PROP = "net.lte.ims.wfc.provisioned";
    private static final String VT_PROVISIONED_PROP = "net.lte.ims.vt.provisioned";
    // Flag indicating data enabled or not. This flag should be in sync with
    // DcTracker.isDataEnabled(). The flag will be set later during boot up.
    private static final String DATA_ENABLED_PROP = "net.lte.ims.data.enabled";

    public static final String TRUE = "true";
    public static final String FALSE = "false";

    /**
     * Gets a manager instance.
     *
     * @param context application context for creating the manager object
     * @param phoneId the phone ID for the IMS Service
     * @return the manager instance corresponding to the phoneId
     */
    public static ImsManager getInstance(Context context, int phoneId) {
        synchronized (sImsManagerInstances) {
            if (sImsManagerInstances.containsKey(phoneId))
                return sImsManagerInstances.get(phoneId);

            ImsManager mgr = new ImsManager(context, phoneId);
            sImsManagerInstances.put(phoneId, mgr);

            return mgr;
        }
    }

    /**
     * Returns the user configuration of Enhanced 4G LTE Mode setting
     */
    public static boolean isEnhanced4gLteModeSettingEnabledByUser(Context context) {
        // If user can't edit Enhanced 4G LTE Mode, it assumes Enhanced 4G LTE Mode is always true.
        // If user changes SIM from editable mode to uneditable mode, need to return true.
        if (!getBooleanCarrierConfig(context,
                    CarrierConfigManager.KEY_EDITABLE_ENHANCED_4G_LTE_BOOL)) {
            return true;
        }
        int enabled = android.provider.Settings.Global.getInt(
                    context.getContentResolver(),
                    android.provider.Settings.Global.ENHANCED_4G_MODE_ENABLED, ImsConfig.FeatureValueConstants.ON);
        return (enabled == 1) ? true : false;
    }

    /**
     * Change persistent Enhanced 4G LTE Mode setting
     */
    public static void setEnhanced4gLteModeSetting(Context context, boolean enabled) {
        int value = enabled ? 1 : 0;
        android.provider.Settings.Global.putInt(
                context.getContentResolver(),
                android.provider.Settings.Global.ENHANCED_4G_MODE_ENABLED, value);

        if (isNonTtyOrTtyOnVolteEnabled(context)) {
            ImsManager imsManager = ImsManager.getInstance(context,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (imsManager != null) {
                try {
                    imsManager.setAdvanced4GMode(enabled);
                } catch (ImsException ie) {
                    // do nothing
                }
            }
        }
    }

    /**
     * Indicates whether the call is non-TTY or if TTY - whether TTY on VoLTE is
     * supported.
     */
    public static boolean isNonTtyOrTtyOnVolteEnabled(Context context) {
        if (getBooleanCarrierConfig(context,
                CarrierConfigManager.KEY_CARRIER_VOLTE_TTY_SUPPORTED_BOOL)) {
            return true;
        }

        return Settings.Secure.getInt(context.getContentResolver(),
                Settings.Secure.PREFERRED_TTY_MODE, TelecomManager.TTY_MODE_OFF)
                == TelecomManager.TTY_MODE_OFF;
    }

    /**
     * Returns a platform configuration for VoLTE which may override the user setting.
     */
    public static boolean isVolteEnabledByPlatform(Context context) {
        if (SystemProperties.getInt(PROPERTY_DBG_VOLTE_AVAIL_OVERRIDE,
                PROPERTY_DBG_VOLTE_AVAIL_OVERRIDE_DEFAULT) == 1) {
            return true;
        }

        return context.getResources().getBoolean(
                com.android.internal.R.bool.config_device_volte_available)
                && getBooleanCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_VOLTE_AVAILABLE_BOOL)
                && isGbaValid(context);
    }

    /**
     * Indicates whether VoLTE is provisioned on device
     */
    public static boolean isVolteProvisionedOnDevice(Context context) {
        if (getBooleanCarrierConfig(context,
                    CarrierConfigManager.KEY_CARRIER_VOLTE_PROVISIONING_REQUIRED_BOOL)) {
            ImsManager mgr = ImsManager.getInstance(context,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (mgr != null) {
                return mgr.isVolteProvisioned();
            }
        }

        return true;
    }

    /**
     * Indicates whether VoWifi is provisioned on device
     */
    public static boolean isWfcProvisionedOnDevice(Context context) {
        if (getBooleanCarrierConfig(context,
                CarrierConfigManager.KEY_CARRIER_VOLTE_PROVISIONING_REQUIRED_BOOL)) {
            ImsManager mgr = ImsManager.getInstance(context,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (mgr != null) {
                return mgr.isWfcProvisioned();
            }
        }

        return true;
    }

    /**
     * Indicates whether VT is provisioned on device
     */
    public static boolean isVtProvisionedOnDevice(Context context) {
        if (getBooleanCarrierConfig(context,
                CarrierConfigManager.KEY_CARRIER_VOLTE_PROVISIONING_REQUIRED_BOOL)) {
            ImsManager mgr = ImsManager.getInstance(context,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (mgr != null) {
                return mgr.isVtProvisioned();
            }
        }

        return true;
    }

    /**
     * Returns a platform configuration for VT which may override the user setting.
     *
     * Note: VT presumes that VoLTE is enabled (these are configuration settings
     * which must be done correctly).
     */
    public static boolean isVtEnabledByPlatform(Context context) {
        if (SystemProperties.getInt(PROPERTY_DBG_VT_AVAIL_OVERRIDE,
                PROPERTY_DBG_VT_AVAIL_OVERRIDE_DEFAULT) == 1) {
            return true;
        }

        return
                context.getResources().getBoolean(
                        com.android.internal.R.bool.config_device_vt_available) &&
                getBooleanCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_VT_AVAILABLE_BOOL) &&
                isGbaValid(context);
    }

    /**
     * Returns the user configuration of VT setting
     */
    public static boolean isVtEnabledByUser(Context context) {
        int enabled = android.provider.Settings.Global.getInt(context.getContentResolver(),
                android.provider.Settings.Global.VT_IMS_ENABLED,
                ImsConfig.FeatureValueConstants.ON);
        return (enabled == 1) ? true : false;
    }

    /**
     * Change persistent VT enabled setting
     */
    public static void setVtSetting(Context context, boolean enabled) {
        int value = enabled ? 1 : 0;
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.VT_IMS_ENABLED, value);

        ImsManager imsManager = ImsManager.getInstance(context,
                SubscriptionManager.getDefaultVoicePhoneId());
        if (imsManager != null) {
            try {
                ImsConfig config = imsManager.getConfigInterface();
                config.setFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE,
                        TelephonyManager.NETWORK_TYPE_LTE,
                        enabled ? ImsConfig.FeatureValueConstants.ON
                                : ImsConfig.FeatureValueConstants.OFF,
                        imsManager.mImsConfigListener);

                if (enabled) {
                    log("setVtSetting() : turnOnIms");
                    imsManager.turnOnIms();
                } else if (isTurnOffImsAllowedByPlatform(context)
                        && (!isVolteEnabledByPlatform(context)
                        || !isEnhanced4gLteModeSettingEnabledByUser(context))) {
                    log("setVtSetting() : imsServiceAllowTurnOff -> turnOffIms");
                    imsManager.turnOffIms();
                }
            } catch (ImsException e) {
                loge("setVtSetting(): ", e);
            }
        }
    }

    /*
     * Returns whether turning off ims is allowed by platform.
     * The platform property may override the carrier config.
     */
    private static boolean isTurnOffImsAllowedByPlatform(Context context) {
        if (SystemProperties.getInt(PROPERTY_DBG_ALLOW_IMS_OFF_OVERRIDE,
                PROPERTY_DBG_ALLOW_IMS_OFF_OVERRIDE_DEFAULT) == 1) {
            return true;
        }
        return getBooleanCarrierConfig(context,
                CarrierConfigManager.KEY_CARRIER_ALLOW_TURNOFF_IMS_BOOL);
    }

    /**
     * Returns the user configuration of WFC setting
     */
    public static boolean isWfcEnabledByUser(Context context) {
        int enabled = android.provider.Settings.Global.getInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_ENABLED,
                getBooleanCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_ENABLED_BOOL) ?
                        ImsConfig.FeatureValueConstants.ON : ImsConfig.FeatureValueConstants.OFF);
        return (enabled == 1) ? true : false;
    }

    /**
     * Change persistent WFC enabled setting
     */
    public static void setWfcSetting(Context context, boolean enabled) {
        int value = enabled ? 1 : 0;
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_ENABLED, value);

        ImsManager imsManager = ImsManager.getInstance(context,
                SubscriptionManager.getDefaultVoicePhoneId());
        if (imsManager != null) {
            try {
                ImsConfig config = imsManager.getConfigInterface();
                config.setFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI,
                        TelephonyManager.NETWORK_TYPE_IWLAN,
                        enabled ? ImsConfig.FeatureValueConstants.ON
                                : ImsConfig.FeatureValueConstants.OFF,
                        imsManager.mImsConfigListener);

                if (enabled) {
                    log("setWfcSetting() : turnOnIms");
                    imsManager.turnOnIms();
                } else if (isTurnOffImsAllowedByPlatform(context)
                        && (!isVolteEnabledByPlatform(context)
                        || !isEnhanced4gLteModeSettingEnabledByUser(context))) {
                    log("setWfcSetting() : imsServiceAllowTurnOff -> turnOffIms");
                    imsManager.turnOffIms();
                }

                // Force IMS to register over LTE when turning off WFC
                setWfcModeInternal(context, enabled
                        ? getWfcMode(context)
                        : ImsConfig.WfcModeFeatureValueConstants.CELLULAR_PREFERRED);
            } catch (ImsException e) {
                loge("setWfcSetting(): ", e);
            }
        }
    }

    /**
     * Returns the user configuration of WFC preference setting
     */
    public static int getWfcMode(Context context) {
        int setting = android.provider.Settings.Global.getInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_MODE, getIntCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_MODE_INT));
        if (DBG) log("getWfcMode - setting=" + setting);
        return setting;
    }

    /**
     * Change persistent WFC preference setting
     */
    public static void setWfcMode(Context context, int wfcMode) {
        if (DBG) log("setWfcMode - setting=" + wfcMode);
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_MODE, wfcMode);

        setWfcModeInternal(context, wfcMode);
    }

    /**
     * Returns the user configuration of WFC preference setting
     *
     * @param roaming {@code false} for home network setting, {@code true} for roaming  setting
     */
    public static int getWfcMode(Context context, boolean roaming) {
        int setting = 0;
        if (!roaming) {
            setting = android.provider.Settings.Global.getInt(context.getContentResolver(),
                    android.provider.Settings.Global.WFC_IMS_MODE, getIntCarrierConfig(context,
                            CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_MODE_INT));
            if (DBG) log("getWfcMode - setting=" + setting);
        } else {
            setting = android.provider.Settings.Global.getInt(context.getContentResolver(),
                    android.provider.Settings.Global.WFC_IMS_ROAMING_MODE,
                    getIntCarrierConfig(context,
                            CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_ROAMING_MODE_INT));
            if (DBG) log("getWfcMode (roaming) - setting=" + setting);
        }
        return setting;
    }

    /**
     * Change persistent WFC preference setting
     *
     * @param roaming {@code false} for home network setting, {@code true} for roaming setting
     */
    public static void setWfcMode(Context context, int wfcMode, boolean roaming) {
        if (!roaming) {
            if (DBG) log("setWfcMode - setting=" + wfcMode);
            android.provider.Settings.Global.putInt(context.getContentResolver(),
                    android.provider.Settings.Global.WFC_IMS_MODE, wfcMode);
        } else {
            if (DBG) log("setWfcMode (roaming) - setting=" + wfcMode);
            android.provider.Settings.Global.putInt(context.getContentResolver(),
                    android.provider.Settings.Global.WFC_IMS_ROAMING_MODE, wfcMode);
        }

        TelephonyManager tm = (TelephonyManager)
                context.getSystemService(Context.TELEPHONY_SERVICE);
        if (roaming == tm.isNetworkRoaming()) {
            setWfcModeInternal(context, wfcMode);
        }
    }

    private static void setWfcModeInternal(Context context, int wfcMode) {
        final ImsManager imsManager = ImsManager.getInstance(context,
                SubscriptionManager.getDefaultVoicePhoneId());
        if (imsManager != null) {
            final int value = wfcMode;
            Thread thread = new Thread(new Runnable() {
                public void run() {
                    try {
                        imsManager.getConfigInterface().setProvisionedValue(
                                ImsConfig.ConfigConstants.VOICE_OVER_WIFI_MODE,
                                value);
                    } catch (ImsException e) {
                        // do nothing
                    }
                }
            });
            thread.start();
        }
    }

    /**
     * Returns the user configuration of WFC roaming setting
     */
    public static boolean isWfcRoamingEnabledByUser(Context context) {
        int enabled = android.provider.Settings.Global.getInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_ROAMING_ENABLED,
                getBooleanCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_ROAMING_ENABLED_BOOL) ?
                        ImsConfig.FeatureValueConstants.ON : ImsConfig.FeatureValueConstants.OFF);
        return (enabled == 1) ? true : false;
    }

    /**
     * Change persistent WFC roaming enabled setting
     */
    public static void setWfcRoamingSetting(Context context, boolean enabled) {
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_ROAMING_ENABLED,
                enabled ? ImsConfig.FeatureValueConstants.ON
                        : ImsConfig.FeatureValueConstants.OFF);

        setWfcRoamingSettingInternal(context, enabled);
    }

    private static void setWfcRoamingSettingInternal(Context context, boolean enabled) {
        final ImsManager imsManager = ImsManager.getInstance(context,
                SubscriptionManager.getDefaultVoicePhoneId());
        if (imsManager != null) {
            final int value = enabled
                    ? ImsConfig.FeatureValueConstants.ON
                    : ImsConfig.FeatureValueConstants.OFF;
            Thread thread = new Thread(new Runnable() {
                public void run() {
                    try {
                        imsManager.getConfigInterface().setProvisionedValue(
                                ImsConfig.ConfigConstants.VOICE_OVER_WIFI_ROAMING,
                                value);
                    } catch (ImsException e) {
                        // do nothing
                    }
                }
            });
            thread.start();
        }
    }

    /**
     * Returns a platform configuration for WFC which may override the user
     * setting. Note: WFC presumes that VoLTE is enabled (these are
     * configuration settings which must be done correctly).
     */
    public static boolean isWfcEnabledByPlatform(Context context) {
        if (SystemProperties.getInt(PROPERTY_DBG_WFC_AVAIL_OVERRIDE,
                PROPERTY_DBG_WFC_AVAIL_OVERRIDE_DEFAULT) == 1) {
            return true;
        }

        return
               context.getResources().getBoolean(
                       com.android.internal.R.bool.config_device_wfc_ims_available) &&
               getBooleanCarrierConfig(context,
                       CarrierConfigManager.KEY_CARRIER_WFC_IMS_AVAILABLE_BOOL) &&
               isGbaValid(context);
    }

    /**
     * If carrier requires that IMS is only available if GBA capable SIM is used,
     * then this function checks GBA bit in EF IST.
     *
     * Format of EF IST is defined in 3GPP TS 31.103 (Section 4.2.7).
     */
    private static boolean isGbaValid(Context context) {
        if (getBooleanCarrierConfig(context,
                CarrierConfigManager.KEY_CARRIER_IMS_GBA_REQUIRED_BOOL)) {
            final TelephonyManager telephonyManager = TelephonyManager.getDefault();
            String efIst = telephonyManager.getIsimIst();
            if (efIst == null) {
                loge("ISF is NULL");
                return true;
            }
            boolean result = efIst != null && efIst.length() > 1 &&
                    (0x02 & (byte)efIst.charAt(1)) != 0;
            if (DBG) log("GBA capable=" + result + ", ISF=" + efIst);
            return result;
        }
        return true;
    }

    /**
     * This function should be called when ImsConfig.ACTION_IMS_CONFIG_CHANGED is received.
     *
     * We cannot register receiver in ImsManager because this would lead to resource leak.
     * ImsManager can be created in different processes and it is not notified when that process
     * is about to be terminated.
     *
     * @hide
     * */
    public static void onProvisionedValueChanged(Context context, int item, String value) {
        if (DBG) Rlog.d(TAG, "onProvisionedValueChanged: item=" + item + " val=" + value);
        ImsManager mgr = ImsManager.getInstance(context,
                SubscriptionManager.getDefaultVoicePhoneId());

        switch (item) {
            case ImsConfig.ConfigConstants.VLT_SETTING_ENABLED:
                mgr.setVolteProvisionedProperty(value.equals("1"));
                if (DBG) Rlog.d(TAG,"isVoLteProvisioned = " + mgr.isVolteProvisioned());
                break;

            case ImsConfig.ConfigConstants.VOICE_OVER_WIFI_SETTING_ENABLED:
                mgr.setWfcProvisionedProperty(value.equals("1"));
                if (DBG) Rlog.d(TAG,"isWfcProvisioned = " + mgr.isWfcProvisioned());
                break;

            case ImsConfig.ConfigConstants.LVC_SETTING_ENABLED:
                mgr.setVtProvisionedProperty(value.equals("1"));
                if (DBG) Rlog.d(TAG,"isVtProvisioned = " + mgr.isVtProvisioned());
                break;

        }
    }

    private class AsyncUpdateProvisionedValues extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... params) {
            // disable on any error
            setVolteProvisionedProperty(false);
            setWfcProvisionedProperty(false);
            setVtProvisionedProperty(false);

            try {
                ImsConfig config = getConfigInterface();
                if (config != null) {
                    setVolteProvisionedProperty(getProvisionedBool(config,
                            ImsConfig.ConfigConstants.VLT_SETTING_ENABLED));
                    if (DBG) Rlog.d(TAG, "isVoLteProvisioned = " + isVolteProvisioned());

                    setWfcProvisionedProperty(getProvisionedBool(config,
                            ImsConfig.ConfigConstants.VOICE_OVER_WIFI_SETTING_ENABLED));
                    if (DBG) Rlog.d(TAG, "isWfcProvisioned = " + isWfcProvisioned());

                    setVtProvisionedProperty(getProvisionedBool(config,
                            ImsConfig.ConfigConstants.LVC_SETTING_ENABLED));
                    if (DBG) Rlog.d(TAG, "isVtProvisioned = " + isVtProvisioned());

                }
            } catch (ImsException ie) {
                Rlog.e(TAG, "AsyncUpdateProvisionedValues error: ", ie);
            }

            return null;
        }

        private boolean getProvisionedBool(ImsConfig config, int item) throws ImsException {
            return config.getProvisionedValue(item) == ImsConfig.FeatureValueConstants.ON;
        }
    }

    /** Asynchronously get VoLTE, WFC, VT provisioning statuses */
    private void updateProvisionedValues() {
        if (getBooleanCarrierConfig(mContext,
                CarrierConfigManager.KEY_CARRIER_VOLTE_PROVISIONING_REQUIRED_BOOL)) {

            new AsyncUpdateProvisionedValues().execute();
        }
    }

    /**
     * Sync carrier config and user settings with ImsConfig.
     *
     * @param context for the manager object
     * @param phoneId phone id
     * @param force update
     */
    public static void updateImsServiceConfig(Context context, int phoneId, boolean force) {
        if (!force) {
            if (TelephonyManager.getDefault().getSimState() != TelephonyManager.SIM_STATE_READY) {
                log("updateImsServiceConfig: SIM not ready");
                // Don't disable IMS if SIM is not ready
                return;
            }
        }

        final ImsManager imsManager = ImsManager.getInstance(context, phoneId);
        if (imsManager != null && (!imsManager.mConfigUpdated || force)) {
            try {
                imsManager.updateProvisionedValues();

                // TODO: Extend ImsConfig API and set all feature values in single function call.

                // Note: currently the order of updates is set to produce different order of
                // setFeatureValue() function calls from setAdvanced4GMode(). This is done to
                // differentiate this code path from vendor code perspective.
                boolean isImsUsed = imsManager.updateVolteFeatureValue();
                isImsUsed |= imsManager.updateWfcFeatureAndProvisionedValues();
                isImsUsed |= imsManager.updateVideoCallFeatureValue();

                if (isImsUsed || !isTurnOffImsAllowedByPlatform(context)) {
                    // Turn on IMS if it is used.
                    // Also, if turning off is not allowed for current carrier,
                    // we need to turn IMS on because it might be turned off before
                    // phone switched to current carrier.
                    log("updateImsServiceConfig: turnOnIms");
                    imsManager.turnOnIms();
                } else {
                    // Turn off IMS if it is not used AND turning off is allowed for carrier.
                    log("updateImsServiceConfig: turnOffIms");
                    imsManager.turnOffIms();
                }

                imsManager.mConfigUpdated = true;
            } catch (ImsException e) {
                loge("updateImsServiceConfig: ", e);
                imsManager.mConfigUpdated = false;
            }
        }
    }

    /**
     * Update VoLTE config
     * @return whether feature is On
     * @throws ImsException
     */
    private boolean updateVolteFeatureValue() throws ImsException {
        boolean available = isVolteEnabledByPlatform(mContext);
        boolean enabled = isEnhanced4gLteModeSettingEnabledByUser(mContext);
        boolean isNonTty = isNonTtyOrTtyOnVolteEnabled(mContext);
        boolean isFeatureOn = available && enabled && isNonTty;

        log("updateVolteFeatureValue: available = " + available
                + ", enabled = " + enabled
                + ", nonTTY = " + isNonTty);

        getConfigInterface().setFeatureValue(
                ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE,
                TelephonyManager.NETWORK_TYPE_LTE,
                isFeatureOn ?
                        ImsConfig.FeatureValueConstants.ON :
                        ImsConfig.FeatureValueConstants.OFF,
                mImsConfigListener);

        return isFeatureOn;
    }

    /**
     * Update video call over LTE config
     * @return whether feature is On
     * @throws ImsException
     */
    private boolean updateVideoCallFeatureValue() throws ImsException {
        boolean available = isVtEnabledByPlatform(mContext);
        boolean enabled = isVtEnabledByUser(mContext);
        boolean isNonTty = isNonTtyOrTtyOnVolteEnabled(mContext);
        boolean isDataEnabled = isDataEnabled();

        boolean isFeatureOn = available && enabled && isNonTty && isDataEnabled;

        log("updateVideoCallFeatureValue: available = " + available
                + ", enabled = " + enabled
                + ", nonTTY = " + isNonTty
                + ", data enabled = " + isDataEnabled);

        getConfigInterface().setFeatureValue(
                ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE,
                TelephonyManager.NETWORK_TYPE_LTE,
                isFeatureOn ?
                        ImsConfig.FeatureValueConstants.ON :
                        ImsConfig.FeatureValueConstants.OFF,
                mImsConfigListener);

        return isFeatureOn;
    }

    /**
     * Update WFC config
     * @return whether feature is On
     * @throws ImsException
     */
    private boolean updateWfcFeatureAndProvisionedValues() throws ImsException {
        boolean isNetworkRoaming = TelephonyManager.getDefault().isNetworkRoaming();
        boolean available = isWfcEnabledByPlatform(mContext);
        boolean enabled = isWfcEnabledByUser(mContext);
        int mode = getWfcMode(mContext, isNetworkRoaming);
        boolean roaming = isWfcRoamingEnabledByUser(mContext);
        boolean isFeatureOn = available && enabled;

        log("updateWfcFeatureAndProvisionedValues: available = " + available
                + ", enabled = " + enabled
                + ", mode = " + mode
                + ", roaming = " + roaming);

        getConfigInterface().setFeatureValue(
                ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI,
                TelephonyManager.NETWORK_TYPE_IWLAN,
                isFeatureOn ?
                        ImsConfig.FeatureValueConstants.ON :
                        ImsConfig.FeatureValueConstants.OFF,
                mImsConfigListener);

        if (!isFeatureOn) {
            mode = ImsConfig.WfcModeFeatureValueConstants.CELLULAR_PREFERRED;
            roaming = false;
        }
        setWfcModeInternal(mContext, mode);
        setWfcRoamingSettingInternal(mContext, roaming);

        return isFeatureOn;
    }

    private ImsManager(Context context, int phoneId) {
        mContext = context;
        mPhoneId = phoneId;
        createImsService(true);
    }

    /*
     * Returns a flag indicating whether the IMS service is available.
     */
    public boolean isServiceAvailable() {
        if (mImsService != null) {
            return true;
        }

        IBinder binder = ServiceManager.checkService(getImsServiceName(mPhoneId));
        if (binder != null) {
            return true;
        }

        return false;
    }

    public void setImsConfigListener(ImsConfigListener listener) {
        mImsConfigListener = listener;
    }

    /**
     * Opens the IMS service for making calls and/or receiving generic IMS calls.
     * The caller may make subsquent calls through {@link #makeCall}.
     * The IMS service will register the device to the operator's network with the credentials
     * (from ISIM) periodically in order to receive calls from the operator's network.
     * When the IMS service receives a new call, it will send out an intent with
     * the provided action string.
     * The intent contains a call ID extra {@link getCallId} and it can be used to take a call.
     *
     * @param serviceClass a service class specified in {@link ImsServiceClass}
     *      For VoLTE service, it MUST be a {@link ImsServiceClass#MMTEL}.
     * @param incomingCallPendingIntent When an incoming call is received,
     *        the IMS service will call {@link PendingIntent#send(Context, int, Intent)} to
     *        send back the intent to the caller with {@link #INCOMING_CALL_RESULT_CODE}
     *        as the result code and the intent to fill in the call ID; It cannot be null
     * @param listener To listen to IMS registration events; It cannot be null
     * @return identifier (greater than 0) for the specified service
     * @throws NullPointerException if {@code incomingCallPendingIntent}
     *      or {@code listener} is null
     * @throws ImsException if calling the IMS service results in an error
     * @see #getCallId
     * @see #getServiceId
     */
    public int open(int serviceClass, PendingIntent incomingCallPendingIntent,
            ImsConnectionStateListener listener) throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        if (incomingCallPendingIntent == null) {
            throw new NullPointerException("incomingCallPendingIntent can't be null");
        }

        if (listener == null) {
            throw new NullPointerException("listener can't be null");
        }

        int result = 0;

        try {
            result = mImsService.open(mPhoneId, serviceClass, incomingCallPendingIntent,
                    createRegistrationListenerProxy(serviceClass, listener));
        } catch (RemoteException e) {
            throw new ImsException("open()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }

        if (result <= 0) {
            // If the return value is a minus value,
            // it means that an error occurred in the service.
            // So, it needs to convert to the reason code specified in ImsReasonInfo.
            throw new ImsException("open()", (result * (-1)));
        }

        return result;
    }

    /**
     * Adds registration listener to the IMS service.
     *
     * @param serviceClass a service class specified in {@link ImsServiceClass}
     *      For VoLTE service, it MUST be a {@link ImsServiceClass#MMTEL}.
     * @param listener To listen to IMS registration events; It cannot be null
     * @throws NullPointerException if {@code listener} is null
     * @throws ImsException if calling the IMS service results in an error
     */
    public void addRegistrationListener(int serviceClass, ImsConnectionStateListener listener)
            throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        if (listener == null) {
            throw new NullPointerException("listener can't be null");
        }

        try {
            mImsService.addRegistrationListener(mPhoneId, serviceClass,
                    createRegistrationListenerProxy(serviceClass, listener));
        } catch (RemoteException e) {
            throw new ImsException("addRegistrationListener()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }

    /**
     * Closes the specified service ({@link ImsServiceClass}) not to make/receive calls.
     * All the resources that were allocated to the service are also released.
     *
     * @param serviceId a service id to be closed which is obtained from {@link ImsManager#open}
     * @throws ImsException if calling the IMS service results in an error
     */
    public void close(int serviceId) throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        try {
            mImsService.close(serviceId);
        } catch (RemoteException e) {
            throw new ImsException("close()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        } finally {
            mUt = null;
            mConfig = null;
            mEcbm = null;
            mMultiEndpoint = null;
        }
    }

    /**
     * Gets the configuration interface to provision / withdraw the supplementary service settings.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @return the Ut interface instance
     * @throws ImsException if getting the Ut interface results in an error
     */
    public ImsUtInterface getSupplementaryServiceConfiguration(int serviceId)
            throws ImsException {
        // FIXME: manage the multiple Ut interfaces based on the service id
        if (mUt == null) {
            checkAndThrowExceptionIfServiceUnavailable();

            try {
                IImsUt iUt = mImsService.getUtInterface(serviceId);

                if (iUt == null) {
                    throw new ImsException("getSupplementaryServiceConfiguration()",
                            ImsReasonInfo.CODE_UT_NOT_SUPPORTED);
                }

                mUt = new ImsUt(iUt);
            } catch (RemoteException e) {
                throw new ImsException("getSupplementaryServiceConfiguration()", e,
                        ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
            }
        }

        return mUt;
    }

    /**
     * Checks if the IMS service has successfully registered to the IMS network
     * with the specified service & call type.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @param serviceType a service type that is specified in {@link ImsCallProfile}
     *        {@link ImsCallProfile#SERVICE_TYPE_NORMAL}
     *        {@link ImsCallProfile#SERVICE_TYPE_EMERGENCY}
     * @param callType a call type that is specified in {@link ImsCallProfile}
     *        {@link ImsCallProfile#CALL_TYPE_VOICE_N_VIDEO}
     *        {@link ImsCallProfile#CALL_TYPE_VOICE}
     *        {@link ImsCallProfile#CALL_TYPE_VT}
     *        {@link ImsCallProfile#CALL_TYPE_VS}
     * @return true if the specified service id is connected to the IMS network;
     *        false otherwise
     * @throws ImsException if calling the IMS service results in an error
     */
    public boolean isConnected(int serviceId, int serviceType, int callType)
            throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        try {
            return mImsService.isConnected(serviceId, serviceType, callType);
        } catch (RemoteException e) {
            throw new ImsException("isServiceConnected()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }

    /**
     * Checks if the specified IMS service is opend.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @return true if the specified service id is opened; false otherwise
     * @throws ImsException if calling the IMS service results in an error
     */
    public boolean isOpened(int serviceId) throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        try {
            return mImsService.isOpened(serviceId);
        } catch (RemoteException e) {
            throw new ImsException("isOpened()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }

    /**
     * Creates a {@link ImsCallProfile} from the service capabilities & IMS registration state.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @param serviceType a service type that is specified in {@link ImsCallProfile}
     *        {@link ImsCallProfile#SERVICE_TYPE_NONE}
     *        {@link ImsCallProfile#SERVICE_TYPE_NORMAL}
     *        {@link ImsCallProfile#SERVICE_TYPE_EMERGENCY}
     * @param callType a call type that is specified in {@link ImsCallProfile}
     *        {@link ImsCallProfile#CALL_TYPE_VOICE}
     *        {@link ImsCallProfile#CALL_TYPE_VT}
     *        {@link ImsCallProfile#CALL_TYPE_VT_TX}
     *        {@link ImsCallProfile#CALL_TYPE_VT_RX}
     *        {@link ImsCallProfile#CALL_TYPE_VT_NODIR}
     *        {@link ImsCallProfile#CALL_TYPE_VS}
     *        {@link ImsCallProfile#CALL_TYPE_VS_TX}
     *        {@link ImsCallProfile#CALL_TYPE_VS_RX}
     * @return a {@link ImsCallProfile} object
     * @throws ImsException if calling the IMS service results in an error
     */
    public ImsCallProfile createCallProfile(int serviceId,
            int serviceType, int callType) throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        try {
            return mImsService.createCallProfile(serviceId, serviceType, callType);
        } catch (RemoteException e) {
            throw new ImsException("createCallProfile()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }

    /**
     * Creates a {@link ImsCall} to make a call.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @param profile a call profile to make the call
     *      (it contains service type, call type, media information, etc.)
     * @param participants participants to invite the conference call
     * @param listener listen to the call events from {@link ImsCall}
     * @return a {@link ImsCall} object
     * @throws ImsException if calling the IMS service results in an error
     */
    public ImsCall makeCall(int serviceId, ImsCallProfile profile, String[] callees,
            ImsCall.Listener listener) throws ImsException {
        if (DBG) {
            log("makeCall :: serviceId=" + serviceId
                    + ", profile=" + profile);
        }

        checkAndThrowExceptionIfServiceUnavailable();

        ImsCall call = new ImsCall(mContext, profile);

        call.setListener(listener);
        ImsCallSession session = createCallSession(serviceId, profile);

        if ((callees != null) && (callees.length == 1)) {
            call.start(session, callees[0]);
        } else {
            call.start(session, callees);
        }

        return call;
    }

    /**
     * Creates a {@link ImsCall} to take an incoming call.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @param incomingCallIntent the incoming call broadcast intent
     * @param listener to listen to the call events from {@link ImsCall}
     * @return a {@link ImsCall} object
     * @throws ImsException if calling the IMS service results in an error
     */
    public ImsCall takeCall(int serviceId, Intent incomingCallIntent,
            ImsCall.Listener listener) throws ImsException {
        if (DBG) {
            log("takeCall :: serviceId=" + serviceId
                    + ", incomingCall=" + incomingCallIntent);
        }

        checkAndThrowExceptionIfServiceUnavailable();

        if (incomingCallIntent == null) {
            throw new ImsException("Can't retrieve session with null intent",
                    ImsReasonInfo.CODE_LOCAL_ILLEGAL_ARGUMENT);
        }

        int incomingServiceId = getServiceId(incomingCallIntent);

        if (serviceId != incomingServiceId) {
            throw new ImsException("Service id is mismatched in the incoming call intent",
                    ImsReasonInfo.CODE_LOCAL_ILLEGAL_ARGUMENT);
        }

        String callId = getCallId(incomingCallIntent);

        if (callId == null) {
            throw new ImsException("Call ID missing in the incoming call intent",
                    ImsReasonInfo.CODE_LOCAL_ILLEGAL_ARGUMENT);
        }

        try {
            IImsCallSession session = mImsService.getPendingCallSession(serviceId, callId);

            if (session == null) {
                throw new ImsException("No pending session for the call",
                        ImsReasonInfo.CODE_LOCAL_NO_PENDING_CALL);
            }

            ImsCall call = new ImsCall(mContext, session.getCallProfile());

            call.attachSession(new ImsCallSession(session));
            call.setListener(listener);

            return call;
        } catch (Throwable t) {
            throw new ImsException("takeCall()", t, ImsReasonInfo.CODE_UNSPECIFIED);
        }
    }

    /**
     * Gets the config interface to get/set service/capability parameters.
     *
     * @return the ImsConfig instance.
     * @throws ImsException if getting the setting interface results in an error.
     */
    public ImsConfig getConfigInterface() throws ImsException {

        if (mConfig == null) {
            checkAndThrowExceptionIfServiceUnavailable();

            try {
                IImsConfig config = mImsService.getConfigInterface(mPhoneId);
                if (config == null) {
                    throw new ImsException("getConfigInterface()",
                            ImsReasonInfo.CODE_LOCAL_SERVICE_UNAVAILABLE);
                }
                mConfig = new ImsConfig(config, mContext);
            } catch (RemoteException e) {
                throw new ImsException("getConfigInterface()", e,
                        ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
            }
        }
        if (DBG) log("getConfigInterface(), mConfig= " + mConfig);
        return mConfig;
    }

    public void setUiTTYMode(Context context, int serviceId, int uiTtyMode, Message onComplete)
            throws ImsException {

        checkAndThrowExceptionIfServiceUnavailable();

        try {
            mImsService.setUiTTYMode(serviceId, uiTtyMode, onComplete);
        } catch (RemoteException e) {
            throw new ImsException("setTTYMode()", e,
                    ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }

        if (!getBooleanCarrierConfig(context,
                CarrierConfigManager.KEY_CARRIER_VOLTE_TTY_SUPPORTED_BOOL)) {
            setAdvanced4GMode((uiTtyMode == TelecomManager.TTY_MODE_OFF) &&
                    isEnhanced4gLteModeSettingEnabledByUser(context));
        }
    }

    /**
     * Get the boolean config from carrier config manager.
     *
     * @param context the context to get carrier service
     * @param key config key defined in CarrierConfigManager
     * @return boolean value of corresponding key.
     */
    private static boolean getBooleanCarrierConfig(Context context, String key) {
        CarrierConfigManager configManager = (CarrierConfigManager) context.getSystemService(
                Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle b = null;
        if (configManager != null) {
            b = configManager.getConfig();
        }
        if (b != null) {
            return b.getBoolean(key);
        } else {
            // Return static default defined in CarrierConfigManager.
            return CarrierConfigManager.getDefaultConfig().getBoolean(key);
        }
    }

    /**
     * Get the int config from carrier config manager.
     *
     * @param context the context to get carrier service
     * @param key config key defined in CarrierConfigManager
     * @return integer value of corresponding key.
     */
    private static int getIntCarrierConfig(Context context, String key) {
        CarrierConfigManager configManager = (CarrierConfigManager) context.getSystemService(
                Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle b = null;
        if (configManager != null) {
            b = configManager.getConfig();
        }
        if (b != null) {
            return b.getInt(key);
        } else {
            // Return static default defined in CarrierConfigManager.
            return CarrierConfigManager.getDefaultConfig().getInt(key);
        }
    }

    /**
     * Gets the call ID from the specified incoming call broadcast intent.
     *
     * @param incomingCallIntent the incoming call broadcast intent
     * @return the call ID or null if the intent does not contain it
     */
    private static String getCallId(Intent incomingCallIntent) {
        if (incomingCallIntent == null) {
            return null;
        }

        return incomingCallIntent.getStringExtra(EXTRA_CALL_ID);
    }

    /**
     * Gets the service type from the specified incoming call broadcast intent.
     *
     * @param incomingCallIntent the incoming call broadcast intent
     * @return the service identifier or -1 if the intent does not contain it
     */
    private static int getServiceId(Intent incomingCallIntent) {
        if (incomingCallIntent == null) {
            return (-1);
        }

        return incomingCallIntent.getIntExtra(EXTRA_SERVICE_ID, -1);
    }

    /**
     * Binds the IMS service only if the service is not created.
     */
    private void checkAndThrowExceptionIfServiceUnavailable()
            throws ImsException {
        if (mImsService == null) {
            createImsService(true);

            if (mImsService == null) {
                throw new ImsException("Service is unavailable",
                        ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
            }
        }
    }

    private static String getImsServiceName(int phoneId) {
        // TODO: MSIM implementation needs to decide on service name as a function of phoneId
        return IMS_SERVICE;
    }

    /**
     * Binds the IMS service to make/receive the call.
     */
    private void createImsService(boolean checkService) {
        if (checkService) {
            IBinder binder = ServiceManager.checkService(getImsServiceName(mPhoneId));

            if (binder == null) {
                return;
            }
        }

        IBinder b = ServiceManager.getService(getImsServiceName(mPhoneId));

        if (b != null) {
            try {
                b.linkToDeath(mDeathRecipient, 0);
            } catch (RemoteException e) {
            }
        }

        mImsService = IImsService.Stub.asInterface(b);
    }

    /**
     * Creates a {@link ImsCallSession} with the specified call profile.
     * Use other methods, if applicable, instead of interacting with
     * {@link ImsCallSession} directly.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @param profile a call profile to make the call
     */
    private ImsCallSession createCallSession(int serviceId,
            ImsCallProfile profile) throws ImsException {
        try {
            return new ImsCallSession(mImsService.createCallSession(serviceId, profile, null));
        } catch (RemoteException e) {
            return null;
        }
    }

    private ImsRegistrationListenerProxy createRegistrationListenerProxy(int serviceClass,
            ImsConnectionStateListener listener) {
        ImsRegistrationListenerProxy proxy =
                new ImsRegistrationListenerProxy(serviceClass, listener);
        return proxy;
    }

    private static void log(String s) {
        Rlog.d(TAG, s);
    }

    private static void loge(String s) {
        Rlog.e(TAG, s);
    }

    private static void loge(String s, Throwable t) {
        Rlog.e(TAG, s, t);
    }

    /**
     * Used for turning on IMS.if its off already
     */
    private void turnOnIms() throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        try {
            mImsService.turnOnIms(mPhoneId);
        } catch (RemoteException e) {
            throw new ImsException("turnOnIms() ", e, ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }

    private boolean isImsTurnOffAllowed() {
        return isTurnOffImsAllowedByPlatform(mContext)
                && (!isWfcEnabledByPlatform(mContext)
                || !isWfcEnabledByUser(mContext));
    }

    private void setLteFeatureValues(boolean turnOn) {
        log("setLteFeatureValues: " + turnOn);
        try {
            ImsConfig config = getConfigInterface();
            if (config != null) {
                config.setFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE,
                        TelephonyManager.NETWORK_TYPE_LTE, turnOn ? 1 : 0, mImsConfigListener);

                if (isVtEnabledByPlatform(mContext)) {
                    boolean enableViLte = turnOn && isVtEnabledByUser(mContext) &&
                            isDataEnabled();
                    config.setFeatureValue(ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE,
                            TelephonyManager.NETWORK_TYPE_LTE,
                            enableViLte ? 1 : 0,
                            mImsConfigListener);
                }
            }
        } catch (ImsException e) {
            loge("setLteFeatureValues: exception ", e);
        }
    }

    private void setAdvanced4GMode(boolean turnOn) throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        // if turnOn: first set feature values then call turnOnIms()
        // if turnOff: only set feature values if IMS turn off is not allowed. If turn off is
        // allowed, first call turnOffIms() then set feature values
        if (turnOn) {
            setLteFeatureValues(turnOn);
            log("setAdvanced4GMode: turnOnIms");
            turnOnIms();
        } else {
            if (isImsTurnOffAllowed()) {
                log("setAdvanced4GMode: turnOffIms");
                turnOffIms();
            }
            setLteFeatureValues(turnOn);
        }
    }

    /**
     * Used for turning off IMS completely in order to make the device CSFB'ed.
     * Once turned off, all calls will be over CS.
     */
    private void turnOffIms() throws ImsException {
        checkAndThrowExceptionIfServiceUnavailable();

        try {
            mImsService.turnOffIms(mPhoneId);
        } catch (RemoteException e) {
            throw new ImsException("turnOffIms() ", e, ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
        }
    }

    /**
     * Death recipient class for monitoring IMS service.
     */
    private class ImsServiceDeathRecipient implements IBinder.DeathRecipient {
        @Override
        public void binderDied() {
            mImsService = null;
            mUt = null;
            mConfig = null;
            mEcbm = null;
            mMultiEndpoint = null;

            if (mContext != null) {
                Intent intent = new Intent(ACTION_IMS_SERVICE_DOWN);
                intent.putExtra(EXTRA_PHONE_ID, mPhoneId);
                mContext.sendBroadcast(new Intent(intent));
            }
        }
    }

    /**
     * Adapter class for {@link IImsRegistrationListener}.
     */
    private class ImsRegistrationListenerProxy extends IImsRegistrationListener.Stub {
        private int mServiceClass;
        private ImsConnectionStateListener mListener;

        public ImsRegistrationListenerProxy(int serviceClass,
                ImsConnectionStateListener listener) {
            mServiceClass = serviceClass;
            mListener = listener;
        }

        public boolean isSameProxy(int serviceClass) {
            return (mServiceClass == serviceClass);
        }

        @Deprecated
        public void registrationConnected() {
            if (DBG) {
                log("registrationConnected ::");
            }

            if (mListener != null) {
                mListener.onImsConnected();
            }
        }

        @Deprecated
        public void registrationProgressing() {
            if (DBG) {
                log("registrationProgressing ::");
            }

            if (mListener != null) {
                mListener.onImsProgressing();
            }
        }

        @Override
        public void registrationConnectedWithRadioTech(int imsRadioTech) {
            // Note: imsRadioTech value maps to RIL_RADIO_TECHNOLOGY
            //       values in ServiceState.java.
            if (DBG) {
                log("registrationConnectedWithRadioTech :: imsRadioTech=" + imsRadioTech);
            }

            if (mListener != null) {
                mListener.onImsConnected();
            }
        }

        @Override
        public void registrationProgressingWithRadioTech(int imsRadioTech) {
            // Note: imsRadioTech value maps to RIL_RADIO_TECHNOLOGY
            //       values in ServiceState.java.
            if (DBG) {
                log("registrationProgressingWithRadioTech :: imsRadioTech=" + imsRadioTech);
            }

            if (mListener != null) {
                mListener.onImsProgressing();
            }
        }

        @Override
        public void registrationDisconnected(ImsReasonInfo imsReasonInfo) {
            if (DBG) {
                log("registrationDisconnected :: imsReasonInfo" + imsReasonInfo);
            }

            if (mListener != null) {
                mListener.onImsDisconnected(imsReasonInfo);
            }
        }

        @Override
        public void registrationResumed() {
            if (DBG) {
                log("registrationResumed ::");
            }

            if (mListener != null) {
                mListener.onImsResumed();
            }
        }

        @Override
        public void registrationSuspended() {
            if (DBG) {
                log("registrationSuspended ::");
            }

            if (mListener != null) {
                mListener.onImsSuspended();
            }
        }

        @Override
        public void registrationServiceCapabilityChanged(int serviceClass, int event) {
            log("registrationServiceCapabilityChanged :: serviceClass=" +
                    serviceClass + ", event=" + event);

            if (mListener != null) {
                mListener.onImsConnected();
            }
        }

        @Override
        public void registrationFeatureCapabilityChanged(int serviceClass,
                int[] enabledFeatures, int[] disabledFeatures) {
            log("registrationFeatureCapabilityChanged :: serviceClass=" +
                    serviceClass);
            if (mListener != null) {
                mListener.onFeatureCapabilityChanged(serviceClass,
                        enabledFeatures, disabledFeatures);
            }
        }

        @Override
        public void voiceMessageCountUpdate(int count) {
            log("voiceMessageCountUpdate :: count=" + count);

            if (mListener != null) {
                mListener.onVoiceMessageCountChanged(count);
            }
        }

        @Override
        public void registrationAssociatedUriChanged(Uri[] uris) {
            if (DBG) log("registrationAssociatedUriChanged ::");

            if (mListener != null) {
                mListener.registrationAssociatedUriChanged(uris);
            }
        }
    }

    /**
     * Gets the ECBM interface to request ECBM exit.
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @return the ECBM interface instance
     * @throws ImsException if getting the ECBM interface results in an error
     */
    public ImsEcbm getEcbmInterface(int serviceId) throws ImsException {
        if (mEcbm == null) {
            checkAndThrowExceptionIfServiceUnavailable();

            try {
                IImsEcbm iEcbm = mImsService.getEcbmInterface(serviceId);

                if (iEcbm == null) {
                    throw new ImsException("getEcbmInterface()",
                            ImsReasonInfo.CODE_ECBM_NOT_SUPPORTED);
                }
                mEcbm = new ImsEcbm(iEcbm);
            } catch (RemoteException e) {
                throw new ImsException("getEcbmInterface()", e,
                        ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
            }
        }
        return mEcbm;
    }

    /**
     * Gets the Multi-Endpoint interface to subscribe to multi-enpoint notifications..
     *
     * @param serviceId a service id which is obtained from {@link ImsManager#open}
     * @return the multi-endpoint interface instance
     * @throws ImsException if getting the multi-endpoint interface results in an error
     */
    public ImsMultiEndpoint getMultiEndpointInterface(int serviceId) throws ImsException {
        if (mMultiEndpoint == null) {
            checkAndThrowExceptionIfServiceUnavailable();

            try {
                IImsMultiEndpoint iImsMultiEndpoint = mImsService.getMultiEndpointInterface(
                        serviceId);

                if (iImsMultiEndpoint == null) {
                    throw new ImsException("getMultiEndpointInterface()",
                            ImsReasonInfo.CODE_MULTIENDPOINT_NOT_SUPPORTED);
                }
                mMultiEndpoint = new ImsMultiEndpoint(iImsMultiEndpoint);
            } catch (RemoteException e) {
                throw new ImsException("getMultiEndpointInterface()", e,
                        ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN);
            }
        }
        return mMultiEndpoint;
    }

    /**
     * Resets ImsManager settings back to factory defaults.
     *
     * @hide
     */
    public static void factoryReset(Context context) {
        // Set VoLTE to default
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.ENHANCED_4G_MODE_ENABLED,
                ImsConfig.FeatureValueConstants.ON);

        // Set VoWiFi to default
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_ENABLED,
                getBooleanCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_ENABLED_BOOL) ?
                        ImsConfig.FeatureValueConstants.ON : ImsConfig.FeatureValueConstants.OFF);

        // Set VoWiFi mode to default
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_MODE,
                getIntCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_MODE_INT));

        // Set VoWiFi roaming to default
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.WFC_IMS_ROAMING_ENABLED,
                getBooleanCarrierConfig(context,
                        CarrierConfigManager.KEY_CARRIER_DEFAULT_WFC_IMS_ROAMING_ENABLED_BOOL) ?
                        ImsConfig.FeatureValueConstants.ON : ImsConfig.FeatureValueConstants.OFF);

        // Set VT to default
        android.provider.Settings.Global.putInt(context.getContentResolver(),
                android.provider.Settings.Global.VT_IMS_ENABLED,
                ImsConfig.FeatureValueConstants.ON);

        // Push settings to ImsConfig
        ImsManager.updateImsServiceConfig(context,
                SubscriptionManager.getDefaultVoicePhoneId(), true);
    }

    private boolean isDataEnabled() {
        return SystemProperties.getBoolean(DATA_ENABLED_PROP, true);
    }

    /**
     * Set data enabled/disabled flag.
     * @param enabled True if data is enabled, otherwise disabled.
     */
    public void setDataEnabled(boolean enabled) {
        log("setDataEnabled: " + enabled);
        SystemProperties.set(DATA_ENABLED_PROP, enabled ? TRUE : FALSE);
    }

    private boolean isVolteProvisioned() {
        return SystemProperties.getBoolean(VOLTE_PROVISIONED_PROP, true);
    }

    private void setVolteProvisionedProperty(boolean provisioned) {
        SystemProperties.set(VOLTE_PROVISIONED_PROP, provisioned ? TRUE : FALSE);
    }

    private boolean isWfcProvisioned() {
        return SystemProperties.getBoolean(WFC_PROVISIONED_PROP, true);
    }

    private void setWfcProvisionedProperty(boolean provisioned) {
        SystemProperties.set(WFC_PROVISIONED_PROP, provisioned ? TRUE : FALSE);
    }

    private boolean isVtProvisioned() {
        return SystemProperties.getBoolean(VT_PROVISIONED_PROP, true);
    }

    private void setVtProvisionedProperty(boolean provisioned) {
        SystemProperties.set(VT_PROVISIONED_PROP, provisioned ? TRUE : FALSE);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("ImsManager:");
        pw.println("  mPhoneId = " + mPhoneId);
        pw.println("  mConfigUpdated = " + mConfigUpdated);
        pw.println("  mImsService = " + mImsService);
        pw.println("  mDataEnabled = " + isDataEnabled());

        pw.println("  isGbaValid = " + isGbaValid(mContext));
        pw.println("  isImsTurnOffAllowed = " + isImsTurnOffAllowed());
        pw.println("  isNonTtyOrTtyOnVolteEnabled = " + isNonTtyOrTtyOnVolteEnabled(mContext));

        pw.println("  isVolteEnabledByPlatform = " + isVolteEnabledByPlatform(mContext));
        pw.println("  isVolteProvisionedOnDevice = " + isVolteProvisionedOnDevice(mContext));
        pw.println("  isEnhanced4gLteModeSettingEnabledByUser = " +
                isEnhanced4gLteModeSettingEnabledByUser(mContext));
        pw.println("  isVtEnabledByPlatform = " + isVtEnabledByPlatform(mContext));
        pw.println("  isVtEnabledByUser = " + isVtEnabledByUser(mContext));

        pw.println("  isWfcEnabledByPlatform = " + isWfcEnabledByPlatform(mContext));
        pw.println("  isWfcEnabledByUser = " + isWfcEnabledByUser(mContext));
        pw.println("  getWfcMode = " + getWfcMode(mContext));
        pw.println("  isWfcRoamingEnabledByUser = " + isWfcRoamingEnabledByUser(mContext));

        pw.println("  isVtProvisionedOnDevice = " + isVtProvisionedOnDevice(mContext));
        pw.println("  isWfcProvisionedOnDevice = " + isWfcProvisionedOnDevice(mContext));
        pw.flush();
    }
}
