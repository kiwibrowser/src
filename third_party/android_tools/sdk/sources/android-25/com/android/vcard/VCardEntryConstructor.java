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

import android.accounts.Account;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * <p>
 * The {@link VCardInterpreter} implementation which enables {@link VCardEntryHandler} objects
 * to easily handle each vCard entry.
 * </p>
 * <p>
 * This class understand details inside vCard and translates it to {@link VCardEntry}.
 * Then the class throw it to {@link VCardEntryHandler} registered via
 * {@link #addEntryHandler(VCardEntryHandler)}, so that all those registered objects
 * are able to handle the {@link VCardEntry} object.
 * </p>
 * <p>
 * If you want to know the detail inside vCard, it would be better to implement
 * {@link VCardInterpreter} directly, instead of relying on this class and
 * {@link VCardEntry} created by the object.
 * </p>
 */
public class VCardEntryConstructor implements VCardInterpreter {
    private static String LOG_TAG = VCardConstants.LOG_TAG;

    /**
     * Represents current stack of VCardEntry. Used to support nested vCard (vCard 2.1).
     */
    private final List<VCardEntry> mEntryStack = new ArrayList<VCardEntry>();
    private VCardEntry mCurrentEntry;

    private final int mVCardType;
    private final Account mAccount;

    private final List<VCardEntryHandler> mEntryHandlers = new ArrayList<VCardEntryHandler>();

    public VCardEntryConstructor() {
        this(VCardConfig.VCARD_TYPE_V21_GENERIC, null, null);
    }

    public VCardEntryConstructor(final int vcardType) {
        this(vcardType, null, null);
    }

    public VCardEntryConstructor(final int vcardType, final Account account) {
        this(vcardType, account, null);
    }

    /**
     * @deprecated targetCharset is not used anymore.
     * Use {@link #VCardEntryConstructor(int, Account)}
     */
    @Deprecated
    public VCardEntryConstructor(final int vcardType, final Account account,
            String targetCharset) {
        mVCardType = vcardType;
        mAccount = account;
    }

    public void addEntryHandler(VCardEntryHandler entryHandler) {
        mEntryHandlers.add(entryHandler);
    }

    @Override
    public void onVCardStarted() {
        for (VCardEntryHandler entryHandler : mEntryHandlers) {
            entryHandler.onStart();
        }
    }

    @Override
    public void onVCardEnded() {
        for (VCardEntryHandler entryHandler : mEntryHandlers) {
            entryHandler.onEnd();
        }
    }

    public void clear() {
        mCurrentEntry = null;
        mEntryStack.clear();
    }

    @Override
    public void onEntryStarted() {
        mCurrentEntry = new VCardEntry(mVCardType, mAccount);
        mEntryStack.add(mCurrentEntry);
    }

    @Override
    public void onEntryEnded() {
        mCurrentEntry.consolidateFields();
        for (VCardEntryHandler entryHandler : mEntryHandlers) {
            entryHandler.onEntryCreated(mCurrentEntry);
        }

        final int size = mEntryStack.size();
        if (size > 1) {
            VCardEntry parent = mEntryStack.get(size - 2);
            parent.addChild(mCurrentEntry);
            mCurrentEntry = parent;
        } else {
            mCurrentEntry = null;
        }
        mEntryStack.remove(size - 1);
    }

    @Override
    public void onPropertyCreated(VCardProperty property) {
        mCurrentEntry.addProperty(property);
    }
}
