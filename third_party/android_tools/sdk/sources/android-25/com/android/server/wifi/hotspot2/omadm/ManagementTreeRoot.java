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

package com.android.server.wifi.hotspot2.omadm;

import java.util.Map;

/**
 * A specialized OMAConstructed OMA-DM node used as the MgmtTree root node in Passpoint
 * management trees.
 */
public class ManagementTreeRoot extends OMAConstructed {
    private final String mDtdRev;

    public ManagementTreeRoot(XMLNode node, String dtdRev) {
        super(null, MOTree.MgmtTreeTag, null, new MultiValueMap<OMANode>(),
                node.getTextualAttributes());
        mDtdRev = dtdRev;
    }

    public ManagementTreeRoot(String dtdRev) {
        super(null, MOTree.MgmtTreeTag, null, "xmlns", OMAConstants.SyncML);
        mDtdRev = dtdRev;
    }

    @Override
    public void toXml(StringBuilder sb) {
        sb.append('<').append(MOTree.MgmtTreeTag);
        if (getAttributes() != null && !getAttributes().isEmpty()) {
            for (Map.Entry<String, String> avp : getAttributes().entrySet()) {
                sb.append(' ').append(avp.getKey()).append("=\"")
                        .append(escape(avp.getValue())).append('"');
            }
        }
        sb.append(">\n");

        sb.append('<').append(OMAConstants.SyncMLVersionTag)
                .append('>').append(mDtdRev)
                .append("</").append(OMAConstants.SyncMLVersionTag).append(">\n");
        for (OMANode child : getChildren()) {
            child.toXml(sb);
        }
        sb.append("</").append(MOTree.MgmtTreeTag).append(">\n");
    }
}
