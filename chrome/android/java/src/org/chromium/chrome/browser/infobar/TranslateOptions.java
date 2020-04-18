// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.text.TextUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A class that keeps the state of the different translation options and
 * languages.
 */
public class TranslateOptions {
    /**
     * A container for Language Code and it's translated representation and it's native UMA
     * specific hashcode.
     * For example for Spanish when viewed from a French locale, this will contain es, Espagnol,
     * 114573335
     **/
    public static class TranslateLanguageData {
        public final String mLanguageCode;
        public final String mLanguageRepresentation;
        public final Integer mLanguageUMAHashCode;

        public TranslateLanguageData(
                String languageCode, String languageRepresentation, Integer uMAhashCode) {
            assert languageCode != null;
            assert languageRepresentation != null;
            mLanguageCode = languageCode;
            mLanguageRepresentation = languageRepresentation;
            mLanguageUMAHashCode = uMAhashCode;
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof TranslateLanguageData)) {
                return false;
            }
            TranslateLanguageData other = (TranslateLanguageData) obj;
            return this.mLanguageCode.equals(other.mLanguageCode)
                    && this.mLanguageRepresentation.equals(other.mLanguageRepresentation)
                    && this.mLanguageUMAHashCode.equals(other.mLanguageUMAHashCode);
        }

        @Override
        public int hashCode() {
            return (mLanguageCode + mLanguageRepresentation).hashCode();
        }

        @Override
        public String toString() {
            return "mLanguageCode:" + mLanguageCode + " - mlanguageRepresentation "
                    + mLanguageRepresentation + " - mLanguageUMAHashCode " + mLanguageUMAHashCode;
        }
    }

    // This would be an enum but they are not good for mobile.
    // The checkBoundaries method below needs to be updated if new options are added.
    private static final int NEVER_LANGUAGE = 0;
    private static final int NEVER_DOMAIN = 1;
    private static final int ALWAYS_LANGUAGE = 2;

    private String mSourceLanguageCode;
    private String mTargetLanguageCode;

    private final ArrayList<TranslateLanguageData> mAllLanguages;

    // language code to translated language name map
    // Conceptually final
    private Map<String, String> mCodeToRepresentation;

    // Langage code to its UMA hashcode representation.
    private Map<String, Integer> mCodeToUMAHashCode;

    // Will reflect the state before the object was ever modified
    private final boolean[] mOriginalOptions;

    private final String mOriginalSourceLanguageCode;
    private final String mOriginalTargetLanguageCode;
    private final boolean mTriggeredFromMenu;

    private final boolean[] mOptions;

    private TranslateOptions(String sourceLanguageCode, String targetLanguageCode,
            ArrayList<TranslateLanguageData> allLanguages, boolean neverLanguage,
            boolean neverDomain, boolean alwaysLanguage, boolean triggeredFromMenu,
            boolean[] originalOptions) {
        mOptions = new boolean[3];
        mOptions[NEVER_LANGUAGE] = neverLanguage;
        mOptions[NEVER_DOMAIN] = neverDomain;
        mOptions[ALWAYS_LANGUAGE] = alwaysLanguage;

        if (originalOptions == null) {
            mOriginalOptions = mOptions.clone();
        } else {
            mOriginalOptions = originalOptions.clone();
        }

        mSourceLanguageCode = sourceLanguageCode;
        mTargetLanguageCode = targetLanguageCode;
        mOriginalSourceLanguageCode = mSourceLanguageCode;
        mOriginalTargetLanguageCode = mTargetLanguageCode;
        mTriggeredFromMenu = triggeredFromMenu;

        mAllLanguages = allLanguages;
        mCodeToRepresentation = new HashMap<String, String>();
        mCodeToUMAHashCode = new HashMap<String, Integer>();
        for (TranslateLanguageData language : allLanguages) {
            mCodeToRepresentation.put(language.mLanguageCode, language.mLanguageRepresentation);
            mCodeToUMAHashCode.put(language.mLanguageCode, language.mLanguageUMAHashCode);
        }
    }

    /**
     * Creates a TranslateOptions by the given data.
     */
    public static TranslateOptions create(String sourceLanguageCode, String targetLanguageCode,
            String[] languages, String[] codes, boolean alwaysTranslate, boolean triggeredFromMenu,
            int[] hashCodes) {
        assert languages.length == codes.length;

        ArrayList<TranslateLanguageData> languageList = new ArrayList<TranslateLanguageData>();
        for (int i = 0; i < languages.length; ++i) {
            Integer hashCode = null;
            if (hashCodes != null) {
                hashCode = Integer.valueOf(hashCodes[i]);
            }

            languageList.add(new TranslateLanguageData(codes[i], languages[i], hashCode));
        }
        return new TranslateOptions(sourceLanguageCode, targetLanguageCode, languageList, false,
                false, alwaysTranslate, triggeredFromMenu, null);
    }

    /**
     * Returns a copy of the current instance.
     */
    TranslateOptions copy() {
        return new TranslateOptions(mSourceLanguageCode, mTargetLanguageCode, mAllLanguages,
                mOptions[NEVER_LANGUAGE], mOptions[NEVER_DOMAIN], mOptions[ALWAYS_LANGUAGE],
                mTriggeredFromMenu, mOriginalOptions);
    }

    public String sourceLanguageName() {
        return getRepresentationFromCode(mSourceLanguageCode);
    }

    public String targetLanguageName() {
        return getRepresentationFromCode(mTargetLanguageCode);
    }

    public String sourceLanguageCode() {
        return mSourceLanguageCode;
    }

    public String targetLanguageCode() {
        return mTargetLanguageCode;
    }

    public boolean triggeredFromMenu() {
        return mTriggeredFromMenu;
    }

    public boolean optionsChanged() {
        return (!mSourceLanguageCode.equals(mOriginalSourceLanguageCode))
                || (!mTargetLanguageCode.equals(mOriginalTargetLanguageCode))
                || (mOptions[NEVER_LANGUAGE] != mOriginalOptions[NEVER_LANGUAGE])
                || (mOptions[NEVER_DOMAIN] != mOriginalOptions[NEVER_DOMAIN])
                || (mOptions[ALWAYS_LANGUAGE] != mOriginalOptions[ALWAYS_LANGUAGE]);
    }

    public List<TranslateLanguageData> allLanguages() {
        return mAllLanguages;
    }

    public boolean neverTranslateLanguageState() {
        return mOptions[NEVER_LANGUAGE];
    }

    public boolean alwaysTranslateLanguageState() {
        return mOptions[ALWAYS_LANGUAGE];
    }

    public boolean neverTranslateDomainState() {
        return mOptions[NEVER_DOMAIN];
    }

    public boolean setSourceLanguage(String languageCode) {
        boolean canSet = canSetLanguage(languageCode, mTargetLanguageCode);
        if (canSet) {
            mSourceLanguageCode = languageCode;
        }
        return canSet;
    }

    public boolean setTargetLanguage(String languageCode) {
        boolean canSet = canSetLanguage(mSourceLanguageCode, languageCode);
        if (canSet) {
            mTargetLanguageCode = languageCode;
        }
        return canSet;
    }

    /**
     * Sets the new state of never translate domain.
     *
     * @return true if the toggling was possible
     */
    public boolean toggleNeverTranslateDomainState(boolean value) {
        return toggleState(NEVER_DOMAIN, value);
    }

    /**
     * Sets the new state of never translate language.
     *
     * @return true if the toggling was possible
     */
    public boolean toggleNeverTranslateLanguageState(boolean value) {
        // Do not toggle if we are activating NeverLanguge but AlwaysTranslate
        // for a language pair with the same source language is already active.
        if (mOptions[ALWAYS_LANGUAGE] && value) {
            return false;
        }
        return toggleState(NEVER_LANGUAGE, value);
    }

    /**
     * Sets the new state of never translate a language pair.
     *
     * @return true if the toggling was possible
     */
    public boolean toggleAlwaysTranslateLanguageState(boolean value) {
        // Do not toggle if we are activating AlwaysLanguge but NeverLanguage is active already.
        if (mOptions[NEVER_LANGUAGE] && value) {
            return false;
        }
        return toggleState(ALWAYS_LANGUAGE, value);
    }

    /**
     * Gets the language's translated representation from a given language code.
     * @param languageCode ISO code for the language
     * @return The translated representation of the language, or "" if not found.
     */
    public String getRepresentationFromCode(String languageCode) {
        if (isValidLanguageCode(languageCode)) {
            return mCodeToRepresentation.get(languageCode);
        }
        return "";
    }

    /**
     * Gets the language's UMA hashcode representation from a given language code.
     * @param languageCode ISO code for the language
     * @return The UMA hashcode representation of the language, or null if not found.
     */
    public Integer getUMAHashCodeFromCode(String languageCode) {
        if (isValidLanguageUMAHashCode(languageCode)) {
            return mCodeToUMAHashCode.get(languageCode);
        }
        return null;
    }

    private boolean toggleState(int element, boolean newValue) {
        if (!checkElementBoundaries(element)) return false;

        mOptions[element] = newValue;
        return true;
    }

    private boolean isValidLanguageCode(String languageCode) {
        return !TextUtils.isEmpty(languageCode) && mCodeToRepresentation.containsKey(languageCode);
    }

    private boolean isValidLanguageUMAHashCode(String languageCode) {
        return !TextUtils.isEmpty(languageCode) && mCodeToUMAHashCode.containsKey(languageCode);
    }

    private boolean canSetLanguage(String sourceCode, String targetCode) {
        return isValidLanguageCode(sourceCode) && isValidLanguageCode(targetCode)
                && !sourceCode.equals(targetCode);
    }

    private static boolean checkElementBoundaries(int element) {
        return element >= NEVER_LANGUAGE && element <= ALWAYS_LANGUAGE;
    }

    @Override
    public String toString() {
        return new StringBuilder()
                .append(sourceLanguageCode())
                .append(" -> ")
                .append(targetLanguageCode())
                .append(" - ")
                .append("Never Language:")
                .append(mOptions[NEVER_LANGUAGE])
                .append(" Always Language:")
                .append(mOptions[ALWAYS_LANGUAGE])
                .append(" Never Domain:")
                .append(mOptions[NEVER_DOMAIN])
                .toString();
    }
}
