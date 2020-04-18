/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.internal.telephony.metrics;

import android.os.SystemClock;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.TelephonyHistogram;
import android.util.Base64;
import android.util.SparseArray;

import com.android.ims.ImsConfig;
import com.android.ims.ImsReasonInfo;
import com.android.ims.internal.ImsCallSession;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.RIL;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.SmsResponse;
import com.android.internal.telephony.TelephonyProto;
import com.android.internal.telephony.TelephonyProto.ImsCapabilities;
import com.android.internal.telephony.TelephonyProto.ImsConnectionState;
import com.android.internal.telephony.TelephonyProto.RilDataCall;
import com.android.internal.telephony.TelephonyProto.SmsSession;
import com.android.internal.telephony.TelephonyProto.TelephonyCallSession;
import com.android.internal.telephony.TelephonyProto.TelephonyEvent;
import com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilDeactivateDataCall;
import com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilSetupDataCall;
import com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilSetupDataCallResponse;
import com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilSetupDataCallResponse.RilDataCallFailCause;
import com.android.internal.telephony.TelephonyProto.TelephonyLog;
import com.android.internal.telephony.TelephonyProto.TelephonyServiceState;
import com.android.internal.telephony.TelephonyProto.TelephonySettings;
import com.android.internal.telephony.TelephonyProto.TimeInterval;
import com.android.internal.telephony.UUSInfo;
import com.android.internal.telephony.dataconnection.DataCallResponse;
import com.android.internal.telephony.imsphone.ImsPhoneCall;
import com.android.internal.util.IndentingPrintWriter;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;

import static android.text.format.DateUtils.MINUTE_IN_MILLIS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ANSWER;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_CDMA_SEND_SMS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_DEACTIVATE_DATA_CALL;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_DIAL;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_HANGUP;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_IMS_SEND_SMS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SEND_SMS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SEND_SMS_EXPECT_MORE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SETUP_DATA_CALL;
import static com.android.internal.telephony.TelephonyProto.PdpType.PDP_TYPE_IP;
import static com.android.internal.telephony.TelephonyProto.PdpType.PDP_TYPE_IPV4V6;
import static com.android.internal.telephony.TelephonyProto.PdpType.PDP_TYPE_IPV6;
import static com.android.internal.telephony.TelephonyProto.PdpType.PDP_TYPE_PPP;
import static com.android.internal.telephony.TelephonyProto.PdpType.PDP_UNKNOWN;

/**
 * Telephony metrics holds all metrics events and convert it into telephony proto buf.
 * @hide
 */
public class TelephonyMetrics {

    private static final String TAG = TelephonyMetrics.class.getSimpleName();

    private static final boolean DBG = true;
    private static final boolean VDBG = false; // STOPSHIP if true

    /** Maximum telephony events stored */
    private static final int MAX_TELEPHONY_EVENTS = 1000;

    /** Maximum call sessions stored */
    private static final int MAX_COMPLETED_CALL_SESSIONS = 50;

    /** Maximum sms sessions stored */
    private static final int MAX_COMPLETED_SMS_SESSIONS = 500;

    /** For reducing the timing precision for privacy purposes */
    private static final int SESSION_START_PRECISION_MINUTES = 5;

    /** The TelephonyMetrics singleton instance */
    private static TelephonyMetrics sInstance;

    /** Telephony events */
    private final Deque<TelephonyEvent> mTelephonyEvents = new ArrayDeque<>();

    /**
     * In progress call sessions. Note that each phone can only have up to 1 in progress call
     * session (might contains multiple calls). Having a sparse array in case we need to support
     * DSDA in the future.
     */
    private final SparseArray<InProgressCallSession> mInProgressCallSessions = new SparseArray<>();

    /** The completed call sessions */
    private final Deque<TelephonyCallSession> mCompletedCallSessions = new ArrayDeque<>();

    /** The in-progress SMS sessions. When finished, it will be moved into the completed sessions */
    private final SparseArray<InProgressSmsSession> mInProgressSmsSessions = new SparseArray<>();

    /** The completed SMS sessions */
    private final Deque<SmsSession> mCompletedSmsSessions = new ArrayDeque<>();

    /** Last service state. This is for injecting the base of a new log or a new call/sms session */
    private final SparseArray<TelephonyServiceState> mLastServiceState = new SparseArray<>();

    /**
     * Last ims capabilities. This is for injecting the base of a new log or a new call/sms
     * session
     */
    private final SparseArray<ImsCapabilities> mLastImsCapabilities = new SparseArray<>();

    /**
     * Last IMS connection state. This is for injecting the base of a new log or a new call/sms
     * session
     */
    private final SparseArray<ImsConnectionState> mLastImsConnectionState = new SparseArray<>();

    /** The start system time of the TelephonyLog in milliseconds*/
    private long mStartSystemTimeMs;

    /** The start elapsed time of the TelephonyLog in milliseconds*/
    private long mStartElapsedTimeMs;

    /** Indicating if some of the telephony events are dropped in this log */
    private boolean mTelephonyEventsDropped = false;

    public TelephonyMetrics() {
        reset();
    }

    /**
     * Get the singleton instance of telephony metrics.
     *
     * @return The instance
     */
    public synchronized static TelephonyMetrics getInstance() {
        if (sInstance == null) {
            sInstance = new TelephonyMetrics();
        }

        return sInstance;
    }

    /**
     * Dump the state of various objects, add calls to other objects as desired.
     *
     * @param fd File descriptor
     * @param pw Print writer
     * @param args Arguments
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (args != null && args.length > 0) {
            switch (args[0]) {
                case "--metrics":
                    printAllMetrics(pw);
                    break;
                case "--metricsproto":
                    pw.println(convertProtoToBase64String(buildProto()));
                    reset();
                    break;
            }
        }
    }

    /**
     * Convert the telephony event to string
     *
     * @param event The event in integer
     * @return The event in string
     */
    private static String telephonyEventToString(int event) {
        switch (event) {
            case TelephonyEvent.Type.UNKNOWN:
                return "UNKNOWN";
            case TelephonyEvent.Type.SETTINGS_CHANGED:
                return "SETTINGS_CHANGED";
            case TelephonyEvent.Type.RIL_SERVICE_STATE_CHANGED:
                return "RIL_SERVICE_STATE_CHANGED";
            case TelephonyEvent.Type.IMS_CONNECTION_STATE_CHANGED:
                return "IMS_CONNECTION_STATE_CHANGED";
            case TelephonyEvent.Type.IMS_CAPABILITIES_CHANGED:
                return "IMS_CAPABILITIES_CHANGED";
            case TelephonyEvent.Type.DATA_CALL_SETUP:
                return "DATA_CALL_SETUP";
            case TelephonyEvent.Type.DATA_CALL_SETUP_RESPONSE:
                return "DATA_CALL_SETUP_RESPONSE";
            case TelephonyEvent.Type.DATA_CALL_LIST_CHANGED:
                return "DATA_CALL_LIST_CHANGED";
            case TelephonyEvent.Type.DATA_CALL_DEACTIVATE:
                return "DATA_CALL_DEACTIVATE";
            case TelephonyEvent.Type.DATA_CALL_DEACTIVATE_RESPONSE:
                return "DATA_CALL_DEACTIVATE_RESPONSE";
            case TelephonyEvent.Type.DATA_STALL_ACTION:
                return "DATA_STALL_ACTION";
            case TelephonyEvent.Type.MODEM_RESTART:
                return "MODEM_RESTART";
            default:
                return Integer.toString(event);
        }
    }

