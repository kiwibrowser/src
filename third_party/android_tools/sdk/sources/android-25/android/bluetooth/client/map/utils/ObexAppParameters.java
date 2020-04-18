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

package android.bluetooth.client.map.utils;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import javax.obex.HeaderSet;

public final class ObexAppParameters {

    private final HashMap<Byte, byte[]> mParams;

    public ObexAppParameters() {
        mParams = new HashMap<Byte, byte[]>();
    }

    public ObexAppParameters(byte[] raw) {
        mParams = new HashMap<Byte, byte[]>();

        if (raw != null) {
            for (int i = 0; i < raw.length;) {
                if (raw.length - i < 2) {
                    break;
                }

                byte tag = raw[i++];
                byte len = raw[i++];

                if (raw.length - i - len < 0) {
                    break;
                }

                byte[] val = new byte[len];

                System.arraycopy(raw, i, val, 0, len);
                this.add(tag, val);

                i += len;
            }
        }
    }

    public static ObexAppParameters fromHeaderSet(HeaderSet headerset) {
        try {
            byte[] raw = (byte[]) headerset.getHeader(HeaderSet.APPLICATION_PARAMETER);
            return new ObexAppParameters(raw);
        } catch (IOException e) {
            // won't happen
        }

        return null;
    }

    public byte[] getHeader() {
        int length = 0;

        for (Map.Entry<Byte, byte[]> entry : mParams.entrySet()) {
            length += (entry.getValue().length + 2);
        }

        byte[] ret = new byte[length];

        int idx = 0;
        for (Map.Entry<Byte, byte[]> entry : mParams.entrySet()) {
            length = entry.getValue().length;

            ret[idx++] = entry.getKey();
            ret[idx++] = (byte) length;
            System.arraycopy(entry.getValue(), 0, ret, idx, length);
            idx += length;
        }

        return ret;
    }

    public void addToHeaderSet(HeaderSet headerset) {
        if (mParams.size() > 0) {
            headerset.setHeader(HeaderSet.APPLICATION_PARAMETER, getHeader());
        }
    }

    public boolean exists(byte tag) {
        return mParams.containsKey(tag);
    }

    public void add(byte tag, byte val) {
        byte[] bval = ByteBuffer.allocate(1).put(val).array();
        mParams.put(tag, bval);
    }

    public void add(byte tag, short val) {
        byte[] bval = ByteBuffer.allocate(2).putShort(val).array();
        mParams.put(tag, bval);
    }

    public void add(byte tag, int val) {
        byte[] bval = ByteBuffer.allocate(4).putInt(val).array();
        mParams.put(tag, bval);
    }

    public void add(byte tag, long val) {
        byte[] bval = ByteBuffer.allocate(8).putLong(val).array();
        mParams.put(tag, bval);
    }

    public void add(byte tag, String val) {
        byte[] bval = val.getBytes();
        mParams.put(tag, bval);
    }

    public void add(byte tag, byte[] bval) {
        mParams.put(tag, bval);
    }

    public byte getByte(byte tag) {
        byte[] bval = mParams.get(tag);

        if (bval == null || bval.length < 1) {
            return 0;
        }

        return ByteBuffer.wrap(bval).get();
    }

    public short getShort(byte tag) {
        byte[] bval = mParams.get(tag);

        if (bval == null || bval.length < 2) {
            return 0;
        }

        return ByteBuffer.wrap(bval).getShort();
    }

    public int getInt(byte tag) {
        byte[] bval = mParams.get(tag);

        if (bval == null || bval.length < 4) {
            return 0;
        }

        return ByteBuffer.wrap(bval).getInt();
    }

    public String getString(byte tag) {
        byte[] bval = mParams.get(tag);

        if (bval == null) {
            return null;
        }

        return new String(bval);
    }

    public byte[] getByteArray(byte tag) {
        byte[] bval = mParams.get(tag);

        return bval;
    }

    @Override
    public String toString() {
        return mParams.toString();
    }
}
