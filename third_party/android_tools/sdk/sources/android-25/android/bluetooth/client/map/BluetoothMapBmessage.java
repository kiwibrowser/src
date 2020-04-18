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

package android.bluetooth.client.map;

import com.android.vcard.VCardEntry;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

/**
 * Object representation of message in bMessage format
 * <p>
 * This object will be received in {@link BluetoothMasClient#EVENT_GET_MESSAGE}
 * callback message.
 */
public class BluetoothMapBmessage {

    String mBmsgVersion;
    Status mBmsgStatus;
    Type mBmsgType;
    String mBmsgFolder;

    String mBbodyEncoding;
    String mBbodyCharset;
    String mBbodyLanguage;
    int mBbodyLength;

    String mMessage;

    ArrayList<VCardEntry> mOriginators;
    ArrayList<VCardEntry> mRecipients;

    public enum Status {
        READ, UNREAD
    }

    public enum Type {
        EMAIL, SMS_GSM, SMS_CDMA, MMS
    }

    /**
     * Constructs empty message object
     */
    public BluetoothMapBmessage() {
        mOriginators = new ArrayList<VCardEntry>();
        mRecipients = new ArrayList<VCardEntry>();
    }

    public VCardEntry getOriginator() {
        if (mOriginators.size() > 0) {
            return mOriginators.get(0);
        } else {
            return null;
        }
    }

    public ArrayList<VCardEntry> getOriginators() {
        return mOriginators;
    }

    public BluetoothMapBmessage addOriginator(VCardEntry vcard) {
        mOriginators.add(vcard);
        return this;
    }

    public ArrayList<VCardEntry> getRecipients() {
        return mRecipients;
    }

    public BluetoothMapBmessage addRecipient(VCardEntry vcard) {
        mRecipients.add(vcard);
        return this;
    }

    public Status getStatus() {
        return mBmsgStatus;
    }

    public BluetoothMapBmessage setStatus(Status status) {
        mBmsgStatus = status;
        return this;
    }

    public Type getType() {
        return mBmsgType;
    }

    public BluetoothMapBmessage setType(Type type) {
        mBmsgType = type;
        return this;
    }

    public String getFolder() {
        return mBmsgFolder;
    }

    public BluetoothMapBmessage setFolder(String folder) {
        mBmsgFolder = folder;
        return this;
    }

    public String getEncoding() {
        return mBbodyEncoding;
    }

    public BluetoothMapBmessage setEncoding(String encoding) {
        mBbodyEncoding = encoding;
        return this;
    }

    public String getCharset() {
        return mBbodyCharset;
    }

    public BluetoothMapBmessage setCharset(String charset) {
        mBbodyCharset = charset;
        return this;
    }

    public String getLanguage() {
        return mBbodyLanguage;
    }

    public BluetoothMapBmessage setLanguage(String language) {
        mBbodyLanguage = language;
        return this;
    }

    public String getBodyContent() {
        return mMessage;
    }

    public BluetoothMapBmessage setBodyContent(String body) {
        mMessage = body;
        return this;
    }

    @Override
    public String toString() {
        JSONObject json = new JSONObject();

        try {
            json.put("status", mBmsgStatus);
            json.put("type", mBmsgType);
            json.put("folder", mBmsgFolder);
            json.put("charset", mBbodyCharset);
            json.put("message", mMessage);
        } catch (JSONException e) {
            // do nothing
        }

        return json.toString();
    }
}