    /**
     * Convert the call session event into string
     *
     * @param event The event in integer
     * @return The event in String
     */
    private static String callSessionEventToString(int event) {
        switch (event) {
            case TelephonyCallSession.Event.Type.EVENT_UNKNOWN:
                return "EVENT_UNKNOWN";
            case TelephonyCallSession.Event.Type.SETTINGS_CHANGED:
                return "SETTINGS_CHANGED";
            case TelephonyCallSession.Event.Type.RIL_SERVICE_STATE_CHANGED:
                return "RIL_SERVICE_STATE_CHANGED";
            case TelephonyCallSession.Event.Type.IMS_CONNECTION_STATE_CHANGED:
                return "IMS_CONNECTION_STATE_CHANGED";
            case TelephonyCallSession.Event.Type.IMS_CAPABILITIES_CHANGED:
                return "IMS_CAPABILITIES_CHANGED";
            case TelephonyCallSession.Event.Type.DATA_CALL_LIST_CHANGED:
                return "DATA_CALL_LIST_CHANGED";
            case TelephonyCallSession.Event.Type.RIL_REQUEST:
                return "RIL_REQUEST";
            case TelephonyCallSession.Event.Type.RIL_RESPONSE:
                return "RIL_RESPONSE";
            case TelephonyCallSession.Event.Type.RIL_CALL_RING:
                return "RIL_CALL_RING";
            case TelephonyCallSession.Event.Type.RIL_CALL_SRVCC:
                return "RIL_CALL_SRVCC";
            case TelephonyCallSession.Event.Type.RIL_CALL_LIST_CHANGED:
                return "RIL_CALL_LIST_CHANGED";
            case TelephonyCallSession.Event.Type.IMS_COMMAND:
                return "IMS_COMMAND";
            case TelephonyCallSession.Event.Type.IMS_COMMAND_RECEIVED:
                return "IMS_COMMAND_RECEIVED";
            case TelephonyCallSession.Event.Type.IMS_COMMAND_FAILED:
                return "IMS_COMMAND_FAILED";
            case TelephonyCallSession.Event.Type.IMS_COMMAND_COMPLETE:
                return "IMS_COMMAND_COMPLETE";
            case TelephonyCallSession.Event.Type.IMS_CALL_RECEIVE:
                return "IMS_CALL_RECEIVE";
            case TelephonyCallSession.Event.Type.IMS_CALL_STATE_CHANGED:
                return "IMS_CALL_STATE_CHANGED";
            case TelephonyCallSession.Event.Type.IMS_CALL_TERMINATED:
                return "IMS_CALL_TERMINATED";
            case TelephonyCallSession.Event.Type.IMS_CALL_HANDOVER:
                return "IMS_CALL_HANDOVER";
            case TelephonyCallSession.Event.Type.IMS_CALL_HANDOVER_FAILED:
                return "IMS_CALL_HANDOVER_FAILED";
            case TelephonyCallSession.Event.Type.PHONE_STATE_CHANGED:
                return "PHONE_STATE_CHANGED";
            case TelephonyCallSession.Event.Type.NITZ_TIME:
                return "NITZ_TIME";
            default:
                return Integer.toString(event);
        }
    }

    /**
     * Convert the SMS session event into string
     * @param event The event in integer
     * @return The event in String
     */
    private static String smsSessionEventToString(int event) {
        switch (event) {
            case SmsSession.Event.Type.EVENT_UNKNOWN:
                return "EVENT_UNKNOWN";
            case SmsSession.Event.Type.SETTINGS_CHANGED:
                return "SETTINGS_CHANGED";
            case SmsSession.Event.Type.RIL_SERVICE_STATE_CHANGED:
                return "RIL_SERVICE_STATE_CHANGED";
            case SmsSession.Event.Type.IMS_CONNECTION_STATE_CHANGED:
                return "IMS_CONNECTION_STATE_CHANGED";
            case SmsSession.Event.Type.IMS_CAPABILITIES_CHANGED:
                return "IMS_CAPABILITIES_CHANGED";
            case SmsSession.Event.Type.DATA_CALL_LIST_CHANGED:
                return "DATA_CALL_LIST_CHANGED";
            case SmsSession.Event.Type.SMS_SEND:
                return "SMS_SEND";
            case SmsSession.Event.Type.SMS_SEND_RESULT:
                return "SMS_SEND_RESULT";
            case SmsSession.Event.Type.SMS_RECEIVED:
                return "SMS_RECEIVED";
            default:
                return Integer.toString(event);
        }
    }

    /**
     * Print all metrics data for debugging purposes
     *
     * @param rawWriter Print writer
     */
    private synchronized void printAllMetrics(PrintWriter rawWriter) {
        final IndentingPrintWriter pw = new IndentingPrintWriter(rawWriter, "  ");

        pw.println("Telephony metrics proto:");
        pw.println("------------------------------------------");
        pw.println("Telephony events:");
        pw.increaseIndent();
        for (TelephonyEvent event : mTelephonyEvents) {
            if (event.hasTimestampMillis()) {
                pw.print(event.getTimestampMillis());
                pw.print(" [");
                if (event.hasPhoneId()) pw.print(event.getPhoneId());
                pw.print("] ");

                if (event.hasType()) {
                    pw.print("T=");
                    if (event.getType() == TelephonyEvent.Type.RIL_SERVICE_STATE_CHANGED) {
                        pw.print(telephonyEventToString(event.getType())
                                + "(" + event.serviceState.getDataRat() + ")");
                    } else {
                        pw.print(telephonyEventToString(event.getType()));
                    }
                }
                pw.println("");
            }
        }

        pw.decreaseIndent();
        pw.println("Call sessions:");
        pw.increaseIndent();

        for (TelephonyCallSession callSession : mCompletedCallSessions) {
            if (callSession.hasStartTimeMinutes()) {
                pw.println("Start time in minutes: " + callSession.getStartTimeMinutes());
            }
            if (callSession.hasEventsDropped()) {
                pw.println("Events dropped: " + callSession.getEventsDropped());
            }
            pw.println("Events: ");
            pw.increaseIndent();
            for (TelephonyCallSession.Event event : callSession.events) {
                pw.print(event.getDelay());
                pw.print(" T=");
                if (event.getType() == TelephonyCallSession.Event.Type.RIL_SERVICE_STATE_CHANGED) {
                    pw.println(callSessionEventToString(event.getType())
                            + "(" + event.serviceState.getDataRat() + ")");
                } else {
                    pw.println(callSessionEventToString(event.getType()));
                }
            }
            pw.decreaseIndent();
        }

        pw.decreaseIndent();
        pw.println("Sms sessions:");
        pw.increaseIndent();

        int count = 0;
        for (SmsSession smsSession : mCompletedSmsSessions) {
            count++;
            if (smsSession.hasStartTimeMinutes()) {
                pw.print("[" + count + "] Start time in minutes: "
                        + smsSession.getStartTimeMinutes());
            }
            if (smsSession.hasEventsDropped()) {
                pw.println(", events dropped: " + smsSession.getEventsDropped());
            }
            pw.println("Events: ");
            pw.increaseIndent();
            for (SmsSession.Event event : smsSession.events) {
                pw.print(event.getDelay());
                pw.print(" T=");
                pw.println(smsSessionEventToString(event.getType()));
            }
            pw.decreaseIndent();
        }

        pw.decreaseIndent();
    }

