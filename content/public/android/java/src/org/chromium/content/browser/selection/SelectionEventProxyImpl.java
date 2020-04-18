// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.selection;

import android.annotation.TargetApi;
import android.os.Build;
import android.support.annotation.NonNull;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextSelection;

import org.chromium.base.Log;

import java.lang.reflect.Method;

/**
 * A wrapper class for Android SmartSelectionEventTracker.SelectionEvent.
 * SmartSelectionEventTracker.SelectionEvent is a hidden class, need reflection to access to it.
 */
@TargetApi(Build.VERSION_CODES.O)
public class SelectionEventProxyImpl implements SmartSelectionMetricsLogger.SelectionEventProxy {
    private static final String TAG = "SmartSelectionLogger";
    private static final boolean DEBUG = false;
    private static final String SELECTION_EVENT_CLASS =
            "android.view.textclassifier.logging.SmartSelectionEventTracker$SelectionEvent";

    // Reflection class and methods.
    private static Class<?> sSelectionEventClass;
    private static Method sSelectionStartedMethod;
    private static Method sSelectionModifiedMethod;
    private static Method sSelectionModifiedClassificationMethod;
    private static Method sSelectionModifiedSelectionMethod;
    private static Method sSelectionActionMethod;
    private static Method sSelectionActionClassificationMethod;
    private static boolean sReflectionFailed = false;

    public static SelectionEventProxyImpl create() {
        if (sReflectionFailed) return null;
        // TODO(ctzsm): Remove reflection after SDK updates.
        if (sSelectionEventClass == null) {
            try {
                sSelectionEventClass = Class.forName(SELECTION_EVENT_CLASS);
                sSelectionStartedMethod =
                        sSelectionEventClass.getMethod("selectionStarted", Integer.TYPE);
                sSelectionModifiedMethod = sSelectionEventClass.getMethod(
                        "selectionModified", Integer.TYPE, Integer.TYPE);
                sSelectionModifiedClassificationMethod = sSelectionEventClass.getMethod(
                        "selectionModified", Integer.TYPE, Integer.TYPE, TextClassification.class);
                sSelectionModifiedSelectionMethod = sSelectionEventClass.getMethod(
                        "selectionModified", Integer.TYPE, Integer.TYPE, TextSelection.class);
                sSelectionActionMethod = sSelectionEventClass.getMethod(
                        "selectionAction", Integer.TYPE, Integer.TYPE, Integer.TYPE);
                sSelectionActionClassificationMethod =
                        sSelectionEventClass.getMethod("selectionAction", Integer.TYPE,
                                Integer.TYPE, Integer.TYPE, TextClassification.class);
            } catch (ClassNotFoundException | NoSuchMethodException e) {
                if (DEBUG) Log.d(TAG, "Reflection failure", e);
                sReflectionFailed = true;
                return null;
            }
        }
        return new SelectionEventProxyImpl();
    }

    private SelectionEventProxyImpl() {}

    // Reflection wrapper of SelectionEvent#selectionStarted(int)
    @Override
    public Object createSelectionStarted(int start) {
        try {
            return sSelectionStartedMethod.invoke(null, start);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int)
    @Override
    public Object createSelectionModified(int start, int end) {
        try {
            return sSelectionModifiedMethod.invoke(null, start, end);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int, TextClassification)
    @Override
    public Object createSelectionModifiedClassification(
            int start, int end, @NonNull Object classification) {
        try {
            return sSelectionModifiedClassificationMethod.invoke(
                    null, start, end, (TextClassification) classification);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int, TextSelection)
    @Override
    public Object createSelectionModifiedSelection(int start, int end, @NonNull Object selection) {
        try {
            return sSelectionModifiedSelectionMethod.invoke(
                    null, start, end, (TextSelection) selection);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionAction(int, int, int)
    @Override
    public Object createSelectionAction(int start, int end, int actionType) {
        try {
            return sSelectionActionMethod.invoke(null, start, end, actionType);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionAction(int, int, int, TextClassification)
    @Override
    public Object createSelectionAction(
            int start, int end, int actionType, @NonNull Object classification) {
        try {
            return sSelectionActionClassificationMethod.invoke(
                    null, start, end, actionType, (TextClassification) classification);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }
}
