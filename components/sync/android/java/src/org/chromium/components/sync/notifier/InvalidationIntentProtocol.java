// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.sync.notifier;

import android.accounts.Account;
import android.content.Intent;

import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.protos.ipc.invalidation.Types;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.CollectionUtil;
import org.chromium.components.sync.ModelTypeHelper;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

/**
 * Constants and utility methods to create the intents used to communicate between the
 * controller and the invalidation client library.
 */
public class InvalidationIntentProtocol {
    /**
     * Action set on register intents.
     */
    public static final String ACTION_REGISTER =
            "org.chromium.components.sync.notifier.ACTION_REGISTER_TYPES";

    /**
     * Parcelable-valued intent extra containing the account of the user.
     */
    public static final String EXTRA_ACCOUNT = "account";

    /**
     * String-list-valued intent extra of the syncable types to sync.
     */
    public static final String EXTRA_REGISTERED_TYPES = "registered_types";

    /**
     * Int-array-valued intent extra containing sources of objects to register for.
     * The array is parallel to EXTRA_REGISTERED_OBJECT_NAMES.
     */
    public static final String EXTRA_REGISTERED_OBJECT_SOURCES = "registered_object_sources";

    /**
     * String-array-valued intent extra containing names of objects to register for.
     * The array is parallel to EXTRA_REGISTERED_OBJECT_SOURCES.
     */
    public static final String EXTRA_REGISTERED_OBJECT_NAMES = "registered_object_names";

    /**
     * Boolean-valued intent extra indicating that the service should be stopped.
     */
    public static final String EXTRA_STOP = "stop";

    /**
     * Create an Intent that will start the invalidation listener service and
     * register for the specified types.
     */
    public static Intent createRegisterIntent(Account account, Set<Integer> types) {
        Intent registerIntent = new Intent(ACTION_REGISTER);
        String[] selectedTypesArray = new String[types.size()];
        int pos = 0;
        for (Integer type : types) {
            selectedTypesArray[pos++] = ModelTypeHelper.toNotificationType(type);
        }
        registerIntent.putStringArrayListExtra(
                EXTRA_REGISTERED_TYPES, CollectionUtil.newArrayList(selectedTypesArray));
        registerIntent.putExtra(EXTRA_ACCOUNT, account);
        return registerIntent;
    }

    /**
     * Create an Intent that will start the invalidation listener service and
     * register for the object ids with the specified sources and names.
     * Sync-specific objects are filtered out of the request since Sync types
     * are registered using the other version of createRegisterIntent.
     */
    public static Intent createRegisterIntent(
            Account account, int[] objectSources, String[] objectNames) {
        if (objectSources.length != objectNames.length) {
            throw new IllegalArgumentException(
                    "objectSources and objectNames must have the same length");
        }

        // Add all non-Sync objects to new lists.
        ArrayList<Integer> sources = new ArrayList<Integer>();
        ArrayList<String> names = new ArrayList<String>();
        for (int i = 0; i < objectSources.length; i++) {
            if (objectSources[i] != Types.ObjectSource.CHROME_SYNC) {
                sources.add(objectSources[i]);
                names.add(objectNames[i]);
            }
        }

        Intent registerIntent = new Intent(ACTION_REGISTER);
        registerIntent.putIntegerArrayListExtra(EXTRA_REGISTERED_OBJECT_SOURCES, sources);
        registerIntent.putStringArrayListExtra(EXTRA_REGISTERED_OBJECT_NAMES, names);
        registerIntent.putExtra(EXTRA_ACCOUNT, account);
        return registerIntent;
    }

    /** Returns whether {@code intent} is a stop intent. */
    public static boolean isStop(Intent intent) {
        return intent.getBooleanExtra(EXTRA_STOP, false);
    }

    /** Returns whether {@code intent} is a registered types change intent. */
    public static boolean isRegisteredTypesChange(Intent intent) {
        return intent.hasExtra(EXTRA_REGISTERED_TYPES)
                || intent.hasExtra(EXTRA_REGISTERED_OBJECT_SOURCES);
    }

    /** Returns the object ids for which to register contained in the intent. */
    public static Set<ObjectId> getRegisteredObjectIds(Intent intent) {
        ArrayList<Integer> objectSources =
                intent.getIntegerArrayListExtra(EXTRA_REGISTERED_OBJECT_SOURCES);
        ArrayList<String> objectNames =
                intent.getStringArrayListExtra(EXTRA_REGISTERED_OBJECT_NAMES);
        if (objectSources == null || objectNames == null
                || objectSources.size() != objectNames.size()) {
            return null;
        }
        Set<ObjectId> objectIds = new HashSet<ObjectId>(objectSources.size());
        for (int i = 0; i < objectSources.size(); i++) {
            objectIds.add(ObjectId.newInstance(
                    objectSources.get(i), ApiCompatibilityUtils.getBytesUtf8(objectNames.get(i))));
        }
        return objectIds;
    }

    private InvalidationIntentProtocol() {
        // Disallow instantiation.
    }
}