    /**
     * Convert the telephony proto into Base-64 encoded string
     *
     * @param proto Telephony proto
     * @return Encoded string
     */
    private static String convertProtoToBase64String(TelephonyLog proto) {
        return Base64.encodeToString(
                TelephonyProto.TelephonyLog.toByteArray(proto), Base64.DEFAULT);
    }

    /**
     * Reset all events and sessions
     */
    private synchronized void reset() {
        mTelephonyEvents.clear();
        mCompletedCallSessions.clear();
        mCompletedSmsSessions.clear();

        mTelephonyEventsDropped = false;

        mStartSystemTimeMs = System.currentTimeMillis();
        mStartElapsedTimeMs = SystemClock.elapsedRealtime();

        // Insert the last known service state, ims capabilities, and ims connection states as the
        // base.
        for (int i = 0; i < mLastServiceState.size(); i++) {
            final int key = mLastServiceState.keyAt(i);

            TelephonyEvent event = new TelephonyEventBuilder(mStartElapsedTimeMs, key)
                    .setServiceState(mLastServiceState.get(key)).build();
            addTelephonyEvent(event);
        }

        for (int i = 0; i < mLastImsCapabilities.size(); i++) {
            final int key = mLastImsCapabilities.keyAt(i);

            TelephonyEvent event = new TelephonyEventBuilder(mStartElapsedTimeMs, key)
                    .setImsCapabilities(mLastImsCapabilities.get(key)).build();
            addTelephonyEvent(event);
        }

        for (int i = 0; i < mLastImsConnectionState.size(); i++) {
            final int key = mLastImsConnectionState.keyAt(i);

            TelephonyEvent event = new TelephonyEventBuilder(mStartElapsedTimeMs, key)
                    .setImsConnectionState(mLastImsConnectionState.get(key)).build();
            addTelephonyEvent(event);
        }
    }

    /**
     * Build the telephony proto
     *
     * @return Telephony proto
     */
    private synchronized TelephonyLog buildProto() {

        TelephonyLog log = new TelephonyLog();
        // Build telephony events
        log.events = new TelephonyEvent[mTelephonyEvents.size()];
        mTelephonyEvents.toArray(log.events);
        log.setEventsDropped(mTelephonyEventsDropped);

        // Build call sessions
        log.callSessions = new TelephonyCallSession[mCompletedCallSessions.size()];
        mCompletedCallSessions.toArray(log.callSessions);

        // Build SMS sessions
        log.smsSessions = new SmsSession[mCompletedSmsSessions.size()];
        mCompletedSmsSessions.toArray(log.smsSessions);

        // Build histogram. Currently we only support RIL histograms.
        List<TelephonyHistogram> rilHistograms = RIL.getTelephonyRILTimingHistograms();
        log.histograms = new TelephonyProto.TelephonyHistogram[rilHistograms.size()];
        for (int i = 0; i < rilHistograms.size(); i++) {
            log.histograms[i] = new TelephonyProto.TelephonyHistogram();
            TelephonyHistogram rilHistogram = rilHistograms.get(i);
            TelephonyProto.TelephonyHistogram histogramProto = log.histograms[i];

            histogramProto.setCategory(rilHistogram.getCategory());
            histogramProto.setId(rilHistogram.getId());
            histogramProto.setMinTimeMillis(rilHistogram.getMinTime());
            histogramProto.setMaxTimeMillis(rilHistogram.getMaxTime());
            histogramProto.setAvgTimeMillis(rilHistogram.getAverageTime());
            histogramProto.setCount(rilHistogram.getSampleCount());
            histogramProto.setBucketCount(rilHistogram.getBucketCount());
            histogramProto.bucketEndPoints = rilHistogram.getBucketEndPoints();
            histogramProto.bucketCounters = rilHistogram.getBucketCounters();
        }

        // Log the starting system time
        log.startTime = new TelephonyProto.Time();
        log.startTime.setSystemTimestampMillis(mStartSystemTimeMs);
        log.startTime.setElapsedTimestampMillis(mStartElapsedTimeMs);

        log.endTime = new TelephonyProto.Time();
        log.endTime.setSystemTimestampMillis(System.currentTimeMillis());
        log.endTime.setElapsedTimestampMillis(SystemClock.elapsedRealtime());

        return log;
    }

    /**
     * Reduce precision to meet privacy requirements.
     *
     * @param timestamp timestamp in milliseconds
     * @return Precision reduced timestamp in minutes
     */
    static int roundSessionStart(long timestamp) {
        return (int) ((timestamp) / (MINUTE_IN_MILLIS * SESSION_START_PRECISION_MINUTES)
                * (SESSION_START_PRECISION_MINUTES));
    }

    /**
     * Get the time interval with reduced prevision
     *
     * @param previousTimestamp Previous timestamp in milliseconds
     * @param currentTimestamp Current timestamp in milliseconds
     * @return The time interval
     */
    static int toPrivacyFuzzedTimeInterval(long previousTimestamp, long currentTimestamp) {
        long diff = currentTimestamp - previousTimestamp;
        if (diff < 0) {
            return TimeInterval.TI_UNKNOWN;
        } else if (diff <= 10) {
            return TimeInterval.TI_10_MILLIS;
        } else if (diff <= 20) {
            return TimeInterval.TI_20_MILLIS;
        } else if (diff <= 50) {
            return TimeInterval.TI_50_MILLIS;
        } else if (diff <= 100) {
            return TimeInterval.TI_100_MILLIS;
        } else if (diff <= 200) {
            return TimeInterval.TI_200_MILLIS;
        } else if (diff <= 500) {
            return TimeInterval.TI_500_MILLIS;
        } else if (diff <= 1000) {
            return TimeInterval.TI_1_SEC;
        } else if (diff <= 2000) {
            return TimeInterval.TI_2_SEC;
        } else if (diff <= 5000) {
            return TimeInterval.TI_5_SEC;
        } else if (diff <= 10000) {
            return TimeInterval.TI_10_SEC;
        } else if (diff <= 30000) {
            return TimeInterval.TI_30_SEC;
        } else if (diff <= 60000) {
            return TimeInterval.TI_1_MINUTE;
        } else if (diff <= 180000) {
            return TimeInterval.TI_3_MINUTES;
        } else if (diff <= 600000) {
            return TimeInterval.TI_10_MINUTES;
        } else if (diff <= 1800000) {
            return TimeInterval.TI_30_MINUTES;
        } else if (diff <= 3600000) {
            return TimeInterval.TI_1_HOUR;
        } else if (diff <= 7200000) {
            return TimeInterval.TI_2_HOURS;
        } else if (diff <= 14400000) {
            return TimeInterval.TI_4_HOURS;
        } else {
            return TimeInterval.TI_MANY_HOURS;
        }
    }

