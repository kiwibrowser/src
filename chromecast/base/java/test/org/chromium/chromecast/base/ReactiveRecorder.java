// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.emptyIterable;

import org.junit.Assert;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Records events emitted by Observables, and provides a fluent interface to perform assertions on
 * the received events. Use this in unit tests to get descriptive output for assertion failures.
 */
public class ReactiveRecorder {
    private final List<Event> mRecord;
    private final Map<Observable<?>, String> mObservableNames;

    public static ReactiveRecorder record(Observable<?>... observables) {
        return new ReactiveRecorder(observables);
    }

    private ReactiveRecorder(Observable<?>... observables) {
        mRecord = new ArrayList<>();
        mObservableNames = new HashMap<>();
        int id = 0;
        for (Observable<?> observable : observables) {
            mObservableNames.put(observable, "Observable" + id++);
            observable.watch((Object value) -> {
                mRecord.add(enterEvent(observable, value));
                return () -> mRecord.add(exitEvent(observable, value));
            });
        }
    }

    public ReactiveRecorder reset() {
        mRecord.clear();
        return this;
    }

    public Validator verify() {
        return new Validator();
    }

    /**
     * The fluent interface used to perform assertions. Each entered() or exited() call pops the
     * least-recently-added event from the record, and verifies that it meets the description
     * provided by the arguments given to entered() or exited(). Use end() to assert that no more
     * events were received.
     */
    public class Validator {
        private Validator() {}

        public Validator entered(Observable observable, Object value) {
            Event event = pop();
            event.checkType("enter");
            event.checkObservable(observable);
            event.checkValue(value);
            return this;
        }

        public Validator entered(Object value) {
            Event event = pop();
            event.checkType("enter");
            event.checkValue(value);
            return this;
        }

        public Validator entered() {
            Event event = pop();
            event.checkType("enter");
            return this;
        }

        public Validator exited(Observable observable) {
            Event event = pop();
            event.checkType("exit");
            event.checkObservable(observable);
            return this;
        }

        public Validator exited() {
            Event event = pop();
            event.checkType("exit");
            return this;
        }

        public void end() {
            Assert.assertThat(mRecord, emptyIterable());
        }
    }

    private Event pop() {
        return mRecord.remove(0);
    }

    private String observableName(Observable<?> observable) {
        String name = mObservableNames.get(observable);
        if (name == null) {
            return "(Unknown)";
        }
        return name;
    }

    private Event enterEvent(Observable<?> observable, Object value) {
        Event result = new Event();
        result.type = "enter";
        result.observable = observable;
        result.value = value;
        return result;
    }

    private Event exitEvent(Observable<?> observable, Object value) {
        Event result = new Event();
        result.type = "exit";
        result.observable = observable;
        result.value = value;
        return result;
    }

    private class Event {
        public String type;
        public Observable<?> observable;
        public Object value;

        private Event() {}

        public void checkType(String type) {
            Assert.assertEquals("Event " + this + " is not an " + type + " event", type, this.type);
        }

        public void checkObservable(Observable<?> observable) {
            Assert.assertEquals("Event " + this + " has wrong observable, expected "
                            + observableName(observable),
                    observable, this.observable);
        }

        public void checkValue(Object value) {
            Assert.assertEquals(
                    "Event " + this + " has wrong value, expected " + value, value, this.value);
        }

        @Override
        public String toString() {
            return "(" + type + " " + observableName(observable) + ": " + value + ")";
        }
    }
}
