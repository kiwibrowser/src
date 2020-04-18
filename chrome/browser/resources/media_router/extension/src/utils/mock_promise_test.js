// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.MockPromiseTest');
goog.setTestOnly('mr.MockPromiseTest');

goog.require('mr.MockPromise');


describe('mr.MockPromise', () => {
  afterEach(() => {
    mr.MockPromise.callPendingHandlers();
  });

  // Simple shared objects used as test values.
  const dummy = {
    toString() {
      return '[object dummy]';
    }
  };
  const sentinel = {
    toString() {
      return '[object sentinel]';
    }
  };

  /**
   * Dummy onfulfilled or onrejected function that should not be called.
   *
   * @param {*} result The result passed into the callback.
   */
  function shouldNotCall(result) {
    fail('This should not have been called (result: ' + String(result) + ')');
  }

  function resolveSoon(value) {
    let resolveFunc;
    const promise = new mr.MockPromise((resolve, reject) => {
      resolveFunc = resolve.bind(null, value);
    });
    return [promise, resolveFunc];
  }

  function rejectSoon(value) {
    let rejectFunc;
    const promise = new mr.MockPromise((resolve, reject) => {
      rejectFunc = reject.bind(null, value);
    });
    return [promise, rejectFunc];
  }

  // A trivial expectation test to suppress Jasmine warnings.
  function noExpectations() {
    expect(true).toBe(true);
  }

  it('testThenIsFulfilled', () => {
    let timesCalled = 0;

    const p = new mr.MockPromise((resolve, reject) => {
      resolve(sentinel);
    });
    p.then(value => {
      timesCalled++;
      expect(value).toBe(sentinel);
    });

    expect(timesCalled).toBe(0);

    mr.MockPromise.callPendingHandlers();
    expect(timesCalled).toBe(1);
  });

  it('testThenIsRejected', () => {
    let timesCalled = 0;

    const p = mr.MockPromise.reject(sentinel);
    p.then(shouldNotCall, value => {
      timesCalled++;
      expect(value).toBe(sentinel);
    });

    expect(timesCalled).toBe(0);

    mr.MockPromise.callPendingHandlers();
    expect(timesCalled).toBe(1);
  });

  it('testThenAsserts', () => {
    const p = mr.MockPromise.resolve();

    expect(() => {
      p.then({});
    }).toThrowError(/onFulfilled should be a function./);

    expect(() => {
      p.then(() => {}, {});
    }).toThrowError(/onRejected should be a function./);
  });

  it('testOptionalOnFulfilled', done => {
    mr.MockPromise.resolve(sentinel)
        .then(null, null)
        .then(null, shouldNotCall)
        .then(value => {
          expect(value).toBe(sentinel);
          done();
        });
    mr.MockPromise.callPendingHandlers();
  });

  it('testOptionalOnRejected', done => {
    mr.MockPromise.reject(sentinel)
        .then(null, null)
        .then(shouldNotCall)
        .then(null, reason => {
          expect(reason).toBe(sentinel);
          done();
        });
    mr.MockPromise.callPendingHandlers();
  });

  it('testMultipleResolves', () => {
    let timesCalled = 0;
    let resolvePromise;

    const p = new mr.MockPromise((resolve, reject) => {
      resolvePromise = resolve;
      resolve('foo');
      resolve('bar');
    });

    p.then(value => {
      timesCalled++;
      expect(timesCalled).toBe(1);
    });

    mr.MockPromise.callPendingHandlers();

    resolvePromise('baz');
    expect(timesCalled).toBe(1);
  });

  it('testMultipleRejects', () => {
    let timesCalled = 0;
    let rejectPromise;

    const p = new mr.MockPromise((resolve, reject) => {
      rejectPromise = reject;
      reject('foo');
      reject('bar');
    });

    p.then(shouldNotCall, value => {
      timesCalled++;
      expect(timesCalled).toBe(1);
    });

    mr.MockPromise.callPendingHandlers();

    rejectPromise('baz');
    expect(timesCalled).toBe(1);
  });

  it('testResolveWithPromise', () => {
    let resolveBlocker;
    let hasFulfilled = false;
    const blocker = new mr.MockPromise((resolve, reject) => {
      resolveBlocker = resolve;
    });

    const p = mr.MockPromise.resolve(blocker);
    p.then(value => {
      hasFulfilled = true;
      expect(value).toBe(sentinel);
    }, shouldNotCall);

    expect(hasFulfilled).toBe(false);
    resolveBlocker(sentinel);

    mr.MockPromise.callPendingHandlers();
    expect(hasFulfilled).toBe(true);
  });

  it('testResolveWithRejectedPromise', () => {
    let rejectBlocker;
    let hasRejected = false;
    const blocker = new mr.MockPromise((resolve, reject) => {
      rejectBlocker = reject;
    });

    const p = mr.MockPromise.resolve(blocker);
    const child = p.then(shouldNotCall, reason => {
      hasRejected = true;
      expect(reason).toBe(sentinel);
    });

    expect(hasRejected).toBe(false);
    rejectBlocker(sentinel);

    mr.MockPromise.callPendingHandlers();
    expect(hasRejected).toBe(true);
  });

  it('testRejectWithPromise', () => {
    let resolveBlocker;
    let hasFulfilled = false;
    const blocker = new mr.MockPromise((resolve, reject) => {
      resolveBlocker = resolve;
    });

    const p = mr.MockPromise.reject(blocker);
    const child = p.then(value => {
      hasFulfilled = true;
      expect(value).toBe(sentinel);
    }, shouldNotCall);

    expect(hasFulfilled).toBe(false);
    resolveBlocker(sentinel);

    mr.MockPromise.callPendingHandlers();
    expect(hasFulfilled).toBe(true);
  });

  it('testRejectWithRejectedPromise', () => {
    let rejectBlocker;
    let hasRejected = false;
    const blocker = new mr.MockPromise((resolve, reject) => {
      rejectBlocker = reject;
    });

    const p = mr.MockPromise.reject(blocker);
    const child = p.then(shouldNotCall, reason => {
      hasRejected = true;
      expect(reason).toBe(sentinel);
    });

    expect(hasRejected).toBe(false);
    rejectBlocker(sentinel);

    mr.MockPromise.callPendingHandlers();
    expect(hasRejected).toBe(true);
  });

  it('testResolveAndReject', () => {
    let onFulfilledCalled = false;
    let onRejectedCalled = false;
    const p = new mr.MockPromise((resolve, reject) => {
      resolve();
      reject();
    });

    p.then(
        () => {
          onFulfilledCalled = true;
        },
        () => {
          onRejectedCalled = true;
        });

    mr.MockPromise.callPendingHandlers();
    expect(onFulfilledCalled).toBe(true);
    expect(onRejectedCalled).toBe(false);
  });

  it('testResolveWithSelfRejects', done => {
    let r;
    const p = new mr.MockPromise(resolve => {
      r = resolve;
    });
    r(p);
    p.then(shouldNotCall, e => {
      expect(e.message).toBe('Promise cannot resolve to itself');
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testResolveWithObjectStringResolves', done => {
    mr.MockPromise.resolve('[object Object]').then(v => {
      expect(v).toBe('[object Object]');
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testRejectAndResolve', done => {
    noExpectations();
    new mr
        .MockPromise((resolve, reject) => {
          reject();
          resolve();
        })
        .then(shouldNotCall, done);
    mr.MockPromise.callPendingHandlers();
  });

  it('testThenReturnsBeforeCallbackWithFulfill', done => {
    let thenHasReturned = false;
    const p = mr.MockPromise.resolve();

    p.then(() => {
      expect(thenHasReturned).toBe(true);
      done();
    });
    thenHasReturned = true;

    mr.MockPromise.callPendingHandlers();
  });

  it('testThenReturnsBeforeCallbackWithReject', done => {
    let thenHasReturned = false;
    const p = mr.MockPromise.reject();

    const child = p.then(shouldNotCall, () => {
      expect(thenHasReturned).toBe(true);
      done();
    });
    thenHasReturned = true;

    mr.MockPromise.callPendingHandlers();
  });

  it('testResolutionOrder', () => {
    const callbacks = [];
    mr.MockPromise.resolve()
        .then(
            () => {
              callbacks.push(1);
            },
            shouldNotCall)
        .then(
            () => {
              callbacks.push(2);
            },
            shouldNotCall)
        .then(() => {
          callbacks.push(3);
        }, shouldNotCall);

    mr.MockPromise.callPendingHandlers();
    expect(callbacks).toEqual([1, 2, 3]);
  });

  it('testResolutionOrderWithThrow', done => {
    const callbacks = [];
    const p = mr.MockPromise.resolve();

    p.then(() => {
      callbacks.push(1);
    }, shouldNotCall);
    const child = p.then(() => {
      callbacks.push(2);
      throw Error();
    }, shouldNotCall);

    child.then(shouldNotCall, () => {
      // The parent callbacks should be evaluated before the child.
      callbacks.push(4);
    });

    p.then(() => {
      callbacks.push(3);
    }, shouldNotCall);

    child.then(shouldNotCall, () => {
      callbacks.push(5);
      expect(callbacks).toEqual([1, 2, 3, 4, 5]);
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testResolutionOrderWithNestedThen', done => {
    const callbacks = [];
    const promise = new mr.MockPromise((resolve, reject) => {
      const p = mr.MockPromise.resolve();

      p.then(() => {
        callbacks.push(1);
        p.then(() => {
          callbacks.push(3);
          resolve();
        });
      });
      p.then(() => {
        callbacks.push(2);
      });
    });

    promise.then(() => {
      expect(callbacks).toEqual([1, 2, 3]);
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testRejectionOrder', () => {
    const callbacks = [];
    const p = mr.MockPromise.reject();

    p.then(shouldNotCall, () => {
      callbacks.push(1);
    });
    p.then(shouldNotCall, () => {
      callbacks.push(2);
    });
    p.then(shouldNotCall, () => {
      callbacks.push(3);
    });

    mr.MockPromise.callPendingHandlers();
    expect(callbacks).toEqual([1, 2, 3]);
  });

  it('testRejectionOrderWithThrow', () => {
    const callbacks = [];
    const p = mr.MockPromise.reject();

    p.then(shouldNotCall, () => {
      callbacks.push(1);
    });
    p.then(shouldNotCall, () => {
       callbacks.push(2);
       throw Error();
     }).catch(() => {});
    p.then(shouldNotCall, () => {
      callbacks.push(3);
    });

    mr.MockPromise.callPendingHandlers();
    expect(callbacks).toEqual([1, 2, 3]);
  });

  it('testRejectionOrderWithNestedThen', done => {
    const callbacks = [];
    const promise = new mr.MockPromise((resolve, reject) => {

      const p = mr.MockPromise.reject();

      p.then(shouldNotCall, () => {
        callbacks.push(1);
        p.then(shouldNotCall, () => {
          callbacks.push(3);
          resolve();
        });
      });
      p.then(shouldNotCall, () => {
        callbacks.push(2);
      });
    });

    promise.then(() => {
      expect(callbacks).toEqual([1, 2, 3]);
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testBranching', () => {
    const p = mr.MockPromise.resolve(2);
    let branchesResolved = 0;

    const branch1 = p.then(value => {
                       expect(value).toBe(2);
                       return value / 2;
                     }).then(value => {
      expect(value).toBe(1);
      ++branchesResolved;
    });

    const branch2 = p.then(value => {
                       expect(value).toBe(2);
                       throw value + 1;
                     }).then(shouldNotCall, reason => {
      expect(reason).toBe(3);
      ++branchesResolved;
    });

    const branch3 = p.then(value => {
                       expect(value).toBe(2);
                       return value * 2;
                     }).then(value => {
      expect(value).toBe(4);
      ++branchesResolved;
    });

    mr.MockPromise.all([branch1, branch2, branch3]);
    mr.MockPromise.callPendingHandlers();
    expect(branchesResolved).toBe(3);
  });

  it('testThenReturnsPromise', () => {
    const parent = mr.MockPromise.resolve();
    const child = parent.then();

    expect(child instanceof mr.MockPromise).toBe(true);
    expect(child).not.toEqual(parent);
  });

  it('testBlockingPromise', () => {
    const p = mr.MockPromise.resolve();
    let wasFulfilled = false;
    let wasRejected = false;

    const p2 = p.then(() => new mr.MockPromise((resolve, reject) => {}));

    p2.then(
        () => {
          wasFulfilled = true;
        },
        () => {
          wasRejected = true;
        });

    mr.MockPromise.callPendingHandlers();
    expect(wasFulfilled).toBe(false);
    expect(wasRejected).toBe(false);
  });

  it('testBlockingPromiseFulfilled', done => {
    let resolveBlockingPromise;
    const blockingPromise = new mr.MockPromise((resolve, reject) => {
      resolveBlockingPromise = resolve;
    });

    const p = mr.MockPromise.resolve(dummy);
    const p2 = p.then(value => blockingPromise);

    p2.then(value => {
      expect(value).toBe(sentinel);
      done();
    });
    resolveBlockingPromise(sentinel);
    mr.MockPromise.callPendingHandlers();
  });

  it('testBlockingPromiseRejected', done => {
    let rejectBlockingPromise;
    const blockingPromise = new mr.MockPromise((resolve, reject) => {
      rejectBlockingPromise = reject;
    });

    const p = mr.MockPromise.resolve(blockingPromise);

    p.then(shouldNotCall, reason => {
      expect(reason).toBe(sentinel);
      done();
    });
    rejectBlockingPromise(sentinel);
    mr.MockPromise.callPendingHandlers();
  });

  it('testCatch', done => {
    let catchCalled = false;
    mr.MockPromise.reject()
        .catch(reason => {
          catchCalled = true;
          return sentinel;
        })
        .then(value => {
          expect(catchCalled).toBe(true);
          expect(value).toBe(sentinel);
          done();
        });
    mr.MockPromise.callPendingHandlers();
  });

  it('testRaceWithEmptyList', done => {
    mr.MockPromise.race([]).then(value => {
      expect(value).toBe(undefined);
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testRaceWithFulfill', done => {
    const [a, resolveA] = resolveSoon('a');
    const [b, resolveB] = resolveSoon('b');
    const [c, resolveC] = resolveSoon('c');
    const [d, resolveD] = resolveSoon('d');

    mr.MockPromise.race([a, b, c, d])
        .then(value => {
          expect(value).toBe('c');
          // Return the slowest input promise to wait for it to complete.
          return a;
        })
        .then(value => {
          expect(value).toBe('a');
          done();
        });
    resolveC();
    resolveD();
    resolveB();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testRaceWithNonThenable', done => {
    const [a, resolveA] = resolveSoon('a');
    const b = 'b';
    const [c, resolveC] = resolveSoon('c');
    const [d, resolveD] = resolveSoon('d');

    mr.MockPromise.race([a, b, c, d])
        .then(value => {
          expect(value).toBe('b');
          // Return the slowest input promise to wait for it to complete.
          return a;
        })
        .then(value => {
          expect(value).toBe('a');
          done();
        });
    resolveC();
    resolveD();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testRaceWithFalseyNonThenable', done => {
    const [a, resolveA] = resolveSoon('a');
    const b = 0;
    const [c, resolveC] = resolveSoon('c');
    const [d, resolveD] = resolveSoon('d');

    mr.MockPromise.race([a, b, c, d])
        .then(value => {
          expect(value).toBe(0);
          // Return the slowest input promise to wait for it to complete.
          return a;
        })
        .then(value => {
          expect(value).toBe('a');
          done();
        });
    resolveC();
    resolveD();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testRaceWithFulfilledBeforeNonThenable', done => {
    const [a, resolveA] = resolveSoon('a');
    const b = mr.MockPromise.resolve('b');
    const c = 'c';
    const [d, resolveD] = resolveSoon('d');

    mr.MockPromise.race([a, b, c, d])
        .then(value => {
          expect(value).toBe('b');
          // Return the slowest input promise to wait for it to complete.
          return a;
        })
        .then(value => {
          expect(value).toBe('a');
          done();
        });
    resolveD();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testRaceWithReject', done => {
    const [a, rejectA] = rejectSoon('rejected-a', 40);
    const [b, rejectB] = rejectSoon('rejected-b', 30);
    const [c, rejectC] = rejectSoon('rejected-c', 10);
    const [d, rejectD] = rejectSoon('rejected-d', 20);

    mr.MockPromise.race([a, b, c, d])
        .then(
            shouldNotCall,
            value => {
              expect(value).toBe('rejected-c');
              return a;
            })
        .then(shouldNotCall, reason => {
          expect(reason).toBe('rejected-a');
          done();
        });
    rejectC();
    rejectD();
    rejectB();
    rejectA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testAllWithEmptyList', done => {
    mr.MockPromise.all([]).then(value => {
      expect(value).toEqual([]);
      done();
    });
    mr.MockPromise.callPendingHandlers();
  });

  it('testAllWithFulfill', done => {
    const [a, resolveA] = resolveSoon('a');
    const [b, resolveB] = resolveSoon('b');
    const [c, resolveC] = resolveSoon('c');
    const [d, resolveD] = resolveSoon('d');
    // Test a falsey value.
    const [z, resolveZ] = resolveSoon(0);

    mr.MockPromise.all([a, b, c, d, z]).then(value => {
      expect(value).toEqual(['a', 'b', 'c', 'd', 0]);
      done();
    });
    resolveC();
    resolveD();
    resolveB();
    resolveZ();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testAllWithNonThenable', done => {
    const [a, resolveA] = resolveSoon('a');
    const b = 'b';
    const [c, resolveC] = resolveSoon('c');
    const [d, resolveD] = resolveSoon('d');
    // Test a falsey value.
    const z = 0;

    mr.MockPromise.all([a, b, c, d, z]).then(value => {
      expect(value).toEqual(['a', 'b', 'c', 'd', 0]);
      done();
    });
    resolveC();
    resolveD();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testAllWithReject', done => {
    const [a, resolveA] = resolveSoon('a');
    const [b, rejectB] = rejectSoon('rejected-b');
    const [c, resolveC] = resolveSoon('c');
    const [d, resolveD] = resolveSoon('d');

    mr.MockPromise.all([a, b, c, d])
        .then(
            shouldNotCall,
            reason => {
              expect(reason).toBe('rejected-b');
              return a;
            })
        .then(value => {
          expect(value).toBe('a');
          done();
        });
    resolveC();
    resolveD();
    rejectB();
    resolveA();
    mr.MockPromise.callPendingHandlers();
  });

  it('testMockClock', () => {
    let resolveA;
    let resolveB;
    const calls = [];

    const p = new mr.MockPromise((resolve, reject) => {
      resolveA = resolve;
    });

    p.then(value => {
      expect(value).toBe(sentinel);
      calls.push('then');
    });

    const fulfilledChild = p.then(value => {
                              expect(value).toBe(sentinel);
                              return mr.MockPromise.resolve(1);
                            }).then(value => {
      expect(value).toBe(1);
      calls.push('fulfilledChild');

    });

    const rejectedChild = p.then(value => {
                             expect(value).toBe(sentinel);
                             return mr.MockPromise.reject(2);
                           }).then(shouldNotCall, reason => {
      expect(reason).toBe(2);
      calls.push('rejectedChild');
    });

    const unresolvedChild = p.then(value => {
                               expect(value).toBe(sentinel);
                               return new mr.MockPromise(r => {
                                 resolveB = r;
                               });
                             }).then(value => {
      expect(value).toBe(3);
      calls.push('unresolvedChild');
    });

    resolveA(sentinel);
    expect(calls).toEqual([]);

    mr.MockPromise.callPendingHandlers();
    expect(calls).toEqual(['then', 'fulfilledChild', 'rejectedChild']);

    resolveB(3);
    expect(calls).toEqual(['then', 'fulfilledChild', 'rejectedChild']);

    mr.MockPromise.callPendingHandlers();
    expect(calls).toEqual(
        ['then', 'fulfilledChild', 'rejectedChild', 'unresolvedChild']);
  });

  it('testHandledRejection', () => {
    noExpectations();
    mr.MockPromise.reject(sentinel).then(shouldNotCall, reason => {});
    mr.MockPromise.callPendingHandlers();
  });

  it('testUnhandledRejection1', () => {
    mr.MockPromise.reject(sentinel);
    expect(mr.MockPromise.callPendingHandlers).toThrow();
  });

  it('testUnhandledRejection2', () => {
    mr.MockPromise.reject(sentinel).then(shouldNotCall);
    expect(mr.MockPromise.callPendingHandlers).toThrow();
  });

  it('testUnhandledThrow', () => {
    mr.MockPromise.resolve().then(() => {
      throw sentinel;
    });
    expect(mr.MockPromise.callPendingHandlers).toThrow();
  });

  it('testUnhandledBlockingRejection', () => {
    const blocker = mr.MockPromise.reject(sentinel);
    mr.MockPromise.resolve(blocker);
    expect(mr.MockPromise.callPendingHandlers).toThrow();
  });

  it('testHandledBlockingRejection', () => {
    noExpectations();
    const blocker = mr.MockPromise.reject(sentinel);
    mr.MockPromise.resolve(blocker).then(shouldNotCall, reason => {});
    mr.MockPromise.callPendingHandlers();
  });
});
