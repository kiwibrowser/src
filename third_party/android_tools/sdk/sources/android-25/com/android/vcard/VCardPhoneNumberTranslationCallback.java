/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.vcard;

/**
 * Callback functionality which can be used with
 * {@link VCardComposer#setPhoneNumberTranslationCallback(VCardPhoneNumberTranslationCallback)}.
 * See the doc for the method.
 *
 * <p>
 * TODO: this should be more generic
 * </p>
 *
 * @hide This will change
 */
public interface VCardPhoneNumberTranslationCallback {
    /**
     * Called when a phone number is being handled.
     * @return formatted phone number.
     */
    public String onValueReceived(String rawValue, int type, String label, boolean isPrimary);
}