    /**
     * Convert the service state into service state proto
     *
     * @param serviceState Service state
     * @return Service state proto
     */
    private TelephonyServiceState toServiceStateProto(ServiceState serviceState) {
        TelephonyServiceState ssProto = new TelephonyServiceState();

        ssProto.setVoiceRoamingType(serviceState.getVoiceRoamingType());
        ssProto.setDataRoamingType(serviceState.getDataRoamingType());

        ssProto.voiceOperator = new TelephonyServiceState.TelephonyOperator();

        if (serviceState.getVoiceOperatorAlphaLong() != null) {
            ssProto.voiceOperator.setAlphaLong(serviceState.getVoiceOperatorAlphaLong());
        }

        if (serviceState.getVoiceOperatorAlphaShort() != null) {
            ssProto.voiceOperator.setAlphaShort(serviceState.getVoiceOperatorAlphaShort());
        }

        if (serviceState.getVoiceOperatorNumeric() != null) {
            ssProto.voiceOperator.setNumeric(serviceState.getVoiceOperatorNumeric());
        }

        ssProto.dataOperator = new TelephonyServiceState.TelephonyOperator();

        if (serviceState.getDataOperatorAlphaLong() != null) {
            ssProto.dataOperator.setAlphaLong(serviceState.getDataOperatorAlphaLong());
        }

        if (serviceState.getDataOperatorAlphaShort() != null) {
            ssProto.dataOperator.setAlphaShort(serviceState.getDataOperatorAlphaShort());
        }

        if (serviceState.getDataOperatorNumeric() != null) {
            ssProto.dataOperator.setNumeric(serviceState.getDataOperatorNumeric());
        }

        ssProto.setVoiceRat(serviceState.getRilVoiceRadioTechnology());
        ssProto.setDataRat(serviceState.getRilDataRadioTechnology());
        return ssProto;
    }

