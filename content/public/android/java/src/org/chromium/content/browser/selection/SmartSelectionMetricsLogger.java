// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.selection;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.IntDef;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionMetricsLogger;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

/**
 * Smart Selection logger, wrapper of Android logger methods.
 * We are logging word indices here. For one example:
 *     New York City , NY
 *    -1   0    1    2 3  4
 * We selected "York" at the first place, so York is [0, 1). Then we assume that Smart Selection
 * expanded the selection range to the whole "New York City , NY", we need to log [-1, 4). After
 * that, we single tap on "City", Smart Selection reset get triggered, we need to log [1, 2). Spaces
 * are ignored but we count each punctuation mark as a word.
 */
@TargetApi(Build.VERSION_CODES.O)
public class SmartSelectionMetricsLogger implements SelectionMetricsLogger {
    private static final String TAG = "SmartSelectionLogger";
    private static final boolean DEBUG = false;

    // Reflection classes, constructor and method.
    private static final String TRACKER_CLASS =
            "android.view.textclassifier.logging.SmartSelectionEventTracker";
    private static Class<?> sTrackerClass;
    private static Class<?> sSelectionEventClass;
    private static Constructor sTrackerConstructor;
    private static Method sLogEventMethod;
    private static boolean sReflectionFailed;

    private Context mContext;
    private Object mTracker;

    private SelectionEventProxy mSelectionEventProxy;
    private SelectionIndicesConverter mConverter;

