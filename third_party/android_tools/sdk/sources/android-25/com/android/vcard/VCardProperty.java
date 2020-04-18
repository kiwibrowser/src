/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.vcard;

import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

/**
 * Represents vCard's property, or logical "one line" of each vCard entry.
 *
 * e.g.
 * Given a vCard below, objects for <code>N:name</code>, <code>TEL:1111111111</code> are
 * "property".
 *
 * <code>
 * BEGIN:VCARD
 * N:name
 * TEL:1111111111
 * END:VCARD
 * </code>
 *
 * vCard's property has three elements: name, parameter (or param), and value. Name is the name
 * of each property. Parameter or param is additional information for the property. Value is one
 * or multiple values representing the parameter.
 *
 * e.g.
 * <code>N;CHARSET=UTF-8:Joe;Due;M.;Mr.;Ph.D.</code>, has "N" for name, "CHALSET=UTF-8" for param,
 * and "Joe;Due;M.;Mr.;Ph.D." for value.
 *
 * Usually params are represented as "key=value" pair, but not always
 * (The property <code>TEL;WORK;VOICE:(111) 555-1212</code> has two params without key "TYPE",
 * which are same as "TYPE=WORK" and "TYPE=VOICE". In vCard 3.0, we can even express them as
 * "TYPE=WORK,VOICE").
 *
 * Sometimes (not always) value can be separated by semi-colon. In the example above "Joe;Due;;;"
 * should be interpreted as five strings: "Joe" (for family name), "Due" (for given name), "M."
 * (for middle name), "Mr." (for prefix), and "Ph.D." (for suffix). Whether the value is
 * separable or not is specified by vCard specs.
 */
public class VCardProperty {
    private static final String LOG_TAG = VCardConstants.LOG_TAG;
    private String mName;
    private List<String> mGroupList;

    private Map<String, Collection<String>> mParameterMap =
            new HashMap<String, Collection<String>>();
    private String mRawValue;

    private List<String> mValueList;
    private byte[] mByteValue;

    public void setName(String name) {
        if (mName != null) {
            Log.w(LOG_TAG, String.format("Property name is re-defined " +
                    "(existing: %s, requested: %s", mName, name));
        }
        mName = name;
    }

    public void addGroup(String group) {
        if (mGroupList == null) {
            mGroupList = new ArrayList<String>();
        }
        mGroupList.add(group);
    }

    public void setParameter(final String paramName, final String paramValue) {
        mParameterMap.clear();
        addParameter(paramName, paramValue);
    }

    public void addParameter(final String paramName, final String paramValue) {
        Collection<String> values;
        if (!mParameterMap.containsKey(paramName)) {
            if (paramName.equals("TYPE")) {
                values = new HashSet<String>();
            } else {
                values = new ArrayList<String>();
            }
            mParameterMap.put(paramName, values);
        } else {
            values = mParameterMap.get(paramName);
        }
        values.add(paramValue);
    }

    public void setRawValue(String rawValue) {
        mRawValue = rawValue;
    }

    // TODO: would be much better to have translateRawValue() functionality instead of forcing
    // VCardParserImpl does this job.

    public void setValues(String... propertyValues) {
        mValueList = Arrays.asList(propertyValues);
    }

    public void setValues(List<String> propertyValueList) {
        mValueList = propertyValueList;
    }

    public void addValues(String... propertyValues) {
        if (mValueList == null) {
            mValueList = Arrays.asList(propertyValues);
        } else {
            mValueList.addAll(Arrays.asList(propertyValues));
        }
    }

    public void addValues(List<String> propertyValueList) {
        if (mValueList == null) {
            mValueList = new ArrayList<String>(propertyValueList);
        } else {
            mValueList.addAll(propertyValueList);
        }
    }

    public void setByteValue(byte[] byteValue) {
        mByteValue = byteValue;
    }

    public String getName() {
        return mName;
    }

    public List<String> getGroupList() {
        return mGroupList;
    }

    public Map<String, Collection<String>> getParameterMap() {
        return mParameterMap;
    }

    public Collection<String> getParameters(String type) {
        return mParameterMap.get(type);
    }

    public String getRawValue() {
        return mRawValue;
    }

    public List<String> getValueList() {
        return mValueList;
    }

    public byte[] getByteValue() {
        return mByteValue;
    }
}


