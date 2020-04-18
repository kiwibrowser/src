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

import android.telephony.Rlog;
import java.util.BitSet;
import android.telephony.ServiceState;

/**
 * {@hide}
 *
 * hardware configuration information reported by the ril layer and for
 * use by the telephone framework.
 *
 * the hardware configuration is managed by the TelephonyDevController
 * (aka: the 'TDC').
 *
 * the hardware resources are:
 *    - modem: physical entity providing acces technology.
 *    - sim: physicaly entity providing a slot interface.
 */
public class HardwareConfig {
    static final String LOG_TAG = "HardwareConfig";

    /**
     * hardware configuration kind.
     */
    public static final int DEV_HARDWARE_TYPE_MODEM = 0;
    public static final int DEV_HARDWARE_TYPE_SIM   = 1;
    /**
     * ril attachment model.  if single, there is a one-to-one
     * relationship between a modem hardware and a ril daemon.
     * if multiple, there is a one-to-many relatioship between a
     * modem hardware and several ril simultaneous ril daemons.
     */
    public static final int DEV_MODEM_RIL_MODEL_SINGLE   = 0;
    public static final int DEV_MODEM_RIL_MODEL_MULTIPLE = 1;
    /**
     * hardware state of the resource.
     *
     *   enabled: the resource can be used by the msim-framework,
     *            call activity can be handled on it.
     *   standby: the resource can be used by the msim-framework but
     *            only for non call related activity.  as example:
     *            reading the address book from a sim device. attempt
     *            to use this resource for call activity leads to
     *            undetermined results.
     *   disabled: the resource  cannot be used and attempt to use
     *             it leads to undetermined results.
     *
     * by default, all resources are 'enabled', what causes a resource
     * to be marked otherwise is a property of the underlying hardware
     * knowledge and implementation and it is out of scope of the TDC.
     */
    public static final int DEV_HARDWARE_STATE_ENABLED  = 0;
    public static final int DEV_HARDWARE_STATE_STANDBY  = 1;
    public static final int DEV_HARDWARE_STATE_DISABLED = 2;

    /**
     * common hardware configuration.
     *
     * type - see DEV_HARDWARE_TYPE_
     * uuid - unique identifier for this hardware.
     * state - see DEV_HARDWARE_STATE_
     */
    public int type;
    public String uuid;
    public int state;
    /**
     * following is some specific hardware configuration based on the hardware type.
     */
    /**
     * DEV_HARDWARE_TYPE_MODEM.
     *
     * rilModel - see DEV_MODEM_RIL_MODEL_
     * rat - BitSet value, based on android.telephony.ServiceState
     * maxActiveVoiceCall - maximum number of concurent active voice calls.
     * maxActiveDataCall - maximum number of concurent active data calls.
     * maxStandby - maximum number of concurent standby connections.
     *
     * note: the maxStandby is not necessarily an equal sum of the maxActiveVoiceCall
     * and maxActiveDataCall (nor a derivative of it) since it really depends on the
     * modem capability, hence it is left for the hardware to define.
     */
    public int rilModel;
    public BitSet rat;
    public int maxActiveVoiceCall;
    public int maxActiveDataCall;
    public int maxStandby;
    /**
     * DEV_HARDWARE_TYPE_SIM.
     *
     * modemUuid - unique association to a modem for a sim.
     */
    public String modemUuid;

    /**
     * default constructor.
     */
    public HardwareConfig(int type) {
        type = type;
    }

    /**
     * create from a resource string format.
     */
    public HardwareConfig(String res) {
        String split[] = res.split(",");

        type = Integer.parseInt(split[0]);

        switch (type) {
            case DEV_HARDWARE_TYPE_MODEM: {
                assignModem(
                    split[1].trim(),            /* uuid */
                    Integer.parseInt(split[2]), /* state */
                    Integer.parseInt(split[3]), /* ril-model */
                    Integer.parseInt(split[4]), /* rat */
                    Integer.parseInt(split[5]), /* max-voice */
                    Integer.parseInt(split[6]), /* max-data */
                    Integer.parseInt(split[7])  /* max-standby */
                );
                break;
            }
            case DEV_HARDWARE_TYPE_SIM: {
                assignSim(
                    split[1].trim(),            /* uuid */
                    Integer.parseInt(split[2]), /* state */
                    split[3].trim()             /* modem-uuid */
                );
                break;
            }
        }
    }

    public void assignModem(String id, int state, int model, int ratBits,
        int maxV, int maxD, int maxS) {
        if (type == DEV_HARDWARE_TYPE_MODEM) {
            char[] bits = Integer.toBinaryString(ratBits).toCharArray();
            uuid = id;
            state = state;
            rilModel = model;
            rat = new BitSet(bits.length);
            for (int i = 0 ; i < bits.length ; i++) {
                rat.set(i, (bits[i] == '1' ? true : false));
            }
            maxActiveVoiceCall = maxV;
            maxActiveDataCall = maxD;
            maxStandby = maxS;
        }
    }

    public void assignSim(String id, int state, String link) {
        if (type == DEV_HARDWARE_TYPE_SIM) {
            uuid = id;
            modemUuid = link;
            state = state;
        }
    }

    public String toString() {
        StringBuilder builder = new StringBuilder();
        if (type == DEV_HARDWARE_TYPE_MODEM) {
            builder.append("Modem ");
            builder.append("{ uuid=" + uuid);
            builder.append(", state=" + state);
            builder.append(", rilModel=" + rilModel);
            builder.append(", rat=" + rat.toString());
            builder.append(", maxActiveVoiceCall=" + maxActiveVoiceCall);
            builder.append(", maxActiveDataCall=" + maxActiveDataCall);
            builder.append(", maxStandby=" + maxStandby);
            builder.append(" }");
        } else if (type == DEV_HARDWARE_TYPE_SIM) {
            builder.append("Sim ");
            builder.append("{ uuid=" + uuid);
            builder.append(", modemUuid=" + modemUuid);
            builder.append(", state=" + state);
            builder.append(" }");
        } else {
            builder.append("Invalid Configration");
        }
        return builder.toString();
    }

    public int compareTo(HardwareConfig hw) {
        String one = this.toString();
        String two = hw.toString();

        return (one.compareTo(two));
    }
}
