// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

/**
 * Tests that assertionss of ReactiveRecorder are thrown.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class ReactiveRecorderTest {
    @Test(expected = AssertionError.class)
    public void testFailEndAtStart() {
        Controller<Unit> controller = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller);
        controller.set(Unit.unit());
        recorder.verify().end();
    }

    @Test(expected = AssertionError.class)
    public void testFailEndAtEnd() {
        Controller<Unit> controller = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller);
        controller.set(Unit.unit());
        controller.reset();
        controller.set(Unit.unit());
        recorder.verify().entered(Unit.unit()).exited().end();
    }

    @Test(expected = AssertionError.class)
    public void testFailEnteredWrongValue() {
        Controller<String> controller = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller);
        controller.set("actual");
        recorder.verify().entered("expected");
    }

    @Test(expected = AssertionError.class)
    public void testFailEnteredGotExit() {
        Controller<String> controller = new Controller<>();
        controller.set("before");
        ReactiveRecorder recorder = ReactiveRecorder.record(controller).reset();
        controller.set("after");
        recorder.verify().entered("after");
    }

    @Test(expected = AssertionError.class)
    public void testFailExitedGotEnter() {
        Controller<Unit> controller = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller);
        controller.set(Unit.unit());
        recorder.verify().exited();
    }

    @Test(expected = AssertionError.class)
    public void testEnteredWrongObservable() {
        Controller<Unit> controller = new Controller<>();
        Controller<Unit> wrong = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller, wrong);
        controller.set(Unit.unit());
        recorder.verify().entered(wrong, Unit.unit());
    }

    @Test(expected = AssertionError.class)
    public void testExitedWrongObservable() {
        Controller<Unit> controller = new Controller<>();
        Controller<Unit> wrong = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller, wrong);
        controller.set(Unit.unit());
        wrong.set(Unit.unit());
        recorder.reset();
        controller.reset();
        recorder.verify().exited(wrong);
    }

    @Test
    public void testHappyPathForOneObservable() {
        Controller<Unit> controller = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(controller);
        controller.set(Unit.unit());
        controller.reset();
        controller.set(Unit.unit());
        controller.reset();
        recorder.verify().entered().exited().entered().exited().end();
    }

    @Test
    public void testHappyPathForManyObservables() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        Controller<String> c = new Controller<>();
        ReactiveRecorder recorder = ReactiveRecorder.record(a, b, c);
        a.set("a");
        b.set("b");
        c.set("c");
        b.reset();
        a.reset();
        c.reset();
        recorder.verify()
                .entered(a, "a")
                .entered(b, "b")
                .entered(c, "c")
                .exited(b)
                .exited(a)
                .exited(c)
                .end();
    }
}
