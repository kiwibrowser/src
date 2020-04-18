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
 * limitations under the License
 */
package com.android.internal.telephony;

import android.annotation.Nullable;
import android.os.Bundle;

public class VisualVoicemailSmsParser {

    private static final String[] ALLOWED_ALTERNATIVE_FORMAT_EVENT = new String[] {
            "MBOXUPDATE", "UNRECOGNIZED"
    };

    /**
     * Class wrapping the raw OMTP message data, internally represented as as map of all key-value
     * pairs found in the SMS body. <p> All the methods return null if either the field was not
     * present or it could not be parsed.
     */
    public static class WrappedMessageData {

        public final String prefix;
        public final Bundle fields;

        @Override
        public String toString() {
            return "WrappedMessageData [type=" + prefix + " fields=" + fields + "]";
        }

        WrappedMessageData(String prefix, Bundle keyValues) {
            this.prefix = prefix;
            fields = keyValues;
        }
    }

    /**
     * Parses the supplied SMS body and returns back a structured OMTP message. Returns null if
     * unable to parse the SMS body.
     */
    @Nullable
    public static WrappedMessageData parse(String clientPrefix, String smsBody) {
        try {
            if (!smsBody.startsWith(clientPrefix)) {
                return null;
            }
            int prefixEnd = clientPrefix.length();
            if (!(smsBody.charAt(prefixEnd) == ':')) {
                return null;
            }
            int eventTypeEnd = smsBody.indexOf(":", prefixEnd + 1);
            if (eventTypeEnd == -1) {
                return null;
            }
            String eventType = smsBody.substring(prefixEnd + 1, eventTypeEnd);
            Bundle fields = parseSmsBody(smsBody.substring(eventTypeEnd + 1));
            if (fields == null) {
                return null;
            }
            return new WrappedMessageData(eventType, fields);
        } catch (IndexOutOfBoundsException e) {
            return null;
        }
    }

    /**
     * Converts a String of key/value pairs into a Map object. The WrappedMessageData object
     * contains helper functions to retrieve the values.
     *
     * e.g. "//VVM:STATUS:st=R;rc=0;srv=1;dn=1;ipt=1;spt=0;u=eg@example.com;pw=1" =>
     * "WrappedMessageData [fields={st=R, ipt=1, srv=1, dn=1, u=eg@example.com, pw=1, rc=0}]"
     *
     * @param message The sms string with the prefix removed.
     * @return A WrappedMessageData object containing the map.
     */
    @Nullable
    private static Bundle parseSmsBody(String message) {
        // TODO: ensure fail if format does not match
        Bundle keyValues = new Bundle();
        String[] entries = message.split(";");
        for (String entry : entries) {
            if (entry.length() == 0) {
                continue;
            }
            // The format for a field is <key>=<value>.
            // As the OMTP spec both key and value are required, but in some cases carriers will
            // send an SMS with missing value, so only the presence of the key is enforced.
            // For example, an SMS for a voicemail from restricted number might have "s=" for the
            // sender field, instead of omitting the field.
            int separatorIndex = entry.indexOf("=");
            if (separatorIndex == -1 || separatorIndex == 0) {
                // No separator or no key.
                // For example "foo" or "=value".
                // A VVM SMS should have all of its' field valid.
                return null;
            }
            String key = entry.substring(0, separatorIndex);
            String value = entry.substring(separatorIndex + 1);
            keyValues.putString(key, value);
        }

        return keyValues;
    }

    /**
     * The alternative format is [Event]?([key]=[value])*, for example
     *
     * <p>"MBOXUPDATE?m=1;server=example.com;port=143;name=foo@example.com;pw=foo".
     *
     * <p>This format is not protected with a client prefix and should be handled with care. For
     * safety, the event type must be one of {@link #ALLOWED_ALTERNATIVE_FORMAT_EVENT}
     */
    @Nullable
    public static WrappedMessageData parseAlternativeFormat(String smsBody) {
        try {
            int eventTypeEnd = smsBody.indexOf("?");
            if (eventTypeEnd == -1) {
                return null;
            }
            String eventType = smsBody.substring(0, eventTypeEnd);
            if (!isAllowedAlternativeFormatEvent(eventType)) {
                return null;
            }
            Bundle fields = parseSmsBody(smsBody.substring(eventTypeEnd + 1));
            if (fields == null) {
                return null;
            }
            return new WrappedMessageData(eventType, fields);
        } catch (IndexOutOfBoundsException e) {
            return null;
        }
    }

    private static boolean isAllowedAlternativeFormatEvent(String eventType) {
        for (String event : ALLOWED_ALTERNATIVE_FORMAT_EVENT) {
            if (event.equals(eventType)) {
                return true;
            }
        }
        return false;
    }
}
