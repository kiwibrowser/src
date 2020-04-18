/*
 * Copyright (C) 2009 The Android Open Source Project
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

import java.util.List;

/**
 * <P>
 * The interface which should be implemented by the classes which have to analyze each
 * vCard entry minutely.
 * </P>
 * <P>
 * Here, there are several terms specific to vCard (and this library).
 * </P>
 * <P>
 * The term "entry" is one vCard representation in the input, which should start with "BEGIN:VCARD"
 * and end with "END:VCARD".
 * </P>
 * <P>
 * The term "property" is one line in vCard entry, which consists of "group", "property name",
 * "parameter(param) names and values", and "property values".
 * </P>
 * <P>
 * e.g. group1.propName;paramName1=paramValue1;paramName2=paramValue2;propertyValue1;propertyValue2...
 * </P>
 */
public interface VCardInterpreter {
    /**
     * Called when vCard interpretation started.
     */
    void onVCardStarted();

    /**
     * Called when vCard interpretation finished.
     */
    void onVCardEnded();

    /**
     * Called when parsing one vCard entry started.
     * More specifically, this method is called when "BEGIN:VCARD" is read.
     *
     * This may be called before {@link #onEntryEnded()} is called, as vCard 2.1 accepts nested
     * vCard.
     *
     * <code>
     * BEGIN:VCARD
     * BEGIN:VCARD
     * VERSION:2.1
     * N:test;;;;
     * END:VCARD
     * END:VCARD
     * </code>
     */
    void onEntryStarted();

    /**
     * Called when parsing one vCard entry ended.
     * More specifically, this method is called when "END:VCARD" is read.
     */
    void onEntryEnded();

    /**
     * Called when a property is created.
     */
    void onPropertyCreated(VCardProperty property);
}
