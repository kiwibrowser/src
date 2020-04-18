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
import com.android.vcard.VCardEntry.EmailData;
import com.android.vcard.VCardEntry.NameData;
import com.android.vcard.VCardEntry.PhoneData;

import java.util.List;

class BluetoothMapBmessageBuilder {

    private final static String CRLF = "\r\n";

    private final static String BMSG_BEGIN = "BEGIN:BMSG";
    private final static String BMSG_VERSION = "VERSION:1.0";
    private final static String BMSG_STATUS = "STATUS:";
    private final static String BMSG_TYPE = "TYPE:";
    private final static String BMSG_FOLDER = "FOLDER:";
    private final static String BMSG_END = "END:BMSG";

    private final static String BENV_BEGIN = "BEGIN:BENV";
    private final static String BENV_END = "END:BENV";

    private final static String BBODY_BEGIN = "BEGIN:BBODY";
    private final static String BBODY_ENCODING = "ENCODING:";
    private final static String BBODY_CHARSET = "CHARSET:";
    private final static String BBODY_LANGUAGE = "LANGUAGE:";
    private final static String BBODY_LENGTH = "LENGTH:";
    private final static String BBODY_END = "END:BBODY";

    private final static String MSG_BEGIN = "BEGIN:MSG";
    private final static String MSG_END = "END:MSG";

    private final static String VCARD_BEGIN = "BEGIN:VCARD";
    private final static String VCARD_VERSION = "VERSION:2.1";
    private final static String VCARD_N = "N:";
    private final static String VCARD_EMAIL = "EMAIL:";
    private final static String VCARD_TEL = "TEL:";
    private final static String VCARD_END = "END:VCARD";

    private final StringBuilder mBmsg;

    private BluetoothMapBmessageBuilder() {
        mBmsg = new StringBuilder();
    }

    static public String createBmessage(BluetoothMapBmessage bmsg) {
        BluetoothMapBmessageBuilder b = new BluetoothMapBmessageBuilder();

        b.build(bmsg);

        return b.mBmsg.toString();
    }

    private void build(BluetoothMapBmessage bmsg) {
        int bodyLen = MSG_BEGIN.length() + MSG_END.length() + 3 * CRLF.length()
                + bmsg.mMessage.getBytes().length;

        mBmsg.append(BMSG_BEGIN).append(CRLF);

        mBmsg.append(BMSG_VERSION).append(CRLF);
        mBmsg.append(BMSG_STATUS).append(bmsg.mBmsgStatus).append(CRLF);
        mBmsg.append(BMSG_TYPE).append(bmsg.mBmsgType).append(CRLF);
        mBmsg.append(BMSG_FOLDER).append(bmsg.mBmsgFolder).append(CRLF);

        for (VCardEntry vcard : bmsg.mOriginators) {
            buildVcard(vcard);
        }

        {
            mBmsg.append(BENV_BEGIN).append(CRLF);

            for (VCardEntry vcard : bmsg.mRecipients) {
                buildVcard(vcard);
            }

            {
                mBmsg.append(BBODY_BEGIN).append(CRLF);

                if (bmsg.mBbodyEncoding != null) {
                    mBmsg.append(BBODY_ENCODING).append(bmsg.mBbodyEncoding).append(CRLF);
                }

                if (bmsg.mBbodyCharset != null) {
                    mBmsg.append(BBODY_CHARSET).append(bmsg.mBbodyCharset).append(CRLF);
                }

                if (bmsg.mBbodyLanguage != null) {
                    mBmsg.append(BBODY_LANGUAGE).append(bmsg.mBbodyLanguage).append(CRLF);
                }

                mBmsg.append(BBODY_LENGTH).append(bodyLen).append(CRLF);

                {
                    mBmsg.append(MSG_BEGIN).append(CRLF);

                    mBmsg.append(bmsg.mMessage).append(CRLF);

                    mBmsg.append(MSG_END).append(CRLF);
                }

                mBmsg.append(BBODY_END).append(CRLF);
            }

            mBmsg.append(BENV_END).append(CRLF);
        }

        mBmsg.append(BMSG_END).append(CRLF);
    }

    private void buildVcard(VCardEntry vcard) {
        String n = buildVcardN(vcard);
        List<PhoneData> tel = vcard.getPhoneList();
        List<EmailData> email = vcard.getEmailList();

        mBmsg.append(VCARD_BEGIN).append(CRLF);

        mBmsg.append(VCARD_VERSION).append(CRLF);

        mBmsg.append(VCARD_N).append(n).append(CRLF);

        if (tel != null && tel.size() > 0) {
            mBmsg.append(VCARD_TEL).append(tel.get(0).getNumber()).append(CRLF);
        }

        if (email != null && email.size() > 0) {
            mBmsg.append(VCARD_EMAIL).append(email.get(0).getAddress()).append(CRLF);
        }

        mBmsg.append(VCARD_END).append(CRLF);
    }

    private String buildVcardN(VCardEntry vcard) {
        NameData nd = vcard.getNameData();
        StringBuilder sb = new StringBuilder();

        sb.append(nd.getFamily()).append(";");
        sb.append(nd.getGiven() == null ? "" : nd.getGiven()).append(";");
        sb.append(nd.getMiddle() == null ? "" : nd.getMiddle()).append(";");
        sb.append(nd.getPrefix() == null ? "" : nd.getPrefix()).append(";");
        sb.append(nd.getSuffix() == null ? "" : nd.getSuffix());

        return sb.toString();
    }
}