    /**
     * Annotate the call session with events
     *
     * @param timestamp Event timestamp
     * @param phoneId Phone id
     * @param eventBuilder Call session event builder
     */
    private synchronized void annotateInProgressCallSession(long timestamp, int phoneId,
                                                            CallSessionEventBuilder eventBuilder) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession != null) {
            callSession.addEvent(timestamp, eventBuilder);
        }
    }

    /**
     * Annotate the SMS session with events
     *
     * @param timestamp Event timestamp
     * @param phoneId Phone id
     * @param eventBuilder SMS session event builder
     */
    private synchronized void annotateInProgressSmsSession(long timestamp, int phoneId,
                                                           SmsSessionEventBuilder eventBuilder) {
        InProgressSmsSession smsSession = mInProgressSmsSessions.get(phoneId);
        if (smsSession != null) {
            smsSession.addEvent(timestamp, eventBuilder);
        }
    }

    /**
     * Create the call session if there isn't any existing one
     *
     * @param phoneId Phone id
     * @return The call session
     */
    private synchronized InProgressCallSession startNewCallSessionIfNeeded(int phoneId) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            if (VDBG) Rlog.v(TAG, "Starting a new call session on phone " + phoneId);
            callSession = new InProgressCallSession(phoneId);
            mInProgressCallSessions.append(phoneId, callSession);

            // Insert the latest service state, ims capabilities, and ims connection states as the
            // base.
            TelephonyServiceState serviceState = mLastServiceState.get(phoneId);
            if (serviceState != null) {
                callSession.addEvent(callSession.startElapsedTimeMs, new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.RIL_SERVICE_STATE_CHANGED)
                        .setServiceState(serviceState));
            }

            ImsCapabilities imsCapabilities = mLastImsCapabilities.get(phoneId);
            if (imsCapabilities != null) {
                callSession.addEvent(callSession.startElapsedTimeMs, new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.IMS_CAPABILITIES_CHANGED)
                        .setImsCapabilities(imsCapabilities));
            }

            ImsConnectionState imsConnectionState = mLastImsConnectionState.get(phoneId);
            if (imsConnectionState != null) {
                callSession.addEvent(callSession.startElapsedTimeMs, new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.IMS_CONNECTION_STATE_CHANGED)
                        .setImsConnectionState(imsConnectionState));
            }
        }
        return callSession;
    }

    /**
     * Create the SMS session if there isn't any existing one
     *
     * @param phoneId Phone id
     * @return The SMS session
     */
    private synchronized InProgressSmsSession startNewSmsSessionIfNeeded(int phoneId) {
        InProgressSmsSession smsSession = mInProgressSmsSessions.get(phoneId);
        if (smsSession == null) {
            if (VDBG) Rlog.v(TAG, "Starting a new sms session on phone " + phoneId);
            smsSession = new InProgressSmsSession(phoneId);
            mInProgressSmsSessions.append(phoneId, smsSession);

            // Insert the latest service state, ims capabilities, and ims connection state as the
            // base.
            TelephonyServiceState serviceState = mLastServiceState.get(phoneId);
            if (serviceState != null) {
                smsSession.addEvent(smsSession.startElapsedTimeMs, new SmsSessionEventBuilder(
                        TelephonyCallSession.Event.Type.RIL_SERVICE_STATE_CHANGED)
                        .setServiceState(serviceState));
            }

            ImsCapabilities imsCapabilities = mLastImsCapabilities.get(phoneId);
            if (imsCapabilities != null) {
                smsSession.addEvent(smsSession.startElapsedTimeMs, new SmsSessionEventBuilder(
                        SmsSession.Event.Type.IMS_CAPABILITIES_CHANGED)
                        .setImsCapabilities(imsCapabilities));
            }

            ImsConnectionState imsConnectionState = mLastImsConnectionState.get(phoneId);
            if (imsConnectionState != null) {
                smsSession.addEvent(smsSession.startElapsedTimeMs, new SmsSessionEventBuilder(
                        SmsSession.Event.Type.IMS_CONNECTION_STATE_CHANGED)
                        .setImsConnectionState(imsConnectionState));
            }
        }
        return smsSession;
    }

    /**
     * Finish the call session and move it into the completed session
     *
     * @param inProgressCallSession The in progress call session
     */
    private synchronized void finishCallSession(InProgressCallSession inProgressCallSession) {
        TelephonyCallSession callSession = new TelephonyCallSession();
        callSession.events = new TelephonyCallSession.Event[inProgressCallSession.events.size()];
        inProgressCallSession.events.toArray(callSession.events);
        callSession.setStartTimeMinutes(inProgressCallSession.startSystemTimeMin);
        callSession.setPhoneId(inProgressCallSession.phoneId);
        callSession.setEventsDropped(inProgressCallSession.isEventsDropped());
        if (mCompletedCallSessions.size() >= MAX_COMPLETED_CALL_SESSIONS) {
            mCompletedCallSessions.removeFirst();
        }
        mCompletedCallSessions.add(callSession);
        mInProgressCallSessions.remove(inProgressCallSession.phoneId);
        if (VDBG) Rlog.v(TAG, "Call session finished");
    }

    /**
     * Finish the SMS session and move it into the completed session
     *
     * @param inProgressSmsSession The in progress SMS session
     */
    private synchronized void finishSmsSessionIfNeeded(InProgressSmsSession inProgressSmsSession) {
        if (inProgressSmsSession.getNumExpectedResponses() == 0) {
            SmsSession smsSession = new SmsSession();
            smsSession.events = new SmsSession.Event[inProgressSmsSession.events.size()];
            inProgressSmsSession.events.toArray(smsSession.events);
            smsSession.setStartTimeMinutes(inProgressSmsSession.startSystemTimeMin);
            smsSession.setPhoneId(inProgressSmsSession.phoneId);
            smsSession.setEventsDropped(inProgressSmsSession.isEventsDropped());
            if (mCompletedSmsSessions.size() >= MAX_COMPLETED_SMS_SESSIONS) {
                mCompletedSmsSessions.removeFirst();
            }
            mCompletedSmsSessions.add(smsSession);
            mInProgressSmsSessions.remove(inProgressSmsSession.phoneId);
            if (VDBG) Rlog.v(TAG, "SMS session finished");
        }
    }

    /**
     * Add telephony event into the queue
     *
     * @param event Telephony event
     */
    private synchronized void addTelephonyEvent(TelephonyEvent event) {
        if (mTelephonyEvents.size() >= MAX_TELEPHONY_EVENTS) {
            mTelephonyEvents.removeFirst();
            mTelephonyEventsDropped = true;
        }
        mTelephonyEvents.add(event);
    }

    /**
     * Write service changed event
     *
     * @param phoneId Phone id
     * @param serviceState Service state
     */
    public synchronized void writeServiceStateChanged(int phoneId, ServiceState serviceState) {

        TelephonyEvent event = new TelephonyEventBuilder(phoneId)
                .setServiceState(toServiceStateProto(serviceState)).build();

        mLastServiceState.put(phoneId, event.serviceState);
        addTelephonyEvent(event);

        annotateInProgressCallSession(event.getTimestampMillis(), phoneId,
                new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.RIL_SERVICE_STATE_CHANGED)
                        .setServiceState(event.serviceState));
        annotateInProgressSmsSession(event.getTimestampMillis(), phoneId,
                new SmsSessionEventBuilder(
                        SmsSession.Event.Type.RIL_SERVICE_STATE_CHANGED)
                        .setServiceState(event.serviceState));
    }

    /**
     * Write data stall event
     *
     * @param phoneId Phone id
     * @param recoveryAction Data stall recovery action
     */
    public void writeDataStallEvent(int phoneId, int recoveryAction) {
        addTelephonyEvent(new TelephonyEventBuilder(phoneId)
                .setDataStallRecoveryAction(recoveryAction).build());
    }

    /**
     * Write IMS feature settings changed event
     *
     * @param phoneId Phone id
     * @param feature IMS feature
     * @param network The IMS network type
     * @param value The settings. 0 indicates disabled, otherwise enabled.
     * @param status IMS operation status. See OperationStatusConstants for details.
     */
    public void writeImsSetFeatureValue(int phoneId, int feature, int network, int value,
                                        int status) {
        TelephonySettings s = new TelephonySettings();
        switch (feature) {
            case ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE:
                s.setIsEnhanced4GLteModeEnabled(value != 0);
                break;
            case ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI:
                s.setIsWifiCallingEnabled(value != 0);
                break;
            case ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE:
                s.setIsVtOverLteEnabled(value != 0);
                break;
            case ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI:
                s.setIsVtOverWifiEnabled(value != 0);
                break;
        }

        TelephonyEvent event = new TelephonyEventBuilder(phoneId).setSettings(s).build();
        addTelephonyEvent(event);

        annotateInProgressCallSession(event.getTimestampMillis(), phoneId,
                new CallSessionEventBuilder(TelephonyCallSession.Event.Type.SETTINGS_CHANGED)
                        .setSettings(s));
        annotateInProgressSmsSession(event.getTimestampMillis(), phoneId,
                new SmsSessionEventBuilder(SmsSession.Event.Type.SETTINGS_CHANGED)
                        .setSettings(s));
    }

    /**
     * Write the preferred network settings changed event
     *
     * @param phoneId Phone id
     * @param networkType The preferred network
     */
    public void writeSetPreferredNetworkType(int phoneId, int networkType) {
        TelephonySettings s = new TelephonySettings();
        s.setPreferredNetworkMode(networkType);
        addTelephonyEvent(new TelephonyEventBuilder(phoneId).setSettings(s).build());
    }

    /**
     * Write the IMS connection state changed event
     *
     * @param phoneId Phone id
     * @param state IMS connection state
     * @param reasonInfo The reason info. Only used for disconnected state.
     */
    public synchronized void writeOnImsConnectionState(int phoneId, int state,
                                                       ImsReasonInfo reasonInfo) {
        ImsConnectionState imsState = new ImsConnectionState();
        imsState.setState(state);
        mLastImsConnectionState.put(phoneId, imsState);

        if (reasonInfo != null) {
            TelephonyProto.ImsReasonInfo ri = new TelephonyProto.ImsReasonInfo();

            ri.setReasonCode(reasonInfo.getCode());
            ri.setExtraCode(reasonInfo.getExtraCode());
            String extraMessage = reasonInfo.getExtraMessage();
            if (extraMessage != null) {
                ri.setExtraMessage(extraMessage);
            }

            imsState.reasonInfo = ri;
        }

        TelephonyEvent event = new TelephonyEventBuilder(phoneId)
                .setImsConnectionState(imsState).build();
        addTelephonyEvent(event);

        annotateInProgressCallSession(event.getTimestampMillis(), phoneId,
                new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.IMS_CONNECTION_STATE_CHANGED)
                        .setImsConnectionState(event.imsConnectionState));
        annotateInProgressSmsSession(event.getTimestampMillis(), phoneId,
                new SmsSessionEventBuilder(
                        SmsSession.Event.Type.IMS_CONNECTION_STATE_CHANGED)
                        .setImsConnectionState(event.imsConnectionState));
    }

    /**
     * Write the IMS capabilities changed event
     *
     * @param phoneId Phone id
     * @param capabilities IMS capabilities array
     */
    public synchronized void writeOnImsCapabilities(int phoneId, boolean[] capabilities) {
        ImsCapabilities cap = new ImsCapabilities();

        cap.setVoiceOverLte(capabilities[0]);
        cap.setVideoOverLte(capabilities[1]);
        cap.setVoiceOverWifi(capabilities[2]);
        cap.setVideoOverWifi(capabilities[3]);
        cap.setUtOverLte(capabilities[4]);
        cap.setUtOverWifi(capabilities[5]);

        TelephonyEvent event = new TelephonyEventBuilder(phoneId).setImsCapabilities(cap).build();
        mLastImsCapabilities.put(phoneId, cap);
        addTelephonyEvent(event);

        annotateInProgressCallSession(event.getTimestampMillis(), phoneId,
                new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.IMS_CAPABILITIES_CHANGED)
                        .setImsCapabilities(event.imsCapabilities));
        annotateInProgressSmsSession(event.getTimestampMillis(), phoneId,
                new SmsSessionEventBuilder(
                        SmsSession.Event.Type.IMS_CAPABILITIES_CHANGED)
                        .setImsCapabilities(event.imsCapabilities));
    }

    /**
     * Convert PDP type into the enumeration
     *
     * @param type PDP type
     * @return The proto defined enumeration
     */
    private int toPdpType(String type) {
        switch (type) {
            case "IP":
                return PDP_TYPE_IP;
            case "IPV6":
                return PDP_TYPE_IPV6;
            case "IPV4V6":
                return PDP_TYPE_IPV4V6;
            case "PPP":
                return PDP_TYPE_PPP;
        }
        Rlog.e(TAG, "Unknown type: " + type);
        return PDP_UNKNOWN;
    }

    /**
     * Write setup data call event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param radioTechnology The data call RAT
     * @param profile Data profile
     * @param apn APN in string
     * @param authType Authentication type
     * @param protocol Data connection protocol
     */
    public void writeRilSetupDataCall(int phoneId, int rilSerial, int radioTechnology, int profile,
                                      String apn, int authType, String protocol) {

        RilSetupDataCall setupDataCall = new RilSetupDataCall();
        setupDataCall.setRat(radioTechnology);
        setupDataCall.setDataProfile(profile + 1);  // off by 1 between proto and RIL constants.
        if (apn != null) {
            setupDataCall.setApn(apn);
        }
        if (protocol != null) {
            setupDataCall.setType(toPdpType(protocol));
        }

        addTelephonyEvent(new TelephonyEventBuilder(phoneId).setSetupDataCall(
                setupDataCall).build());
    }

    /**
     * Write data call deactivate event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param cid call id
     * @param reason Deactivate reason
     */
    public void writeRilDeactivateDataCall(int phoneId, int rilSerial, int cid, int reason) {

        RilDeactivateDataCall deactivateDataCall = new RilDeactivateDataCall();
        deactivateDataCall.setCid(cid);
        deactivateDataCall.setReason(reason + 1);

        addTelephonyEvent(new TelephonyEventBuilder(phoneId).setDeactivateDataCall(
                deactivateDataCall).build());
    }

    /**
     * Write get data call list event
     *
     * @param phoneId Phone id
     * @param dcsList Data call list
     */
    public void writeRilDataCallList(int phoneId, ArrayList<DataCallResponse> dcsList) {

        RilDataCall[] dataCalls = new RilDataCall[dcsList.size()];

        for (int i = 0; i < dcsList.size(); i++) {
            dataCalls[i] = new RilDataCall();
            dataCalls[i].setCid(dcsList.get(i).cid);
            if (dcsList.get(i).ifname != null) {
                dataCalls[i].setIframe(dcsList.get(i).ifname);
            }
            if (dcsList.get(i).type != null) {
                dataCalls[i].setType(toPdpType(dcsList.get(i).type));
            }
        }

        addTelephonyEvent(new TelephonyEventBuilder(phoneId).setDataCalls(dataCalls).build());
    }

    /**
     * Write dial event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param clirMode CLIR (Calling Line Identification Restriction) mode
     * @param uusInfo User-to-User signaling Info
     */
    public void writeRilDial(int phoneId, int rilSerial, int clirMode, UUSInfo uusInfo) {

        InProgressCallSession callSession = startNewCallSessionIfNeeded(phoneId);

        callSession.addEvent(callSession.startElapsedTimeMs,
                new CallSessionEventBuilder(TelephonyCallSession.Event.Type.RIL_REQUEST)
                        .setRilRequest(TelephonyCallSession.Event.RilRequest.RIL_REQUEST_DIAL)
                        .setRilRequestId(rilSerial)
        );
    }

    /**
     * Write incoming call event
     *
     * @param phoneId Phone id
     * @param response Unused today
     */
    public void writeRilCallRing(int phoneId, char[] response) {
        InProgressCallSession callSession = startNewCallSessionIfNeeded(phoneId);

        callSession.addEvent(callSession.startElapsedTimeMs,
                new CallSessionEventBuilder(TelephonyCallSession.Event.Type.RIL_CALL_RING));
    }

    /**
     * Write call hangup event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param callId Call id
     */
    public void writeRilHangup(int phoneId, int rilSerial, int callId) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(
                    new CallSessionEventBuilder(TelephonyCallSession.Event.Type.RIL_REQUEST)
                            .setRilRequest(TelephonyCallSession.Event.RilRequest.RIL_REQUEST_HANGUP)
                            .setRilRequestId(rilSerial)
                            .setCallIndex(callId));
        }
    }

    /**
     * Write call answer event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     */
    public void writeRilAnswer(int phoneId, int rilSerial) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(
                    new CallSessionEventBuilder(TelephonyCallSession.Event.Type.RIL_REQUEST)
                            .setRilRequest(TelephonyCallSession.Event.RilRequest.RIL_REQUEST_ANSWER)
                            .setRilRequestId(rilSerial));
        }
    }

    /**
     * Write IMS call SRVCC event
     *
     * @param phoneId Phone id
     * @param rilSrvccState SRVCC state
     */
    public void writeRilSrvcc(int phoneId, int rilSrvccState) {
        InProgressCallSession callSession =  mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(
                    new CallSessionEventBuilder(TelephonyCallSession.Event.Type.RIL_CALL_SRVCC)
                            .setSrvccState(rilSrvccState + 1));
        }
    }

    /**
     * Convert RIL request into proto defined RIL request
     *
     * @param r RIL request
     * @return RIL request defined in call session proto
     */
    private int toCallSessionRilRequest(int r) {
        switch (r) {
            case RILConstants.RIL_REQUEST_DIAL:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_DIAL;

            case RILConstants.RIL_REQUEST_ANSWER:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_ANSWER;

            case RILConstants.RIL_REQUEST_HANGUP:
            case RILConstants.RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
            case RILConstants.RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_HANGUP;

            case RILConstants.RIL_REQUEST_SET_CALL_WAITING:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_SET_CALL_WAITING;

            case RILConstants.RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE;

            case RILConstants.RIL_REQUEST_CDMA_FLASH:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_CDMA_FLASH;

            case RILConstants.RIL_REQUEST_CONFERENCE:
                return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_CONFERENCE;
        }
        Rlog.e(TAG, "Unknown RIL request: " + r);
        return TelephonyCallSession.Event.RilRequest.RIL_REQUEST_UNKNOWN;
    }

    /**
     * Write setup data call response event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param rilError RIL error
     * @param rilRequest RIL request
     * @param response Data call response
     */
    private void writeOnSetupDataCallResponse(int phoneId, int rilSerial, int rilError,
                                              int rilRequest, DataCallResponse response) {

        RilSetupDataCallResponse setupDataCallResponse = new RilSetupDataCallResponse();
        RilDataCall dataCall = new RilDataCall();

        if (response != null) {
            setupDataCallResponse.setStatus(
                    response.status == 0 ? RilDataCallFailCause.PDP_FAIL_NONE : response.status);
            setupDataCallResponse.setSuggestedRetryTimeMillis(response.suggestedRetryTime);

            dataCall.setCid(response.cid);
            if (response.type != null) {
                dataCall.setType(toPdpType(response.type));
            }

            if (response.ifname != null) {
                dataCall.setIframe(response.ifname);
            }
        }
        setupDataCallResponse.call = dataCall;

        addTelephonyEvent(new TelephonyEventBuilder(phoneId)
                .setSetupDataCallResponse(setupDataCallResponse).build());
    }

    /**
     * Write call related solicited response event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param rilError RIL error
     * @param rilRequest RIL request
     */
    private void writeOnCallSolicitedResponse(int phoneId, int rilSerial, int rilError,
                                              int rilRequest) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(new CallSessionEventBuilder(
                    TelephonyCallSession.Event.Type.RIL_RESPONSE)
                    .setRilRequest(toCallSessionRilRequest(rilRequest))
                    .setRilRequestId(rilSerial)
                    .setRilError(rilError));
        }
    }

    /**
     * Write SMS related solicited response event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param rilError RIL error
     * @param response SMS response
     */
    private synchronized void writeOnSmsSolicitedResponse(int phoneId, int rilSerial, int rilError,
                                                          SmsResponse response) {

        InProgressSmsSession smsSession = mInProgressSmsSessions.get(phoneId);
        if (smsSession == null) {
            Rlog.e(TAG, "SMS session is missing");
        } else {

            int errorCode = 0;
            if (response != null) {
                errorCode = response.mErrorCode;
            }

            smsSession.addEvent(new SmsSessionEventBuilder(
                    SmsSession.Event.Type.SMS_SEND_RESULT)
                    .setErrorCode(errorCode)
                    .setRilErrno(rilError)
                    .setRilRequestId(rilSerial)
            );

            smsSession.decreaseExpectedResponse();
            finishSmsSessionIfNeeded(smsSession);
        }
    }

    /**
     * Write deactivate data call response event
     *
     * @param phoneId Phone id
     * @param rilError RIL error
     */
    private void writeOnDeactivateDataCallResponse(int phoneId, int rilError) {
        addTelephonyEvent(new TelephonyEventBuilder(phoneId)
                .setDeactivateDataCallResponse(rilError + 1).build());
    }

    /**
     * Write RIL solicited response event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param rilError RIL error
     * @param rilRequest RIL request
     * @param ret The returned RIL response
     */
    public void writeOnRilSolicitedResponse(int phoneId, int rilSerial, int rilError,
                                            int rilRequest, Object ret) {
        switch (rilRequest) {
            case RIL_REQUEST_SETUP_DATA_CALL:
                DataCallResponse dataCall = (DataCallResponse) ret;
                writeOnSetupDataCallResponse(phoneId, rilSerial, rilError, rilRequest, dataCall);
                break;
            case RIL_REQUEST_DEACTIVATE_DATA_CALL:
                writeOnDeactivateDataCallResponse(phoneId, rilError);
                break;
            case RIL_REQUEST_HANGUP:
            case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
            case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
            case RIL_REQUEST_DIAL:
            case RIL_REQUEST_ANSWER:
                writeOnCallSolicitedResponse(phoneId, rilSerial, rilError, rilRequest);
                break;
            case RIL_REQUEST_SEND_SMS:
            case RIL_REQUEST_SEND_SMS_EXPECT_MORE:
            case RIL_REQUEST_CDMA_SEND_SMS:
            case RIL_REQUEST_IMS_SEND_SMS:
                SmsResponse smsResponse = (SmsResponse) ret;
                writeOnSmsSolicitedResponse(phoneId, rilSerial, rilError, smsResponse);
                break;
        }
    }

    /**
     * Write phone state changed event
     *
     * @param phoneId Phone id
     * @param phoneState Phone state. See PhoneConstants.State for the details.
     */
    public void writePhoneState(int phoneId, PhoneConstants.State phoneState) {
        int state;
        switch (phoneState) {
            case IDLE:
                state = TelephonyCallSession.Event.PhoneState.STATE_IDLE;
                break;
            case RINGING:
                state = TelephonyCallSession.Event.PhoneState.STATE_RINGING;
                break;
            case OFFHOOK:
                state = TelephonyCallSession.Event.PhoneState.STATE_OFFHOOK;
                break;
            default:
                state = TelephonyCallSession.Event.PhoneState.STATE_UNKNOWN;
                break;
        }

        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            if (state == TelephonyCallSession.Event.PhoneState.STATE_IDLE) {
                finishCallSession(callSession);
            }
            callSession.addEvent(new CallSessionEventBuilder(
                    TelephonyCallSession.Event.Type.PHONE_STATE_CHANGED)
                    .setPhoneState(state));
        }
    }

    /**
     * Extracts the call ID from an ImsSession.
     *
     * @param session The session.
     * @return The call ID for the session, or -1 if none was found.
     */
    private int getCallId(ImsCallSession session) {
        if (session == null) {
            return -1;
        }

        try {
            return Integer.parseInt(session.getCallId());
        } catch (NumberFormatException nfe) {
            return -1;
        }
    }

    /**
     * Write IMS call state changed event
     *
     * @param phoneId Phone id
     * @param session IMS call session
     * @param callState IMS call state
     */
    public void writeImsCallState(int phoneId, ImsCallSession session,
                                  ImsPhoneCall.State callState) {
        int state;
        switch (callState) {
            case IDLE:
                state = TelephonyCallSession.Event.CallState.CALL_IDLE; break;
            case ACTIVE:
                state = TelephonyCallSession.Event.CallState.CALL_ACTIVE; break;
            case HOLDING:
                state = TelephonyCallSession.Event.CallState.CALL_HOLDING; break;
            case DIALING:
                state = TelephonyCallSession.Event.CallState.CALL_DIALING; break;
            case ALERTING:
                state = TelephonyCallSession.Event.CallState.CALL_ALERTING; break;
            case INCOMING:
                state = TelephonyCallSession.Event.CallState.CALL_INCOMING; break;
            case WAITING:
                state = TelephonyCallSession.Event.CallState.CALL_WAITING; break;
            case DISCONNECTED:
                state = TelephonyCallSession.Event.CallState.CALL_DISCONNECTED; break;
            case DISCONNECTING:
                state = TelephonyCallSession.Event.CallState.CALL_DISCONNECTING; break;
            default:
                state = TelephonyCallSession.Event.CallState.CALL_UNKNOWN; break;
        }

        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(new CallSessionEventBuilder(
                    TelephonyCallSession.Event.Type.IMS_CALL_STATE_CHANGED)
                    .setCallIndex(getCallId(session))
                    .setCallState(state));
        }
    }

    /**
     * Write IMS call start event
     *
     * @param phoneId Phone id
     * @param session IMS call session
     */
    public void writeOnImsCallStart(int phoneId, ImsCallSession session) {
        InProgressCallSession callSession = startNewCallSessionIfNeeded(phoneId);

        callSession.addEvent(
                new CallSessionEventBuilder(TelephonyCallSession.Event.Type.IMS_COMMAND)
                        .setCallIndex(getCallId(session))
                        .setImsCommand(TelephonyCallSession.Event.ImsCommand.IMS_CMD_START));
    }

    /**
     * Write IMS incoming call event
     *
     * @param phoneId Phone id
     * @param session IMS call session
     */
    public void writeOnImsCallReceive(int phoneId, ImsCallSession session) {
        writeOnImsCallStart(phoneId, session);
    }

    /**
     * Write IMS command event
     *
     * @param phoneId Phone id
     * @param session IMS call session
     * @param command IMS command
     */
    public void writeOnImsCommand(int phoneId, ImsCallSession session, int command) {

        InProgressCallSession callSession =  mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(
                    new CallSessionEventBuilder(TelephonyCallSession.Event.Type.IMS_COMMAND)
                            .setCallIndex(getCallId(session))
                            .setImsCommand(command));
        }
    }

    /**
     * Convert IMS reason info into proto
     *
     * @param reasonInfo IMS reason info
     * @return Converted proto
     */
    private TelephonyProto.ImsReasonInfo toImsReasonInfoProto(ImsReasonInfo reasonInfo) {
        TelephonyProto.ImsReasonInfo ri = new TelephonyProto.ImsReasonInfo();
        if (reasonInfo != null) {
            ri.setReasonCode(reasonInfo.getCode());
            ri.setExtraCode(reasonInfo.getExtraCode());
            String extraMessage = reasonInfo.getExtraMessage();
            if (extraMessage != null) {
                ri.setExtraMessage(extraMessage);
            }
        }
        return ri;
    }

    /**
     * Write IMS call end event
     *
     * @param phoneId Phone id
     * @param session IMS call session
     * @param reasonInfo Call end reason
     */
    public void writeOnImsCallTerminated(int phoneId, ImsCallSession session,
                                         ImsReasonInfo reasonInfo) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(
                    new CallSessionEventBuilder(TelephonyCallSession.Event.Type.IMS_CALL_TERMINATED)
                            .setCallIndex(getCallId(session))
                            .setImsReasonInfo(toImsReasonInfoProto(reasonInfo)));
        }
    }

    /**
     * Write IMS call hangover event
     *
     * @param phoneId Phone id
     * @param eventType hangover type
     * @param session IMS call session
     * @param srcAccessTech Hangover starting RAT
     * @param targetAccessTech Hangover destination RAT
     * @param reasonInfo Hangover reason
     */
    public void writeOnImsCallHandoverEvent(int phoneId, int eventType, ImsCallSession session,
                                            int srcAccessTech, int targetAccessTech,
                                            ImsReasonInfo reasonInfo) {
        InProgressCallSession callSession = mInProgressCallSessions.get(phoneId);
        if (callSession == null) {
            Rlog.e(TAG, "Call session is missing");
        } else {
            callSession.addEvent(
                    new CallSessionEventBuilder(eventType)
                            .setCallIndex(getCallId(session))
                            .setSrcAccessTech(srcAccessTech)
                            .setTargetAccessTech(targetAccessTech)
                            .setImsReasonInfo(toImsReasonInfoProto(reasonInfo)));
        }
    }

    /**
     * Write Send SMS event
     *
     * @param phoneId Phone id
     * @param rilSerial RIL request serial number
     * @param tech SMS RAT
     * @param format SMS format. Either 3GPP or 3GPP2.
     */
    public void writeRilSendSms(int phoneId, int rilSerial, int tech, int format) {
        InProgressSmsSession smsSession = startNewSmsSessionIfNeeded(phoneId);

        smsSession.addEvent(new SmsSessionEventBuilder(SmsSession.Event.Type.SMS_SEND)
                .setTech(tech)
                .setRilRequestId(rilSerial)
                .setFormat(format)
        );

        smsSession.increaseExpectedResponse();
    }

    /**
     * Write incoming SMS event
     *
     * @param phoneId Phone id
     * @param tech SMS RAT
     * @param format SMS format. Either 3GPP or 3GPP2.
     */
    public void writeRilNewSms(int phoneId, int tech, int format) {
        InProgressSmsSession smsSession = startNewSmsSessionIfNeeded(phoneId);

        smsSession.addEvent(new SmsSessionEventBuilder(SmsSession.Event.Type.SMS_RECEIVED)
                .setTech(tech)
                .setFormat(format)
        );

        finishSmsSessionIfNeeded(smsSession);
    }

    /**
     * Write NITZ event
     *
     * @param phoneId Phone id
     * @param timestamp NITZ time in milliseconds
     */
    public void writeNITZEvent(int phoneId, long timestamp) {
        TelephonyEvent event = new TelephonyEventBuilder(phoneId).setNITZ(timestamp).build();
        addTelephonyEvent(event);

        annotateInProgressCallSession(event.getTimestampMillis(), phoneId,
                new CallSessionEventBuilder(
                        TelephonyCallSession.Event.Type.NITZ_TIME)
                        .setNITZ(timestamp));
    }

    //TODO: Expand the proto in the future
    public void writeOnImsCallProgressing(int phoneId, ImsCallSession session) {}
    public void writeOnImsCallStarted(int phoneId, ImsCallSession session) {}
    public void writeOnImsCallStartFailed(int phoneId, ImsCallSession session,
                                          ImsReasonInfo reasonInfo) {}
    public void writeOnImsCallHeld(int phoneId, ImsCallSession session) {}
    public void writeOnImsCallHoldReceived(int phoneId, ImsCallSession session) {}
    public void writeOnImsCallHoldFailed(int phoneId, ImsCallSession session,
                                         ImsReasonInfo reasonInfo) {}
    public void writeOnImsCallResumed(int phoneId, ImsCallSession session) {}
    public void writeOnImsCallResumeReceived(int phoneId, ImsCallSession session) {}
    public void writeOnImsCallResumeFailed(int phoneId, ImsCallSession session,
                                           ImsReasonInfo reasonInfo) {}
    public void writeOnRilTimeoutResponse(int phoneId, int rilSerial, int rilRequest) {}
}