    // ActionType, from SmartSelectionEventTracker.SelectionEvent class.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ActionType.OVERTYPE, ActionType.COPY, ActionType.PASTE, ActionType.CUT,
            ActionType.SHARE, ActionType.SMART_SHARE, ActionType.DRAG, ActionType.ABANDON,
            ActionType.OTHER, ActionType.SELECT_ALL, ActionType.RESET})
    public @interface ActionType {
        /** User typed over the selection. */
        int OVERTYPE = 100;
        /** User copied the selection. */
        int COPY = 101;
        /** User pasted over the selection. */
        int PASTE = 102;
        /** User cut the selection. */
        int CUT = 103;
        /** User shared the selection. */
        int SHARE = 104;
        /** User clicked the textAssist menu item. */
        int SMART_SHARE = 105;
        /** User dragged+dropped the selection. */
        int DRAG = 106;
        /** User abandoned the selection. */
        int ABANDON = 107;
        /** User performed an action on the selection. */
        int OTHER = 108;

        /* Non-terminal actions. */
        /** User activated Select All */
        int SELECT_ALL = 200;
        /** User reset the smart selection. */
        int RESET = 201;
    }

    /**
     * SmartSelectionEventTracker.SelectionEvent class proxy. Having this interface for testing
     * purpose.
     */
    // TODO(ctzsm): Replace Object with corresponding APIs after Robolectric updated.
    public static interface SelectionEventProxy {
        /**
         * Creates a SelectionEvent for selection started event.
         * @param start Start word index.
         */
        Object createSelectionStarted(int start);

        /**
         * Creates a SelectionEvent for selection modified event.
         * @param start Start word index.
         * @param end End word index.
         */
        Object createSelectionModified(int start, int end);

        /**
         * Creates a SelectionEvent for selection modified event.
         * @param start Start word index.
         * @param end End word index.
         * @param classification {@link android.view.textclassifier.TextClassification object} to
         *                       log.
         */
        Object createSelectionModifiedClassification(int start, int end, Object classification);

        /**
         * Creates a SelectionEvent for selection modified event.
         * @param start Start word index.
         * @param end End word index.
         * @param selection {@link android.view.textclassifier.TextSelection} object to log.
         */
        Object createSelectionModifiedSelection(int start, int end, Object selection);

        /**
         * Creates a SelectionEvent for taking action on selection.
         * @param start Start word index.
         * @param end End word index.
         * @param actionType The action type defined in SelectionMetricsLogger.ActionType.
         */
        Object createSelectionAction(int start, int end, int actionType);

        /**
         * Creates a SelectionEvent for taking action on selection.
         * @param start Start word index.
         * @param end End word index.
         * @param actionType The action type defined in SelectionMetricsLogger.ActionType.
         * @param classification {@link android.view.textclassifier.TextClassification object} to
         *                       log.
         */
        Object createSelectionAction(int start, int end, int actionType, Object classification);
    }

    public static SmartSelectionMetricsLogger create(Context context) {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || context == null
                || sReflectionFailed) {
            return null;
        }

        // TODO(ctzsm): Remove reflection after SDK updates.
        if (sTrackerClass == null) {
            try {
                sTrackerClass = Class.forName(TRACKER_CLASS);
                sSelectionEventClass = Class.forName(TRACKER_CLASS + "$SelectionEvent");
                sTrackerConstructor = sTrackerClass.getConstructor(Context.class, Integer.TYPE);
                sLogEventMethod = sTrackerClass.getMethod("logEvent", sSelectionEventClass);
            } catch (ClassNotFoundException | NoSuchMethodException e) {
                if (DEBUG) Log.d(TAG, "Reflection failure", e);
                sReflectionFailed = true;
                return null;
            }
        }

        SelectionEventProxy selectionEventProxy = SelectionEventProxyImpl.create();
        if (selectionEventProxy == null) return null;

        return new SmartSelectionMetricsLogger(context, selectionEventProxy);
    }

    private SmartSelectionMetricsLogger(Context context, SelectionEventProxy selectionEventProxy) {
        mContext = context;
        mSelectionEventProxy = selectionEventProxy;
    }

    @VisibleForTesting
    protected SmartSelectionMetricsLogger(SelectionEventProxy selectionEventProxy) {
        mSelectionEventProxy = selectionEventProxy;
    }

    public void logSelectionStarted(String selectionText, int startOffset, boolean editable) {
        mTracker = createTracker(mContext, editable);
        mConverter = new SelectionIndicesConverter();
        mConverter.updateSelectionState(selectionText, startOffset);
        mConverter.setInitialStartOffset(startOffset);

        if (DEBUG) Log.d(TAG, "logSelectionStarted");
        logEvent(mSelectionEventProxy.createSelectionStarted(0));
    }

    public void logSelectionModified(
            String selectionText, int startOffset, SelectionClient.Result result) {
        if (mTracker == null) return;
        if (!mConverter.updateSelectionState(selectionText, startOffset)) {
            // DOM change detected, end logging session.
            mTracker = null;
            return;
        }

        int endOffset = startOffset + selectionText.length();
        int[] indices = new int[2];
        if (!mConverter.getWordDelta(startOffset, endOffset, indices)) {
            // Invalid indices, end logging session.
            mTracker = null;
            return;
        }

        if (DEBUG) Log.d(TAG, "logSelectionModified [%d, %d)", indices[0], indices[1]);
        if (result != null && result.textSelection != null) {
            logEvent(mSelectionEventProxy.createSelectionModifiedSelection(
                    indices[0], indices[1], result.textSelection));
        } else if (result != null && result.textClassification != null) {
            logEvent(mSelectionEventProxy.createSelectionModifiedClassification(
                    indices[0], indices[1], result.textClassification));
        } else {
            logEvent(mSelectionEventProxy.createSelectionModified(indices[0], indices[1]));
        }
    }

    public void logSelectionAction(String selectionText, int startOffset, @ActionType int action,
            SelectionClient.Result result) {
        if (mTracker == null) return;
        if (!mConverter.updateSelectionState(selectionText, startOffset)) {
            // DOM change detected, end logging session.
            mTracker = null;
            return;
        }

        int endOffset = startOffset + selectionText.length();
        int[] indices = new int[2];
        if (!mConverter.getWordDelta(startOffset, endOffset, indices)) {
            // Invalid indices, end logging session.
            mTracker = null;
            return;
        }

        if (DEBUG) {
            Log.d(TAG, "logSelectionAction [%d, %d)", indices[0], indices[1]);
            Log.d(TAG, "logSelectionAction ActionType = %d", action);
        }

        if (result != null && result.textClassification != null) {
            logEvent(mSelectionEventProxy.createSelectionAction(
                    indices[0], indices[1], action, result.textClassification));
        } else {
            logEvent(mSelectionEventProxy.createSelectionAction(indices[0], indices[1], action));
        }

        if (isTerminal(action)) mTracker = null;
    }

    public Object createTracker(Context context, boolean editable) {
        try {
            return sTrackerConstructor.newInstance(
                    context, editable ? /* EDIT_WEBVIEW */ 4 : /* WEBVIEW */ 2);
        } catch (ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
        return null;
    }

    // Reflection wrapper of SmartSelectionEventTracker#logEvent(SelectionEvent)
    public void logEvent(Object selectionEvent) {
        if (selectionEvent == null) return;
        try {
            sLogEventMethod.invoke(mTracker, sSelectionEventClass.cast(selectionEvent));
        } catch (ClassCastException | ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, "Reflection failure", e);
        }
    }

    public static boolean isTerminal(@ActionType int actionType) {
        switch (actionType) {
            case ActionType.OVERTYPE: // fall through
            case ActionType.COPY: // fall through
            case ActionType.PASTE: // fall through
            case ActionType.CUT: // fall through
            case ActionType.SHARE: // fall through
            case ActionType.SMART_SHARE: // fall through
            case ActionType.DRAG: // fall through
            case ActionType.ABANDON: // fall through
            case ActionType.OTHER: // fall through
                return true;
            default:
                return false;
        }
    }
}
