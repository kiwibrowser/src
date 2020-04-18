// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import org.chromium.base.CollectionUtil;

import java.util.ArrayList;
import java.util.List;

/**
 * Wraps the {@link ContextualSearchInternalStateController} and adds some simple instrumentation
 * for testing.
 */
class ContextualSearchInternalStateControllerWrapper
        extends ContextualSearchInternalStateController {
    static final List<InternalState> EXPECTED_TAP_RESOLVE_SEQUENCE =
            CollectionUtil.newArrayList(InternalState.TAP_RECOGNIZED,
                    InternalState.TAP_GESTURE_COMMIT, InternalState.GATHERING_SURROUNDINGS,
                    InternalState.DECIDING_SUPPRESSION, InternalState.START_SHOWING_TAP_UI,
                    InternalState.SHOW_FULL_TAP_UI, InternalState.RESOLVING);
    static final List<InternalState> EXPECTED_LONGPRESS_SEQUENCE =
            CollectionUtil.newArrayList(InternalState.LONG_PRESS_RECOGNIZED,
                    InternalState.GATHERING_SURROUNDINGS, InternalState.SHOWING_LONGPRESS_SEARCH);

    private List<InternalState> mStartedStates = new ArrayList<InternalState>();
    private List<InternalState> mFinishedStates = new ArrayList<InternalState>();

    /**
     * Creates a wrapper around a {@link ContextualSearchInternalStateController} with the given
     * parameters.
     * @param policy The {@link ContextualSearchPolicy} to construct the controller with.
     * @param handler The {@link ContextualSearchInternalStateHandler} to use for state transitions.
     */
    private ContextualSearchInternalStateControllerWrapper(
            ContextualSearchPolicy policy, ContextualSearchInternalStateHandler handler) {
        super(policy, handler);
    }

    @Override
    void notifyStartingWorkOn(InternalState state) {
        mStartedStates.add(state);
        super.notifyStartingWorkOn(state);
    }

    @Override
    void notifyFinishedWorkOn(InternalState state) {
        mFinishedStates.add(state);
        super.notifyFinishedWorkOn(state);
    }

    /**
     * @return A {@link List} of all states that were started.
     */
    List<InternalState> getStartedStates() {
        return mStartedStates;
    }

    /**
     * @return A {@link List} of all states that were finished.
     */
    List<InternalState> getFinishedStates() {
        return mFinishedStates;
    }

    /**
     * @return A wrapper for a new {@link ContextualSearchInternalStateHandler} created using
     *         parameters from the given manager.
     */
    static ContextualSearchInternalStateControllerWrapper makeNewInternalStateControllerWrapper(
            ContextualSearchManager manager) {
        return new ContextualSearchInternalStateControllerWrapper(
                manager.getContextualSearchPolicy(),
                manager.getContextualSearchInternalStateHandler());
    }
}
